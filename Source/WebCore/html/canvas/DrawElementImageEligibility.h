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

#include "ExceptionOr.h"

namespace WebCore {

class CanvasChildPaintRecord;
class Element;
class HTMLCanvasElement;

// Mirrors Blink's HTMLCanvasElement::VerifyDrawElementImageEligibility (Chrome
// `core/html/canvas/html_canvas_element.cc:979`). Enforces the live-Element
// preconditions for drawElementImage / getElementTransform / captureElementImage:
//   1. canvas is not nested inside another <canvas layoutsubtree>     → NotSupportedError
//   2. element is a direct child of canvas                            → TypeError
//   3. canvas has the layoutsubtree attribute set                     → InvalidStateError
//   4. a snapshot for the element exists (paint cycle has run)        → InvalidStateError
// On success returns the snapshot record so the caller can replay it without
// a second map lookup.
WEBCORE_EXPORT ExceptionOr<CanvasChildPaintRecord*> verifyDrawElementImageEligibility(HTMLCanvasElement&, Element&);

} // namespace WebCore
