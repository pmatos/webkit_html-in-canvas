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

#include "FloatRect.h"
#include "IntSize.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class CanvasChildPaintRecord;
class Image;
class ImageBuffer;

// Rasterises a CanvasChildPaintRecord into an ImageBuffer. WebGPU's
// GPUQueue::copyElementImageToTexture (TB9) reads pixels back out of the
// buffer via ImageBuffer::getPixelBuffer; WebGL's texElementImage2D (TB8)
// receives an Image wrapper via the sibling rasterizeCanvasChildPaintRecord
// below.
//
// sourceRect is in the element's natural CSS-px coordinate space; destSize is
// the output pixel size. When destSize differs from sourceRect's size the
// display list is replayed at the resulting scale.
//
// Returns nullptr on zero-area sourceRect / destSize, or on ImageBuffer
// allocation failure. Callers treat null as a silent no-op.
RefPtr<ImageBuffer> rasterizeCanvasChildPaintRecordToBuffer(const CanvasChildPaintRecord&, const FloatRect& sourceRect, const IntSize& destSize);

// Thin wrapper around rasterizeCanvasChildPaintRecordToBuffer that returns the
// rasterised buffer as a BitmapImage, suitable for
// WebGLRenderingContextBase::texImageImpl.
//
// Returns nullptr on the same conditions as the sibling helper. Callers treat
// null as a silent no-op (matches the zero-area behaviour pinned by
// tex-element-image-2d-basic.html:83-84).
WEBCORE_EXPORT RefPtr<Image> rasterizeCanvasChildPaintRecord(const CanvasChildPaintRecord&, const FloatRect& sourceRect, const IntSize& destSize);

} // namespace WebCore
