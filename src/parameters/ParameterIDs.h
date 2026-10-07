#pragma once

#include <cstdint>
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
    inline constexpr const char* seqKeyLock  = "seqKeyLock";
    inline constexpr const char* seqKey      = "seqKey";
    inline constexpr const char* seqScale    = "seqScale";

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

    inline constexpr int fxCount = 9;

    inline constexpr const char* fxNames[fxCount] = {
        "Chorus", "Compressor", "Delay", "Distortion", "Equalizer",
        "Filter", "Flanger", "Phaser", "Reverb"
    };

    inline constexpr const char* fxOnIds[fxCount] = {
        "fxChoOn", "fxCmpOn", "fxDlyOn", "fxDstOn", "fxEqOn",
        "fxFltOn", "fxFlaOn", "fxPhaOn", "fxRevOn"
    };

    inline constexpr const char* fxChoRate = "fxChoRate";
    inline constexpr const char* fxChoDepth = "fxChoDepth";
    inline constexpr const char* fxChoMix = "fxChoMix";
    inline constexpr const char* fxChoFb = "fxChoFb";

    inline constexpr const char* fxCmpThr = "fxCmpThr";
    inline constexpr const char* fxCmpRat = "fxCmpRat";
    inline constexpr const char* fxCmpAtk = "fxCmpAtk";
    inline constexpr const char* fxCmpRel = "fxCmpRel";
    inline constexpr const char* fxCmpMix = "fxCmpMix";

    inline constexpr const char* fxDlyTime = "fxDlyTime";
    inline constexpr const char* fxDlyFb = "fxDlyFb";
    inline constexpr const char* fxDlyMix = "fxDlyMix";
    inline constexpr const char* fxDlyCut = "fxDlyCut";
    inline constexpr const char* fxDlySync = "fxDlySync";
    inline constexpr const char* fxDlyDiv = "fxDlyDiv";

    inline constexpr const char* fxDstDrive = "fxDstDrive";
    inline constexpr const char* fxDstMix = "fxDstMix";

    inline constexpr const char* fxFltCut = "fxFltCut";
    inline constexpr const char* fxFltRes = "fxFltRes";
    inline constexpr const char* fxFltMix = "fxFltMix";
    inline constexpr const char* fxFltType = "fxFltType";

    inline constexpr const char* fxFlaRate = "fxFlaRate";
    inline constexpr const char* fxFlaDepth = "fxFlaDepth";
    inline constexpr const char* fxFlaFb = "fxFlaFb";
    inline constexpr const char* fxFlaMix = "fxFlaMix";

    inline constexpr const char* fxPhaRate = "fxPhaRate";
    inline constexpr const char* fxPhaDepth = "fxPhaDepth";
    inline constexpr const char* fxPhaFb = "fxPhaFb";
    inline constexpr const char* fxPhaMix = "fxPhaMix";
    inline constexpr const char* fxPhaCentre = "fxPhaCentre";

    inline constexpr const char* fxRevSize = "fxRevSize";
    inline constexpr const char* fxRevDamp = "fxRevDamp";
    inline constexpr const char* fxRevMix = "fxRevMix";
    inline constexpr const char* fxRevWidth = "fxRevWidth";

    inline constexpr int eqBandCount = 5;
    inline constexpr const char* fxEqType[eqBandCount] = {
        "fxEq1Type", "fxEq2Type", "fxEq3Type", "fxEq4Type", "fxEq5Type"
    };
    inline constexpr const char* fxEqFreq[eqBandCount] = {
        "fxEq1Freq", "fxEq2Freq", "fxEq3Freq", "fxEq4Freq", "fxEq5Freq"
    };
    inline constexpr const char* fxEqGain[eqBandCount] = {
        "fxEq1Gain", "fxEq2Gain", "fxEq3Gain", "fxEq4Gain", "fxEq5Gain"
    };
    inline constexpr const char* fxEqQ[eqBandCount] = {
        "fxEq1Q", "fxEq2Q", "fxEq3Q", "fxEq4Q", "fxEq5Q"
    };
    inline constexpr const char* preEqOn = "preEqOn";
    inline constexpr const char* postEqOn = "postEqOn";
    inline constexpr const char* preEqType[eqBandCount] = {
        "preEq1Type", "preEq2Type", "preEq3Type", "preEq4Type", "preEq5Type"
    };
    inline constexpr const char* preEqFreq[eqBandCount] = {
        "preEq1Freq", "preEq2Freq", "preEq3Freq", "preEq4Freq", "preEq5Freq"
    };
    inline constexpr const char* preEqGain[eqBandCount] = {
        "preEq1Gain", "preEq2Gain", "preEq3Gain", "preEq4Gain", "preEq5Gain"
    };
    inline constexpr const char* preEqQ[eqBandCount] = {
        "preEq1Q", "preEq2Q", "preEq3Q", "preEq4Q", "preEq5Q"
    };
    inline constexpr const char* postEqType[eqBandCount] = {
        "postEq1Type", "postEq2Type", "postEq3Type", "postEq4Type", "postEq5Type"
    };
    inline constexpr const char* postEqFreq[eqBandCount] = {
        "postEq1Freq", "postEq2Freq", "postEq3Freq", "postEq4Freq", "postEq5Freq"
    };
    inline constexpr const char* postEqGain[eqBandCount] = {
        "postEq1Gain", "postEq2Gain", "postEq3Gain", "postEq4Gain", "postEq5Gain"
    };
    inline constexpr const char* postEqQ[eqBandCount] = {
        "postEq1Q", "postEq2Q", "postEq3Q", "postEq4Q", "postEq5Q"
    };
}

