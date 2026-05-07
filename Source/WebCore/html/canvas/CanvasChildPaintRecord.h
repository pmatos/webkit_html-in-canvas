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

#pragma once

#include "FloatSize.h"
#include <wtf/Ref.h>
#include <wtf/TZoneMalloc.h>

namespace WebCore {

namespace DisplayList {
class DisplayList;
}

// Metadata captured at recording time alongside the DisplayList. Consumed at replay time
// (CanvasRenderingContext2DBase::drawElementImage) and by TB3a's alignment-matrix code,
// which will read transform-origin / zoom / canvas backing-store dims to build T_align.
struct CanvasChildPaintState {
    FloatSize boxSize; // child's CSS border-box size
    FloatSize canvasBackingStoreSize; // canvas.width/height in device px
    float childZoom { 1.0f }; // CSS effective zoom on the child
    // transform-origin is intentionally omitted here: TB1b returns identity matrix and
    // does not consume it. TB3a will store it (likely as Style::TransformOrigin) when it
    // builds the real alignment matrix.
};

// Snapshot of one direct child of a <canvas layoutsubtree>: a recorded display list plus
// the metadata needed to project it onto the canvas grid at replay time.
//
// Thread-safety: the wrapped DisplayList is main-thread-only (see DisplayList.h). TB7's
// ElementImage transfer will introduce a serialised companion form rather than retrofit
// this struct.
class CanvasChildPaintRecord {
    WTF_MAKE_TZONE_ALLOCATED(CanvasChildPaintRecord);
    WTF_MAKE_NONCOPYABLE(CanvasChildPaintRecord);
public:
    CanvasChildPaintRecord(Ref<const DisplayList::DisplayList>&&, CanvasChildPaintState);
    ~CanvasChildPaintRecord();

    const DisplayList::DisplayList& displayList() const { return m_displayList.get(); }
    const CanvasChildPaintState& state() const { return m_state; }

private:
    Ref<const DisplayList::DisplayList> m_displayList;
    CanvasChildPaintState m_state;
};

} // namespace WebCore
