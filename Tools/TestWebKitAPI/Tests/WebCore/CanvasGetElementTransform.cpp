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

#include <WebCore/AffineTransform.h>
#include <WebCore/DrawElementImageMath.h>
#include <WebCore/FloatPoint.h>
#include <WebCore/FloatSize.h>
#include <WebCore/TransformationMatrix.h>

namespace TestWebKitAPI {

using namespace WebCore;

// TB3b acceptance: drawElementImage's matrix-return path and getElementTransform's both
// feed an AffineTransform `T_draw` plus the same `(transformOrigin, cssToGridScale)`
// triple into computeDrawElementAlignmentMatrix. drawElementImage builds T_draw as
// `state().transform . T(dx,dy) . S(destScale)`. getElementTransform builds T_draw via
// `AffineTransform { m.a(), m.b(), m.c(), m.d(), m.e(), m.f() }` from the caller-supplied
// DOMMatrix (HTMLCanvasElement.cpp). These tests verify the two paths produce identical
// alignment matrices when their inputs are equivalent.
//
// DOMMatrix is not part of WebCore's exported header surface, so the projection step is
// modelled here with AffineTransform's 6-component constructor — the same constructor
// HTMLCanvasElement::getElementTransform uses to project a..f.

static void expectMatrixNear(const TransformationMatrix& actual, const TransformationMatrix& expected, double eps = 1e-6)
{
    EXPECT_NEAR(expected.a(), actual.a(), eps);
    EXPECT_NEAR(expected.b(), actual.b(), eps);
    EXPECT_NEAR(expected.c(), actual.c(), eps);
    EXPECT_NEAR(expected.d(), actual.d(), eps);
    EXPECT_NEAR(expected.e(), actual.e(), eps);
    EXPECT_NEAR(expected.f(), actual.f(), eps);
}

TEST(CanvasGetElementTransform, Equivalence_Identity)
{
    // drawElementImage path: T_draw = identity . T(0,0) . S(1,1) = identity.
    AffineTransform deiDraw;
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = deiDraw,
    };

    // getElementTransform path: identity DOMMatrix projects to AffineTransform { 1,0,0,1,0,0 }.
    AffineTransform getDraw { 1, 0, 0, 1, 0, 0 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs));
}

TEST(CanvasGetElementTransform, Equivalence_TranslationOnly)
{
    AffineTransform deiDraw;
    deiDraw.translate(17, -9);
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = deiDraw,
    };

    // DOMMatrix(translate(17,-9)).{a,b,c,d,e,f} = {1, 0, 0, 1, 17, -9}.
    AffineTransform getDraw { 1, 0, 0, 1, 17, -9 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs));
}

TEST(CanvasGetElementTransform, Equivalence_ScaledCTMWithTranslate)
{
    // drawElementImage: state().transform = scale(2,3) then translate(5,7).
    AffineTransform deiDraw;
    deiDraw.scaleNonUniform(2, 3);
    deiDraw.translate(5, 7);
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = deiDraw,
    };

    // Equivalent DOMMatrix.a..f: scale(2,3).translate(5,7) → a=2, b=0, c=0, d=3, e=10, f=21.
    AffineTransform getDraw { 2, 0, 0, 3, 10, 21 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs));
}

TEST(CanvasGetElementTransform, Equivalence_ExplicitDestScale)
{
    // drawElementImage 4-arg: T_draw = identity . T(0,0) . S(2, 0.5).
    AffineTransform deiDraw;
    deiDraw.scaleNonUniform(2, 0.5);
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = deiDraw,
    };

    // Equivalent DOMMatrix.a..f for scale(2, 0.5).
    AffineTransform getDraw { 2, 0, 0, 0.5, 0, 0 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs));
}

TEST(CanvasGetElementTransform, Equivalence_NonDefaultOriginRotation)
{
    // Mirrors the corpus subtest "Element transform should account for transform-origin".
    AffineTransform deiDraw;
    deiDraw.rotate(45);
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = deiDraw,
    };

    // Equivalent DOMMatrix.a..f for rotate(45deg).
    constexpr double kCos45 = 0.7071067811865476;
    constexpr double kSin45 = 0.7071067811865475;
    AffineTransform getDraw { kCos45, kSin45, -kSin45, kCos45, 0, 0 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs), 1e-3);
}

TEST(CanvasGetElementTransform, Equivalence_CompositeWithCssToGridScale)
{
    // Mirrors corpus subtest "should account for canvas pixel scale, transform-origin, and a
    // complex draw transform". Both paths must agree on the same final alignment matrix.
    AffineTransform deiDraw;
    deiDraw.translate(42, 17);
    deiDraw.rotate(45);
    DrawElementImageMathInputs deiInputs {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 2, 0.5 },
        .drawTransform = deiDraw,
    };

    // DOMMatrix.translate(42,17).rotate(45) → a=cos45, b=sin45, c=-sin45, d=cos45, e=42, f=17.
    constexpr double kCos45 = 0.7071067811865476;
    constexpr double kSin45 = 0.7071067811865475;
    AffineTransform getDraw { kCos45, kSin45, -kSin45, kCos45, 42, 17 };
    DrawElementImageMathInputs getInputs = deiInputs;
    getInputs.drawTransform = getDraw;

    expectMatrixNear(computeDrawElementAlignmentMatrix(deiInputs), computeDrawElementAlignmentMatrix(getInputs), 1e-3);
}

TEST(CanvasGetElementTransform, Projection_2DComponentsRoundTrip)
{
    // The 6-component AffineTransform constructor matches the DOMMatrix.a..f extraction in
    // HTMLCanvasElement::getElementTransform. Build an AffineTransform via translate+scale,
    // capture its a..f, reconstruct, and verify the alignment math output is identical.
    AffineTransform composed;
    composed.translate(11, -7);
    composed.scaleNonUniform(1.5, 0.25);
    AffineTransform reconstructed { composed.a(), composed.b(), composed.c(), composed.d(), composed.e(), composed.f() };

    DrawElementImageMathInputs viaComposed {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 2, 0.5 },
        .drawTransform = composed,
    };
    DrawElementImageMathInputs viaReconstructed = viaComposed;
    viaReconstructed.drawTransform = reconstructed;

    expectMatrixNear(computeDrawElementAlignmentMatrix(viaComposed), computeDrawElementAlignmentMatrix(viaReconstructed));
}

} // namespace TestWebKitAPI
