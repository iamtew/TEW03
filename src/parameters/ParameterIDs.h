#pragma once

#include <cstring>

namespace ParamID
{
    inline constexpr const char* cutoff      = "cutoff";
    inline constexpr const char* resonance   = "resonance";
    inline constexpr const char* decay       = "decay";
    inline constexpr const char* accent      = "accent";
    inline constexpr const char* envMod      = "envMod";
    inline constexpr const char* drive       = "drive";
    inline constexpr const char* volume      = "volume";
    inline constexpr const char* waveform    = "waveform";
    inline constexpr const char* glide       = "glide";
    inline constexpr const char* seqPlay     = "seqPlay";
    inline constexpr const char* playMode    = "playMode";
    inline constexpr const char* seqTempo    = "seqTempo";
    inline constexpr const char* seq2x       = "seq2x";
    inline constexpr const char* seqBank     = "seqBank";
    inline constexpr const char* seqPattern  = "seqPattern";

    inline constexpr const char* lfo1Rate    = "lfo1Rate";
    inline constexpr const char* lfo1Sync    = "lfo1Sync";
    inline constexpr const char* lfo1Div     = "lfo1Div";
    inline constexpr const char* lfo1Mode    = "lfo1Mode";
    inline constexpr const char* lfo1Smooth  = "lfo1Smooth";
    inline constexpr const char* lfo2Rate    = "lfo2Rate";
    inline constexpr const char* lfo2Sync    = "lfo2Sync";
    inline constexpr const char* lfo2Div     = "lfo2Div";
    inline constexpr const char* lfo2Mode    = "lfo2Mode";
    inline constexpr const char* lfo2Smooth  = "lfo2Smooth";

    inline constexpr const char* cutoffLfo      = "cutoffLfo";
    inline constexpr const char* cutoffLfoAmt   = "cutoffLfoAmt";
    inline constexpr const char* resonanceLfo   = "resonanceLfo";
    inline constexpr const char* resonanceLfoAmt = "resonanceLfoAmt";
    inline constexpr const char* envModLfo      = "envModLfo";
    inline constexpr const char* envModLfoAmt   = "envModLfoAmt";
    inline constexpr const char* decayLfo       = "decayLfo";
    inline constexpr const char* decayLfoAmt    = "decayLfoAmt";
    inline constexpr const char* accentLfo      = "accentLfo";
    inline constexpr const char* accentLfoAmt   = "accentLfoAmt";
    inline constexpr const char* driveLfo       = "driveLfo";
    inline constexpr const char* driveLfoAmt    = "driveLfoAmt";
    inline constexpr const char* glideLfo       = "glideLfo";
    inline constexpr const char* glideLfoAmt    = "glideLfoAmt";
    inline constexpr const char* volumeLfo      = "volumeLfo";
    inline constexpr const char* volumeLfoAmt   = "volumeLfoAmt";

    inline constexpr const char* destIds[] = {
        cutoff, resonance, envMod, decay, accent, drive, glide, volume
    };
    inline constexpr const char* destLfoIds[] = {
        cutoffLfo, resonanceLfo, envModLfo, decayLfo, accentLfo, driveLfo, glideLfo, volumeLfo
    };
    inline constexpr const char* destAmtIds[] = {
        cutoffLfoAmt, resonanceLfoAmt, envModLfoAmt, decayLfoAmt,
        accentLfoAmt, driveLfoAmt, glideLfoAmt, volumeLfoAmt
    };
    inline constexpr const char* lfoRateIds[]   = { lfo1Rate, lfo2Rate };
    inline constexpr const char* lfoSyncIds[]   = { lfo1Sync, lfo2Sync };
    inline constexpr const char* lfoDivIds[]    = { lfo1Div, lfo2Div };
    inline constexpr const char* lfoModeIds[]   = { lfo1Mode, lfo2Mode };
    inline constexpr const char* lfoSmoothIds[] = { lfo1Smooth, lfo2Smooth };

    inline int destIndexForId (const char* id)
    {
        for (int i = 0; i < 8; ++i)
            if (std::strcmp (destIds[i], id) == 0)
                return i;
        return -1;
    }
}
