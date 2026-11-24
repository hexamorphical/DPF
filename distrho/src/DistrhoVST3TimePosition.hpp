/*
 * VST3 BBT conversion for DPF.
 * Copyright (C) 2026
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_VST3_TIME_POSITION_HPP_INCLUDED
#define DISTRHO_VST3_TIME_POSITION_HPP_INCLUDED

#include "../DistrhoDetails.hpp"
#include "travesty/audio_processor.h"
#include <cmath>

START_NAMESPACE_DISTRHO

// Kept independent of the processor so the exact conversion can be tested.
static inline void fillVST3BarBeatTick(const v3_process_context& ctx,
                                     TimePosition::BarBeatTick& bbt) noexcept
{
    bbt.valid = false;
    bbt.bar = bbt.beat = 1;
    bbt.tick = bbt.barStartTick = 0.0;
    bbt.beatsPerBar = bbt.beatType = 4.0f;
    bbt.ticksPerBeat = 1920.0;

    const uint32_t required = V3_PROCESS_CTX_PROJECT_TIME_VALID | V3_PROCESS_CTX_TIME_SIG_VALID;
    if ((ctx.state & required) != required || ctx.time_sig_numerator <= 0 || ctx.time_sig_denom <= 0
        || !std::isfinite(ctx.project_time_quarters))
        return;

    const double numerator = ctx.time_sig_numerator;
    const double beatScale = ctx.time_sig_denom / 4.0;
    const double quartersPerBar = numerator / beatScale; // 19/16 is 4.75, 1/16 is 0.25.
    const double ppq = ctx.project_time_quarters;
    double barStart = std::floor(ppq / quartersPerBar) * quartersPerBar;

    // A meter change need not lie on the grid obtained by extrapolating the new
    // meter back to PPQ zero. Prefer the host's actual last bar start.
    if ((ctx.state & V3_PROCESS_CTX_BAR_POSITION_VALID) != 0
        && std::isfinite(ctx.bar_position_quarters))
    {
        const double relative = ppq - ctx.bar_position_quarters;
        if (relative >= -1.0e-9 && relative <= quartersPerBar + 1.0e-9)
            barStart = ctx.bar_position_quarters;
    }

    double barBeats = (ppq - barStart) * beatScale;
    if (barBeats < 0.0) // only possible for a host's sub-nanosecond rounding error
        barBeats = 0.0;
    if (barBeats >= numerator)
    {
        barStart += quartersPerBar;
        barBeats = 0.0;
    }
    const double beat = std::floor(barBeats);
    bbt.valid = true;
    // VST3 has no absolute bar-number field. This is exact for constant meter;
    // bar-relative beat/tick and barStartTick remain exact across meter changes.
    bbt.bar = static_cast<int32_t>(std::floor(barStart / quartersPerBar + 1.0e-9)) + 1;
    bbt.beat = static_cast<int32_t>(beat) + 1;
    bbt.tick = (barBeats - beat) * bbt.ticksPerBeat;
    bbt.barStartTick = barStart * beatScale * bbt.ticksPerBeat;
    bbt.beatsPerBar = static_cast<float>(numerator);
    bbt.beatType = static_cast<float>(ctx.time_sig_denom);
}

END_NAMESPACE_DISTRHO

#endif
