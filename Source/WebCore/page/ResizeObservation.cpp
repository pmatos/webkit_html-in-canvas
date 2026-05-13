/*
 * Copyright (C) 2019 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ResizeObservation.h"

#include "ElementInlines.h"
#include "FloatSize.h"
#include "HTMLFrameOwnerElement.h"
#include "LayoutPoint.h"
#include "Logging.h"
#include "RenderBoxInlines.h"
#include "RenderElementStyleInlines.h"
#include "RenderObjectEnums.h"
#include "SVGElement.h"
#include "StyleZoomPrimitivesInlines.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

Ref<ResizeObservation> ResizeObservation::create(Element& target, ResizeObserverBoxOptions observedBox)
{
    return adoptRef(*new ResizeObservation(target, observedBox));
}

WTF_MAKE_TZONE_ALLOCATED_IMPL(ResizeObservation);

ResizeObservation::ResizeObservation(Element& element, ResizeObserverBoxOptions observedBox)
    : m_target { element }
    , m_lastObservationSizes { LayoutSize(-1, -1), LayoutSize(-1, -1), LayoutSize(-1, -1), LayoutSize(-1, -1) }
    , m_observedBox { observedBox }
{
}

ResizeObservation::~ResizeObservation() = default;

void ResizeObservation::updateObservationSize(const BoxSizes& boxSizes)
{
    m_lastObservationSizes = boxSizes;
}

void ResizeObservation::resetObservationSize()
{
    m_lastObservationSizes = { LayoutSize(-1, -1), LayoutSize(-1, -1), LayoutSize(-1, -1), LayoutSize(-1, -1) };
}

auto ResizeObservation::computeObservedSizes() const -> std::optional<BoxSizes>
{
    if (RefPtr svg = dynamicDowncast<SVGElement>(target())) {
        if (svg->hasAssociatedSVGLayoutBox()) {
            LayoutSize size;
            if (auto svgRect = svg->getBoundingBox()) {
                size.setWidth(svgRect->width());
                size.setHeight(svgRect->height());
            }
            LayoutSize devicePixelSize = size;
            if (CheckedPtr renderer = svg->renderer())
                devicePixelSize.scale(renderer->style().usedZoom() * svg->document().deviceScaleFactor());
            return { { size, size, size, devicePixelSize } };
        }
    }

    CheckedPtr box = m_target->renderBox();
    if (box) {
        if (box->isSkippedContent())
            return std::nullopt;
        auto contentBoxSizeRender = box->contentBoxSize();
        auto logicalContent = Style::adjustLayoutSizeForAbsoluteZoom(box->contentBoxLogicalSize(), *box);
        auto logicalBorder = Style::adjustLayoutSizeForAbsoluteZoom(box->borderBoxLogicalSize(), *box);

        // https://w3c.github.io/csswg-drafts/resize-observer/#calculate-depx-content-size
        // Snap the render-pixel content box in absolute (transformed) coordinates to device pixels,
        // then rotate to logical axes for vertical writing modes. contentBoxSize() is already
        // zoom-applied, so do not divide it by usedZoom here.
        auto absoluteContentBoxOrigin = box->localToAbsolute(FloatPoint(box->contentBoxRect().location()), MapCoordinatesMode::UseTransforms);
        float deviceScaleFactor = m_target->document().deviceScaleFactor();
        auto physicalSnapped = snapSizeToDevicePixel(contentBoxSizeRender, LayoutPoint(absoluteContentBoxOrigin), deviceScaleFactor);
        auto devicePixelLogical = box->isHorizontalWritingMode()
            ? LayoutSize(physicalSnapped)
            : LayoutSize(physicalSnapped.transposedSize());

        auto physicalContentCSS = Style::adjustLayoutSizeForAbsoluteZoom(contentBoxSizeRender, *box);
        return { { physicalContentCSS, logicalContent, logicalBorder, devicePixelLogical } };
    }

    return BoxSizes { };
}

LayoutPoint ResizeObservation::computeTargetLocation() const
{
    if (!m_target)
        return { };

    if (!m_target->isSVGElement()) {
        if (CheckedPtr box = m_target->renderBox())
            return LayoutPoint(box->paddingLeft(), box->paddingTop());
    }

    return { };
}

FloatRect ResizeObservation::computeContentRect() const
{
    return FloatRect(FloatPoint(computeTargetLocation()), FloatSize(m_lastObservationSizes.contentBoxSize));
}

FloatSize ResizeObservation::borderBoxSize() const
{
    return m_lastObservationSizes.borderBoxLogicalSize;
}

FloatSize ResizeObservation::contentBoxSize() const
{
    return m_lastObservationSizes.contentBoxLogicalSize;
}

FloatSize ResizeObservation::devicePixelContentBoxSize() const
{
    return m_lastObservationSizes.devicePixelContentBoxLogicalSize;
}

std::optional<ResizeObservation::BoxSizes> ResizeObservation::elementSizeChanged() const
{
    auto currentSizes = computeObservedSizes();
    if (!currentSizes)
        return std::nullopt;

    switch (m_observedBox) {
    case ResizeObserverBoxOptions::BorderBox:
        if (m_lastObservationSizes.borderBoxLogicalSize != currentSizes->borderBoxLogicalSize) {
            LOG_WITH_STREAM(ResizeObserver, stream << "ResizeObservation " << *this << " elementSizeChanged - border box size changed from " << m_lastObservationSizes.borderBoxLogicalSize << " to " << currentSizes->borderBoxLogicalSize);
            return currentSizes;
        }
        break;
    case ResizeObserverBoxOptions::ContentBox:
        if (m_lastObservationSizes.contentBoxLogicalSize != currentSizes->contentBoxLogicalSize) {
            LOG_WITH_STREAM(ResizeObserver, stream << "ResizeObservation " << *this << " elementSizeChanged - content box size changed from " << m_lastObservationSizes.contentBoxLogicalSize << " to " << currentSizes->contentBoxLogicalSize);
            return currentSizes;
        }
        break;
    case ResizeObserverBoxOptions::DevicePixelContentBox:
        if (m_lastObservationSizes.devicePixelContentBoxLogicalSize != currentSizes->devicePixelContentBoxLogicalSize) {
            LOG_WITH_STREAM(ResizeObserver, stream << "ResizeObservation " << *this << " elementSizeChanged - device pixel content box size changed from " << m_lastObservationSizes.devicePixelContentBoxLogicalSize << " to " << currentSizes->devicePixelContentBoxLogicalSize);
            return currentSizes;
        }
        break;
    }

    return { };
}

// https://drafts.csswg.org/resize-observer/#calculate-depth-for-node
size_t ResizeObservation::targetElementDepth() const
{
    unsigned depth = 0;
    for (auto* ownerElement = m_target.get(); ownerElement; ownerElement = ownerElement->document().ownerElement()) {
        for (auto* parent = ownerElement; parent; parent = parent->parentElementInComposedTree())
            ++depth;
    }

    return depth;
}

TextStream& operator<<(TextStream& ts, const ResizeObservation& observation)
{
    ts.dumpProperty("target"_s, ValueOrNull(observation.target()));

    if (CheckedPtr box = observation.target()->renderBox())
        ts.dumpProperty("target box"_s, box);

    ts.dumpProperty("border box"_s, observation.borderBoxSize());
    ts.dumpProperty("content box"_s, observation.contentBoxSize());
    ts.dumpProperty("device pixel content box"_s, observation.devicePixelContentBoxSize());
    return ts;
}

} // namespace WebCore
