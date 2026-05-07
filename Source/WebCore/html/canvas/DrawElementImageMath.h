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

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "FloatSize.h"
#include "TransformationMatrix.h"

namespace WebCore {

// Inputs for the drawElementImage alignment-matrix computation.
//
// The returned matrix is the value the caller assigns to `el.style.transform`
// to align the element's DOM hit-test region with its drawn-on-canvas pixels.
//
// Formula: T_align = T_origin^-1 . S_cssToGrid^-1 . T_draw . S_cssToGrid . T_origin
struct DrawElementImageMathInputs {
    FloatPoint transformOrigin; // element transform-origin, unzoomed CSS px, element-local
    FloatSize cssToGridScale; // canvas.width / canvasUnzoomedCSSWidth (and same for height)
    AffineTransform drawTransform; // CTM . T(dx, dy) . S(destScaleX, destScaleY)
};

WEBCORE_EXPORT TransformationMatrix computeDrawElementAlignmentMatrix(const DrawElementImageMathInputs&);

} // namespace WebCore
