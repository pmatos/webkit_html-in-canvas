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
#include "DrawElementImageEligibility.h"

#include "Element.h"
#include "ElementInlines.h"
#include "HTMLCanvasElement.h"
#include "HTMLNames.h"

namespace WebCore {

static bool hasLayoutSubtreeCanvasAncestor(const HTMLCanvasElement& canvas)
{
    for (auto* ancestor = canvas.parentElement(); ancestor; ancestor = ancestor->parentElement()) {
        if (auto* canvasAncestor = dynamicDowncast<HTMLCanvasElement>(ancestor)) {
            if (canvasAncestor->hasAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr))
                return true;
        }
    }
    return false;
}

ExceptionOr<CanvasChildPaintRecord*> verifyDrawElementImageEligibility(HTMLCanvasElement& canvas, Element& element)
{
    // Order matches Blink's HTMLCanvasElement::VerifyDrawElementImageEligibility
    // (Chrome `core/html/canvas/html_canvas_element.cc:979`).

    // Rule 1 (issue #9): the canvas itself must not be inside another <canvas layoutsubtree>.
    // WPT pin: nested-layoutsubtree-canvas.html:17-18 — assert_throws_dom("NotSupportedError", ...).
    if (hasLayoutSubtreeCanvasAncestor(canvas))
        return Exception { ExceptionCode::NotSupportedError, "Nested canvases are not supported."_s };

    // Rule 2 (issue #9): the element must be a direct child of the canvas.
    // Throws TypeError, not InvalidStateError — pinned by WPT
    // wpt_internal/.../drawElementImage/error-conditions.html:35-37
    // (`assert_throws_js(TypeError, …, "Can't draw non-direct children.")`).
    if (element.parentNode() != &canvas)
        return Exception { ExceptionCode::TypeError, "Only immediate children of the <canvas> element can be passed to drawElementImage."_s };

    // Rule 3 (issue #9): the canvas must have the layoutsubtree attribute.
    // Pinned by error-conditions.html:40-42, draw-element-image-detached.html:27-30.
    if (!canvas.hasAttributeWithoutSynchronization(HTMLNames::layoutsubtreeAttr))
        return Exception { ExceptionCode::InvalidStateError, "drawElementImage requires the canvas to have the layoutsubtree attribute."_s };

    // Rule 4 (issue #9): a snapshot must exist for this element. The snapshot map is
    // populated only by the recording walk, so this naturally subsumes display:none on
    // either the child (skipped) or the canvas (no walk runs) and the "no paint cycle
    // yet" case (draw-element-image-empty.html:19-22, draw-element-image-detached.html:27-30,
    // draw-element-image-display-none.html:38-45).
    auto record = canvas.canvasChildPaintRecord(element.nodeIdentifier());
    if (!record)
        return Exception { ExceptionCode::InvalidStateError, "No snapshot recorded for element."_s };

    return record.get();
}

} // namespace WebCore
