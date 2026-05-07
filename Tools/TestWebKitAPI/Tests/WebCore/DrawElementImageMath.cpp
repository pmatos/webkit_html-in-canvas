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

static void expectMatrixNear(const TransformationMatrix& actual, const TransformationMatrix& expected, double eps = 1e-6)
{
    EXPECT_NEAR(expected.a(), actual.a(), eps);
    EXPECT_NEAR(expected.b(), actual.b(), eps);
    EXPECT_NEAR(expected.c(), actual.c(), eps);
    EXPECT_NEAR(expected.d(), actual.d(), eps);
    EXPECT_NEAR(expected.e(), actual.e(), eps);
    EXPECT_NEAR(expected.f(), actual.f(), eps);
}

TEST(DrawElementImageMath, Identity)
{
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = AffineTransform { },
    };
    auto result = computeDrawElementAlignmentMatrix(inputs);
    expectMatrixNear(result, TransformationMatrix { });
}

TEST(DrawElementImageMath, Translation)
{
    AffineTransform drawTransform;
    drawTransform.translate(17, -9);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(17, -9);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, DestinationScale_TopLeftOrigin)
{
    AffineTransform drawTransform;
    drawTransform.scaleNonUniform(2, 0.5);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.scaleNonUniform(2, 0.5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, CTM_Rotation)
{
    AffineTransform drawTransform;
    drawTransform.rotate(45);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.rotate(45);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected, 1e-3);
}

TEST(DrawElementImageMath, TransformOrigin_5_5_Rotation)
{
    AffineTransform drawTransform;
    drawTransform.rotate(45);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(-5, -5);
    expected.rotate(45);
    expected.translate(5, 5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected, 1e-3);
}

TEST(DrawElementImageMath, CssToGridScale_PassThrough_Identity)
{
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 2, 0.5 },
        .drawTransform = AffineTransform { },
    };
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), TransformationMatrix { });
}

TEST(DrawElementImageMath, CssToGridScale_With_TDraw)
{
    AffineTransform drawTransform;
    drawTransform.translate(10, 10);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 2, 0.5 },
        .drawTransform = drawTransform,
    };
    // T_origin=I; expected = S(0.5, 2) . T(10, 10) . S(2, 0.5) = T(20, 5).
    TransformationMatrix expected;
    expected.scaleNonUniform(0.5, 2);
    expected.translate(10, 10);
    expected.scaleNonUniform(2, 0.5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, Composite_TransformOrigin_Scale_CTM_Translation)
{
    // Mirrors corpus test 10: transform-origin (5,5), cssToGrid=(2,0.5),
    // drawTransform = T(42,17) . R(45) . T(13,-9).
    AffineTransform drawTransform;
    drawTransform.translate(42, 17);
    drawTransform.rotate(45);
    drawTransform.translate(13, -9);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 5, 5 },
        .cssToGridScale = { 2, 0.5 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(-5, -5);
    expected.scaleNonUniform(0.5, 2);
    expected.translate(42, 17);
    expected.rotate(45);
    expected.translate(13, -9);
    expected.scaleNonUniform(2, 0.5);
    expected.translate(5, 5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected, 1e-3);
}

// Issue 8 acceptance: the math API stays stable across all four drawElementImage
// overload shapes (3-/5-/7-/9-arg). Source-rect crop is a clip operation in the
// 7-/9-arg paths — it does not enter T_draw — so 3-arg ↔ 7-arg and 5-arg ↔ 9-arg
// produce identical drawTransforms.

TEST(DrawElementImageMath, Overload_3arg_NoScale)
{
    // drawElementImage(el, dx, dy) → T_draw = CTM . T(dx, dy). Here CTM=I.
    AffineTransform drawTransform;
    drawTransform.translate(11, 22);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(11, 22);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, Overload_5arg_DestScale)
{
    // drawElementImage(el, dx, dy, dwidth, dheight) → T_draw = CTM . T(dx, dy) . S(dw/bw, dh/bh).
    AffineTransform drawTransform;
    drawTransform.translate(11, 22);
    drawTransform.scaleNonUniform(2, 0.5);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(11, 22);
    expected.scaleNonUniform(2, 0.5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, Overload_7arg_SrcCrop)
{
    // drawElementImage(el, sx, sy, sw, sh, dx, dy): src rect is a clip; T_draw matches 3-arg.
    AffineTransform drawTransform;
    drawTransform.translate(11, 22);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(11, 22);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

TEST(DrawElementImageMath, Overload_9arg_SrcCropAndDestScale)
{
    // drawElementImage(el, sx, sy, sw, sh, dx, dy, dw, dh): T_draw matches 5-arg.
    AffineTransform drawTransform;
    drawTransform.translate(11, 22);
    drawTransform.scaleNonUniform(2, 0.5);
    DrawElementImageMathInputs inputs {
        .transformOrigin = { 0, 0 },
        .cssToGridScale = { 1, 1 },
        .drawTransform = drawTransform,
    };
    TransformationMatrix expected;
    expected.translate(11, 22);
    expected.scaleNonUniform(2, 0.5);
    expectMatrixNear(computeDrawElementAlignmentMatrix(inputs), expected);
}

} // namespace TestWebKitAPI
