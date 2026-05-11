/*
 * Copyright (C) 2026 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <WebCore/CanvasChildPaintRecord.h>
#include <WebCore/CommonAtomStrings.h>
#include <WebCore/DisplayList.h>
#include <WebCore/DocumentInlines.h>
#include <WebCore/DrawElementImageEligibility.h>
#include <WebCore/ExceptionCode.h>
#include <WebCore/HTMLBodyElement.h>
#include <WebCore/HTMLCanvasElement.h>
#include <WebCore/HTMLDivElement.h>
#include <WebCore/HTMLHtmlElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/Node.h>
#include <WebCore/ProcessWarming.h>
#include <WebCore/Settings.h>

namespace TestWebKitAPI {

using namespace WebCore;

static Ref<Document> createDocument()
{
    ProcessWarming::initializeNames();
    auto settings = Settings::create(nullptr);
    auto document = Document::create(settings.get(), aboutBlankURL());
    auto html = HTMLHtmlElement::create(document);
    document->appendChild(html);
    auto body = HTMLBodyElement::create(document);
    html->appendChild(body);
    return document;
}

// Rule 2 (issue #9): a non-direct child throws TypeError, not InvalidStateError.
// WPT pin: wpt_internal/html/canvas/drawElementImage/error-conditions.html:35-37 uses
// assert_throws_js(TypeError, …, "Can't draw non-direct children.").
// This test was the canary for the bug fix in TB4: pre-TB4 the inline validator at
// HTMLCanvasElement.cpp:632 returned InvalidStateError for this case.
TEST(DrawElementImageEligibility, NotDirectChildThrowsTypeError)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    canvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    document->body()->appendChild(canvas);

    auto child = HTMLDivElement::create(document);
    canvas->appendChild(child);
    auto grandchild = HTMLDivElement::create(document);
    child->appendChild(grandchild);

    auto result = verifyDrawElementImageEligibility(canvas.get(), grandchild.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::TypeError);
}

// Rule 3 (issue #9): the canvas must carry the layoutsubtree attribute.
// WPT pin: error-conditions.html:40-42 — assert_throws_dom("InvalidStateError", ...).
TEST(DrawElementImageEligibility, NoLayoutSubtreeAttributeThrows)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto child = HTMLDivElement::create(document);
    canvas->appendChild(child);

    auto result = verifyDrawElementImageEligibility(canvas.get(), child.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::InvalidStateError);
}

// Rule 1 (issue #9): a canvas inside another <canvas layoutsubtree> throws
// NotSupportedError. WPT pin: nested-layoutsubtree-canvas.html:17-18.
// Ancestor walk only — does not gate on isInCanvasSubtree() (which is set on every
// canvas including the outer one).
TEST(DrawElementImageEligibility, NestedCanvasThrowsNotSupported)
{
    auto document = createDocument();
    auto outerCanvas = HTMLCanvasElement::create(document);
    outerCanvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    document->body()->appendChild(outerCanvas);

    auto innerCanvas = HTMLCanvasElement::create(document);
    innerCanvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    outerCanvas->appendChild(innerCanvas);

    auto child = HTMLDivElement::create(document);
    innerCanvas->appendChild(child);

    auto result = verifyDrawElementImageEligibility(innerCanvas.get(), child.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::NotSupportedError);
}

// Rule 4 (issue #9): if all other checks pass but no snapshot exists yet, throw
// InvalidStateError. WPT pin: draw-element-image-empty.html:19-22.
TEST(DrawElementImageEligibility, NoSnapshotThrows)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    canvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    document->body()->appendChild(canvas);

    auto child = HTMLDivElement::create(document);
    canvas->appendChild(child);

    auto result = verifyDrawElementImageEligibility(canvas.get(), child.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::InvalidStateError);
}

// Success path: canvas + direct child + layoutsubtree + recorded snapshot returns
// the record. Verifies callers can replay without a second map lookup.
TEST(DrawElementImageEligibility, EligibleSuccess)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    canvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    document->body()->appendChild(canvas);

    auto child = HTMLDivElement::create(document);
    canvas->appendChild(child);
    auto displayList = DisplayList::DisplayList::create({ });
    auto record = CanvasChildPaintRecord::create(WTF::move(displayList), CanvasChildPaintState { });
    auto* recordPtr = record.ptr();
    canvas->setCanvasChildPaintRecord(child.get(), child->nodeIdentifier(), WTF::move(record));

    auto result = verifyDrawElementImageEligibility(canvas.get(), child.get());
    ASSERT_FALSE(result.hasException());
    EXPECT_EQ(result.returnValue(), recordPtr);
}

// Order-of-checks (issue #9): when both rule 2 (non-direct child) and rule 3
// (missing layoutsubtree) would trip, rule 2 wins → TypeError. Documents the
// Chrome-parity choice; the corpus tests do not stack violations and so do not
// pin order, but this gtest does.
TEST(DrawElementImageEligibility, OrderTypeErrorBeforeInvalidState)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    // Deliberately do NOT set layoutsubtree — both rule 2 and rule 3 would fire.
    document->body()->appendChild(canvas);

    auto child = HTMLDivElement::create(document);
    canvas->appendChild(child);
    auto grandchild = HTMLDivElement::create(document);
    child->appendChild(grandchild);

    auto result = verifyDrawElementImageEligibility(canvas.get(), grandchild.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::TypeError)
        << "Direct-child check (TypeError) wins over layoutsubtree check (InvalidStateError)";
}

// Order-of-checks: nested-canvas is checked before direct-child. An inner canvas
// that is also called with a non-direct-child element still gets NotSupportedError.
TEST(DrawElementImageEligibility, OrderNestedBeforeDirectChild)
{
    auto document = createDocument();
    auto outerCanvas = HTMLCanvasElement::create(document);
    outerCanvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    document->body()->appendChild(outerCanvas);

    auto innerCanvas = HTMLCanvasElement::create(document);
    innerCanvas->setAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr, emptyAtom());
    outerCanvas->appendChild(innerCanvas);

    // grandchild lives in the inner canvas's subtree but is not a direct child.
    auto child = HTMLDivElement::create(document);
    innerCanvas->appendChild(child);
    auto grandchild = HTMLDivElement::create(document);
    child->appendChild(grandchild);

    auto result = verifyDrawElementImageEligibility(innerCanvas.get(), grandchild.get());
    ASSERT_TRUE(result.hasException());
    EXPECT_EQ(result.exception().code(), ExceptionCode::NotSupportedError)
        << "Nested-canvas check (NotSupportedError) wins over direct-child check (TypeError)";
}

} // namespace TestWebKitAPI
