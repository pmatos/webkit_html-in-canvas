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
#include "TextureUploadFromSnapshot.h"

#include "BitmapImage.h"
#include "CanvasChildPaintRecord.h"
#include "DestinationColorSpace.h"
#include "DisplayList.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "NativeImage.h"
#include "PixelFormat.h"

namespace WebCore {

RefPtr<ImageBuffer> rasterizeCanvasChildPaintRecordToBuffer(const CanvasChildPaintRecord& record, const FloatRect& sourceRect, const IntSize& destSize)
{
    if (sourceRect.isEmpty() || destSize.isEmpty())
        return nullptr;

    auto buffer = ImageBuffer::create(FloatSize(destSize), RenderingMode::Unaccelerated, RenderingPurpose::Canvas, 1.0f, DestinationColorSpace::SRGB(), PixelFormat::BGRA8);
    if (!buffer)
        return nullptr;

    auto& gc = buffer->context();
    if (destSize.width() != sourceRect.width() || destSize.height() != sourceRect.height())
        gc.scale({ destSize.width() / sourceRect.width(), destSize.height() / sourceRect.height() });
    gc.translate(-sourceRect.x(), -sourceRect.y());
    if (RefPtr rasterized = record.rasterizedImage()) {
        // TB7: a transferred-and-reconstructed ElementImage carries pre-rasterized
        // pixels. Paint them directly — GraphicsContext::drawDisplayList calls
        // ControlFactory::singleton() which is main-thread-only and would crash on
        // a worker-backed OffscreenCanvas-WebGL context.
        FloatRect imageRect { FloatPoint { }, FloatSize { rasterized->size() } };
        gc.drawNativeImage(*rasterized, imageRect, imageRect);
    } else
        gc.drawDisplayList(record.displayList());

    return buffer;
}

RefPtr<Image> rasterizeCanvasChildPaintRecord(const CanvasChildPaintRecord& record, const FloatRect& sourceRect, const IntSize& destSize)
{
    auto buffer = rasterizeCanvasChildPaintRecordToBuffer(record, sourceRect, destSize);
    if (!buffer)
        return nullptr;
    return BitmapImage::create(buffer->copyNativeImage());
}

} // namespace WebCore
