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
#include <WebCore/HTMLBodyElement.h>
#include <WebCore/HTMLCanvasElement.h>
#include <WebCore/HTMLDivElement.h>
#include <WebCore/HTMLHtmlElement.h>
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

// Helper to fabricate an empty CanvasChildPaintRecord for tests that exercise the store
// API but don't care about pixel content.
static std::unique_ptr<CanvasChildPaintRecord> makeEmptyRecord()
{
    auto displayList = DisplayList::DisplayList::create({ });
    return makeUnique<CanvasChildPaintRecord>(WTF::move(displayList), CanvasChildPaintState { });
}

TEST(CanvasChildSnapshotStore, EmptyByDefault)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 0u)
        << "fresh canvas should have no recorded child snapshots";
}

TEST(CanvasChildSnapshotStore, RecordSetGetClear)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto div = HTMLDivElement::create(document);
    canvas->appendChild(div);
    auto id = div->nodeIdentifier();

    EXPECT_EQ(canvas->canvasChildPaintRecord(id), nullptr) << "no record before set";
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 0u);

    canvas->setCanvasChildPaintRecord(id, makeEmptyRecord());
    EXPECT_NE(canvas->canvasChildPaintRecord(id), nullptr) << "record present after set";
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 1u);

    canvas->clearCanvasChildPaintRecord(id);
    EXPECT_EQ(canvas->canvasChildPaintRecord(id), nullptr) << "record gone after clear";
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 0u);
}

TEST(CanvasChildSnapshotStore, ClearAllOnResize)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto div1 = HTMLDivElement::create(document);
    auto div2 = HTMLDivElement::create(document);
    canvas->appendChild(div1);
    canvas->appendChild(div2);

    canvas->setCanvasChildPaintRecord(div1->nodeIdentifier(), makeEmptyRecord());
    canvas->setCanvasChildPaintRecord(div2->nodeIdentifier(), makeEmptyRecord());
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 2u);

    auto result = canvas->setWidth(200);
    EXPECT_FALSE(result.hasException()) << "setWidth must not throw on a free-standing canvas";

    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 0u)
        << "canvas resize must clear all child snapshots";
}

TEST(CanvasChildSnapshotStore, ChildrenChangedRemovesStaleEntries)
{
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto div1 = HTMLDivElement::create(document);
    auto div2 = HTMLDivElement::create(document);
    canvas->appendChild(div1);
    canvas->appendChild(div2);

    auto id1 = div1->nodeIdentifier();
    auto id2 = div2->nodeIdentifier();
    canvas->setCanvasChildPaintRecord(id1, makeEmptyRecord());
    canvas->setCanvasChildPaintRecord(id2, makeEmptyRecord());
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 2u);

    // Move div1 out of the canvas (still in the document, but no longer the canvas's child).
    document->body()->appendChild(div1);

    EXPECT_EQ(canvas->canvasChildPaintRecord(id1), nullptr) << "removed child's record purged";
    EXPECT_NE(canvas->canvasChildPaintRecord(id2), nullptr) << "remaining child's record kept";
    EXPECT_EQ(canvas->canvasChildPaintRecordCount(), 1u);
}

TEST(CanvasChildSnapshotStore, NodeIdentifierMonotonic)
{
    // ObjectIdentifier (NodeIdentifier's underlying type) is monotonic — slots are never reused
    // after destruction. This invariant lets the snapshot store key on NodeIdentifier safely:
    // a stale entry from a destroyed element cannot collide with a future element's identifier.
    auto document = createDocument();
    auto canvas = HTMLCanvasElement::create(document);
    document->body()->appendChild(canvas);

    auto childA = HTMLDivElement::create(document);
    canvas->appendChild(childA);
    auto idA = childA->nodeIdentifier();
    canvas->setCanvasChildPaintRecord(idA, makeEmptyRecord());
    canvas->removeChild(childA);
    EXPECT_EQ(canvas->canvasChildPaintRecord(idA), nullptr) << "purged on removal";

    auto childB = HTMLDivElement::create(document);
    canvas->appendChild(childB);
    auto idB = childB->nodeIdentifier();
    EXPECT_NE(idA, idB) << "fresh element gets a fresh NodeIdentifier (no slot recycling)";
}

} // namespace TestWebKitAPI
