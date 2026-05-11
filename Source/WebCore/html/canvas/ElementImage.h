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

#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TZoneMalloc.h>

namespace WebCore {

class CanvasChildPaintRecord;

// Snapshot of an HTML element captured by HTMLCanvasElement.captureElementImage()
// (TB6). Wraps a Ref to the canvas's CanvasChildPaintRecord at the moment of
// capture; subsequent paints replace the canvas's stored record without
// affecting the snapshot held here, so an ElementImage always carries the
// last-painted geometry at the time captureElementImage() was called.
//
// close() drops the record reference and zeros the reported width/height; a
// subsequent drawElementImage(ElementImage, ...) throws InvalidStateError per
// the WICG spec. Per-instance lifetime — multiple ElementImage objects can
// reference the same record cheaply (one allocation, one display list).
//
// [Transferable] across postMessage is TB7 territory; this skeleton is
// main-thread / Window-exposed only.
class ElementImage : public RefCounted<ElementImage> {
    WTF_MAKE_TZONE_ALLOCATED(ElementImage);
public:
    static Ref<ElementImage> create(Ref<CanvasChildPaintRecord>&&);
    ~ElementImage();

    double width() const;
    double height() const;
    void close();

    bool isClosed() const { return !m_record; }
    const CanvasChildPaintRecord* record() const { return m_record.get(); }

private:
    explicit ElementImage(Ref<CanvasChildPaintRecord>&&);

    RefPtr<CanvasChildPaintRecord> m_record;
};

} // namespace WebCore