namespace tew
{
inline constexpr int fxCount = ParamID::fxCount;

inline int fillDefaultFxOrder (int* types)
{
    if (types == nullptr)
        return 0;
    for (int i = 0; i < fxCount; ++i)
        types[i] = i;
    return fxCount;
}

inline int completeFxOrder (int* types, int n)
{
    if (types == nullptr)
        return 0;
    bool seen[fxCount] {};
    int tmp[fxCount];
    int m = 0;
    n = n < 0 ? 0 : (n > fxCount ? fxCount : n);
    for (int i = 0; i < n; ++i)
    {
        const int t = types[i];
        if (t < 0 || t >= fxCount || seen[t])
            continue;
        seen[t] = true;
        tmp[m++] = t;
    }
    for (int t = 0; t < fxCount; ++t)
        if (! seen[t])
            tmp[m++] = t;
    for (int i = 0; i < m; ++i)
        types[i] = tmp[i];
    return m;
}

inline std::uint64_t packFxOrder (const int* types, int n)
{
    std::uint64_t packed = 0;
    if (types == nullptr)
        return 0;
    n = n < 0 ? 0 : (n > fxCount ? fxCount : n);
    for (int i = 0; i < n; ++i)
    {
        const int t = types[i];
        if (t < 0 || t >= fxCount)
            continue;
        packed |= (std::uint64_t) ((t & 15) + 1) << (i * 4);
    }
    return packed;
}

inline int unpackFxOrder (std::uint64_t packed, int* types)
{
    if (types == nullptr)
        return 0;
    int n = 0;
    bool seen[fxCount] {};
    for (int i = 0; i < fxCount; ++i)
    {
        const int v = (int) ((packed >> (i * 4)) & 15);
        if (v == 0)
            break;
        const int t = v - 1;
        if (t < 0 || t >= fxCount || seen[t])
            continue;
        seen[t] = true;
        types[n++] = t;
    }
    return n;
}

inline int parseFxOrder (const char* s, int* types)
{
    int n = 0;
    if (s == nullptr || types == nullptr)
        return 0;
    bool seen[fxCount] {};
    while (*s != 0 && n < fxCount)
    {
        if (*s < '0' || *s > '8')
        {
            ++s;
            continue;
        }
        const int t = *s - '0';
        ++s;
        if (seen[t])
            continue;
        seen[t] = true;
        types[n++] = t;
    }
    return n;
}

inline void formatFxOrder (std::uint64_t packed, char* buf, int bufSize)
{
    if (buf == nullptr || bufSize <= 0)
        return;
    int types[fxCount];
    const int n = unpackFxOrder (packed, types);
    int w = 0;
    for (int i = 0; i < n && w + 2 < bufSize; ++i)
    {
        if (i > 0)
            buf[w++] = ',';
        buf[w++] = (char) ('0' + types[i]);
    }
    buf[w] = 0;
}
}
