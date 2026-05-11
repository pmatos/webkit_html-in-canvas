/*
 * Copyright (C) 2026 Igalia S.L. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

namespace WebCore {

// TB5b: assertion-only RAII guard that flags re-entrant style invalidation
// triggered by author JS running inside a `paint` event handler. Modeled on
// LayoutDisallowedScope (rendering/LayoutDisallowedScope.h). The handler is
// allowed to run script (so ScriptDisallowedScope is wrong) and may force
// layout for hit-testing (so this scope deliberately does NOT cover layout —
// see Page::dispatchCanvasPaintEvents). The single ASSERT placement is in
// Document::updateStyleIfNeeded, which is the entrypoint reached by author
// JS performing `getComputedStyle()` / forced style recalc.
class InvalidationDisallowedScope {
public:
#if !ASSERT_ENABLED
    InvalidationDisallowedScope() = default;
    static bool isAllowed() { return true; }
#else
    InvalidationDisallowedScope()
        : m_previous(s_current)
    {
        s_current = this;
    }

    ~InvalidationDisallowedScope()
    {
        s_current = m_previous;
    }

    static bool isAllowed() { return !s_current; }

private:
    InvalidationDisallowedScope* m_previous;
    WEBCORE_EXPORT static InvalidationDisallowedScope* s_current;
#endif
};

} // namespace WebCore
