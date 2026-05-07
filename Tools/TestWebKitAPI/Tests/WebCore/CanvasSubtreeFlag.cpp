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

#include <WebCore/CommonAtomStrings.h>
#include <WebCore/DocumentInlines.h>
#include <WebCore/HTMLBodyElement.h>
#include <WebCore/HTMLCanvasElement.h>
#include <WebCore/HTMLDivElement.h>
#include <WebCore/HTMLHtmlElement.h>
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

TEST(CanvasSubtreeFlag, CanvasItselfIsInCanvasSubtreeAtConstruction)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    EXPECT_TRUE(canvas->isInCanvasSubtree());
}

TEST(CanvasSubtreeFlag, DivAppendedToCanvasInheritsFlag)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto div = HTMLDivElement::create(document);
    EXPECT_FALSE(div->isInCanvasSubtree()) << "newly-created div has no flag";

    canvas->appendChild(div);
    EXPECT_TRUE(div->isInCanvasSubtree()) << "div appended to canvas inherits flag";
}

TEST(CanvasSubtreeFlag, DivRemovedFromCanvasLosesFlag)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto div = HTMLDivElement::create(document);
    canvas->appendChild(div);
    EXPECT_TRUE(div->isInCanvasSubtree());

    document->body()->appendChild(div); // moves out of canvas
    EXPECT_FALSE(div->isInCanvasSubtree()) << "div moved out of canvas loses flag";
}

TEST(CanvasSubtreeFlag, GrandchildOfCanvasInheritsFlag)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto outer = HTMLDivElement::create(document);
    auto inner = HTMLDivElement::create(document);
    outer->appendChild(inner);
    canvas->appendChild(outer); // attaches the whole subtree

    EXPECT_TRUE(outer->isInCanvasSubtree());
    EXPECT_TRUE(inner->isInCanvasSubtree()) << "grandchild also inherits";
}

TEST(CanvasSubtreeFlag, ElementOutsideCanvasHasNoFlag)
{
    auto document = createDocument();
    auto div = HTMLDivElement::create(document);
    document->body()->appendChild(div);
    EXPECT_FALSE(div->isInCanvasSubtree());
}

} // namespace TestWebKitAPI
