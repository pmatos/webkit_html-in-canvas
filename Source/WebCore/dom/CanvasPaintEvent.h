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

#include <WebCore/Element.h>
#include <WebCore/Event.h>
#include <wtf/Ref.h>
#include <wtf/Vector.h>

namespace WebCore {

class CanvasPaintEvent : public Event {
    WTF_MAKE_TZONE_ALLOCATED(CanvasPaintEvent);
public:
    static Ref<CanvasPaintEvent> create(const AtomString& type, Vector<Ref<Element>>&& changedElements = { })
    {
        return adoptRef(*new CanvasPaintEvent(EventInterfaceType::CanvasPaintEvent, type, WTF::move(changedElements)));
    }

    struct Init : EventInit {
        Vector<Ref<Element>> changedElements;
    };

    static Ref<CanvasPaintEvent> create(const AtomString& type, const Init& initializer, IsTrusted isTrusted = IsTrusted::No)
    {
        return adoptRef(*new CanvasPaintEvent(EventInterfaceType::CanvasPaintEvent, type, initializer, isTrusted));
    }

    const Vector<Ref<Element>>& changedElements() const { return m_changedElements; }

protected:
    CanvasPaintEvent(enum EventInterfaceType, const AtomString& type, Vector<Ref<Element>>&&);
    CanvasPaintEvent(enum EventInterfaceType, const AtomString& type, const Init&, IsTrusted);

private:
    Vector<Ref<Element>> m_changedElements;
};

} // namespace WebCore
