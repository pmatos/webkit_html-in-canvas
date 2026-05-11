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

// Rasterises a CanvasChildPaintRecord into an Image suitable for
// WebGLRenderingContextBase::texImageImpl (TB8) and, in the future,
// GPUQueue::copyElementImageToTexture (TB9).
//
// sourceRect is the rectangle of the recorded element to copy, in the element's
// natural CSS-px coordinate space. destSize is the output pixel size; when
// destSize differs from sourceRect's size the display list is replayed at the
// resulting scale.
//
// Returns nullptr on zero-area sourceRect / destSize, or on ImageBuffer
// allocation failure. Callers treat null as a silent no-op (matches the
// zero-area behaviour pinned by tex-element-image-2d-basic.html:83-84).
WEBCORE_EXPORT RefPtr<Image> rasterizeCanvasChildPaintRecord(const CanvasChildPaintRecord&, const FloatRect& sourceRect, const IntSize& destSize);

} // namespace WebCore
