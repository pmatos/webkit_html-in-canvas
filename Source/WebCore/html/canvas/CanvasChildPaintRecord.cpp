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
#include "CanvasChildPaintRecord.h"

#include "DisplayList.h"
#include "NativeImage.h"
#include <wtf/StdLibExtras.h>
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(CanvasChildPaintRecord);

CanvasChildPaintRecord::CanvasChildPaintRecord(Ref<const DisplayList::DisplayList>&& displayList, RefPtr<NativeImage>&& rasterizedImage, CanvasChildPaintState state)
    : m_displayList(WTF::move(displayList))
    , m_rasterizedImage(WTF::move(rasterizedImage))
    , m_state(WTF::move(state))
{
}

Ref<CanvasChildPaintRecord> CanvasChildPaintRecord::createFromRasterized(Ref<const DisplayList::DisplayList>&& displayList, Ref<NativeImage>&& rasterized, CanvasChildPaintState state)
{
    return adoptRef(*new CanvasChildPaintRecord(WTF::move(displayList), WTF::move(rasterized), state));
}

CanvasChildPaintRecord::~CanvasChildPaintRecord() = default;

} // namespace WebCore
