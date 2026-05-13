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

#include "FloatPoint.h"
#include "FloatSize.h"
#include "NativeImage.h"
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TZoneMalloc.h>

namespace WebCore {

namespace DisplayList {
class DisplayList;
}

// Metadata captured at recording time alongside the DisplayList. Consumed at replay time
// (CanvasRenderingContext2DBase::drawElementImage) and by DrawElementImageMath when
// building the T_align matrix returned to script.
struct CanvasChildPaintState {
    FloatSize boxSize; // child's CSS border-box size
    FloatSize canvasBackingStoreSize; // canvas.width/height in device px
    float childZoom { 1.0f }; // CSS effective zoom on the child
    FloatPoint transformOrigin; // element transform-origin, unzoomed CSS px, element-local
    // TB6: the display list's draw operations are in absolute (document) coordinates,
    // because RenderLayer::paintLayer applies the layer's offset to the recorder's CTM.
    // Replay needs to subtract this origin so drawElementImage(source, dx, dy) lands
    // pixels at (dx, dy) — the spec's "element image" semantics — independent of the
    // canvas's document position. Captured as borderBox.location() expressed in
    // recording-space (the canvas's absolute origin + the child's local borderBox.x/y).
    FloatPoint recordingOrigin;
    // TB7: source canvas's renderBox()->size() / usedZoom() at recording time —
    // i.e. the unzoomed CSS px dimensions of the source canvas. Captured here so the
    // alignment-matrix path on an OffscreenCanvas receiver (worker side, no live
    // renderBox available) can still compute cssToGridScale as
    // receiverCanvas.size() / sourceUnzoomedCSSSize.
    FloatSize sourceUnzoomedCSSSize;
};

// Snapshot of one direct child of a <canvas layoutsubtree>: a recorded display list plus
// the metadata needed to project it onto the canvas grid at replay time.
//
// RefCounted so a record can be shared between the canvas's snapshot map and any
// ElementImage objects produced by canvas.captureElementImage(). The wrapped
// DisplayList is itself a Ref<const DisplayList::DisplayList>, so two records
// pointing at the same display list pay one allocation, not two.
//
// Thread-safety: the wrapped DisplayList is main-thread-only (see DisplayList.h). TB7's
// ElementImage transfer will introduce a serialised companion form rather than retrofit
// this struct.
class CanvasChildPaintRecord : public RefCounted<CanvasChildPaintRecord> {
    WTF_MAKE_TZONE_ALLOCATED(CanvasChildPaintRecord);
public:
    static Ref<CanvasChildPaintRecord> create(Ref<const DisplayList::DisplayList>&& displayList, CanvasChildPaintState state)
    {
        return adoptRef(*new CanvasChildPaintRecord(WTF::move(displayList), nullptr, state));
    }
    // TB7: rasterized form used when an ElementImage is reconstructed on the
    // receiving side of a postMessage transfer. The DisplayList is still kept
    // (a single-item DrawNativeImage list) so callers that walk the list see
    // the same shape, but replay paths prefer rasterizedImage() to avoid the
    // main-thread-only ControlFactory inside GraphicsContext::drawDisplayList.
    WEBCORE_EXPORT static Ref<CanvasChildPaintRecord> createFromRasterized(Ref<const DisplayList::DisplayList>&&, Ref<NativeImage>&&, CanvasChildPaintState);
    ~CanvasChildPaintRecord();

    const DisplayList::DisplayList& displayList() const { return m_displayList.get(); }
    NativeImage* rasterizedImage() const { return m_rasterizedImage.get(); }
    const CanvasChildPaintState& state() const { return m_state; }

private:
    CanvasChildPaintRecord(Ref<const DisplayList::DisplayList>&&, RefPtr<NativeImage>&&, CanvasChildPaintState);

    Ref<const DisplayList::DisplayList> m_displayList;
    RefPtr<NativeImage> m_rasterizedImage;
    CanvasChildPaintState m_state;
};

} // namespace WebCore
