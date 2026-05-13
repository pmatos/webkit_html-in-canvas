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
#include "ElementImage.h"

#include "CanvasChildPaintRecord.h"
#include "DestinationColorSpace.h"
#include "DisplayList.h"
#include "DisplayListItem.h"
#include "DisplayListItems.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "NativeImage.h"
#include "PixelFormat.h"
#include "RenderingMode.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(ElementImage);

DetachedElementImage::DetachedElementImage(RefPtr<NativeImage>&& rasterized, FloatSize boxSize,
    FloatSize inkOverflowSize, FloatPoint rasterizeOrigin, FloatSize sourceUnzoomedCSSSize,
    FloatPoint transformOrigin)
    : m_rasterized(WTF::move(rasterized))
    , m_boxSize(boxSize)
    , m_inkOverflowSize(inkOverflowSize)
    , m_rasterizeOrigin(rasterizeOrigin)
    , m_sourceUnzoomedCSSSize(sourceUnzoomedCSSSize)
    , m_transformOrigin(transformOrigin)
{
}

DetachedElementImage::DetachedElementImage(DetachedElementImage&&) = default;
DetachedElementImage& DetachedElementImage::operator=(DetachedElementImage&&) = default;
DetachedElementImage::~DetachedElementImage() = default;

Ref<ElementImage> ElementImage::create(Ref<CanvasChildPaintRecord>&& record)
{
    return adoptRef(*new ElementImage(WTF::move(record)));
}

RefPtr<ElementImage> ElementImage::createFromDetached(DetachedElementImage&& detached)
{
    auto rasterized = detached.takeRasterized();
    if (!rasterized)
        return nullptr;

    // Build a single-item display list that paints the rasterized image at its
    // local origin. The buffer dims equal inkOverflowSize; the box's origin
    // pixel within the buffer is at rasterizeOrigin (zero for the no-overflow
    // case Slice B exercises).
    auto inkOverflowSize = detached.inkOverflowSize();
    FloatRect bufferRect { 0, 0, inkOverflowSize.width(), inkOverflowSize.height() };
    Vector<DisplayList::Item> items;
    items.append(DisplayList::DrawNativeImage { *rasterized, bufferRect, bufferRect, ImagePaintingOptions { } });
    auto displayList = DisplayList::DisplayList::create(WTF::move(items));

    // The replay path translates by (dx - recordingOrigin) so the display
    // list's (0, 0) lands at (dx - rasterizeOrigin) — placing the box origin
    // pixel (which sits at rasterizeOrigin in the buffer) at exactly (dx, dy).
    CanvasChildPaintState state {
        detached.boxSize(),
        detached.boxSize(),
        1.0f,
        detached.transformOrigin(),
        detached.rasterizeOrigin(),
        detached.sourceUnzoomedCSSSize(),
    };

    auto record = CanvasChildPaintRecord::createFromRasterized(WTF::move(displayList), rasterized.releaseNonNull(), state);
    return ElementImage::create(WTF::move(record));
}

ElementImage::ElementImage(Ref<CanvasChildPaintRecord>&& record)
    : m_record(WTF::move(record))
{
}

ElementImage::~ElementImage() = default;

double ElementImage::width() const
{
    return m_record ? m_record->state().boxSize.width() : 0;
}

double ElementImage::height() const
{
    return m_record ? m_record->state().boxSize.height() : 0;
}

void ElementImage::close()
{
    m_record = nullptr;
}

std::optional<DetachedElementImage> ElementImage::detach()
{
    if (isClosed())
        return std::nullopt;

    auto& recordState = m_record->state();
    auto boxSize = recordState.boxSize;
    if (boxSize.isEmpty())
        return std::nullopt;

    // Slice B: rasterize at boxSize (inkOverflowSize == boxSize). Slice E will
    // expand to the recorded display list's ink-overflow bounding box so
    // shadows / outlines / blur survive the transfer.
    //
    // CPU-backed buffer (RenderingMode::Unaccelerated): the resulting NativeImage
    // must be safe to consume on a worker thread. Accelerated backends produce
    // NativeImages whose backing texture references a main-thread/GPU-process
    // resource and would race when the worker replays the display list.
    auto buffer = ImageBuffer::create(
        boxSize,
        RenderingMode::Unaccelerated,
        RenderingPurpose::Unspecified,
        1.0f,
        DestinationColorSpace::SRGB(),
        PixelFormat::BGRA8);
    if (!buffer)
        return std::nullopt;

    auto& bufferContext = buffer->context();
    // Match the live-replay translate: subtract recordingOrigin so the box's
    // origin (at recordingOrigin in the recorded coordinates) lands at the
    // buffer's (0, 0).
    bufferContext.translate(-recordState.recordingOrigin.x(), -recordState.recordingOrigin.y());
    bufferContext.drawDisplayList(m_record->displayList());

    auto rasterized = ImageBuffer::sinkIntoNativeImage(WTF::move(buffer));
    if (!rasterized)
        return std::nullopt;

    DetachedElementImage detached(
        WTF::move(rasterized),
        boxSize,
        boxSize,
        FloatPoint { 0, 0 },
        recordState.sourceUnzoomedCSSSize,
        recordState.transformOrigin);

    m_record = nullptr;
    return detached;
}

} // namespace WebCore
