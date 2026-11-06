#pragma once

#include "Library.h"

namespace tew
{

struct FactoryPatch
{
    const char* name;
    const char* category;
    unsigned fxMask;
    const char* fxOrder;
    float cutoff;
    float resonance;
    float envMod;
    float decay;
    float accent;
    float drive;
    float nasty;
    float glide;
    float waveform;
    float gate;
    float swing;
};

struct FactoryFxParam
{
    int patch;
    const char* id;
    float value;
};

inline constexpr unsigned fxBit (int t) { return 1u << t; }

inline constexpr FactoryPatch factoryPatches[] = {
    { "01 Squelch",     "Acid",    fxBit (2),           "2",     280.f,  0.82f, 0.88f, 0.16f, 0.75f, 0.12f, 0.f,   0.f,   0.f, 1.f, 0.f },
    { "02 Scream",      "Dirty",   fxBit (3) | fxBit (1), "3,1",  160.f,  0.96f, 0.98f, 0.10f, 0.90f, 0.40f, 0.25f, 0.f,   0.f, 1.f, 0.f },
    { "03 Rubber",      "Acid",    fxBit (0),           "0",     420.f,  0.72f, 0.55f, 0.55f, 0.45f, 0.08f, 0.f,   0.04f, 1.f, 1.f, 0.f },
    { "04 Hollow",      "Space",   fxBit (8),           "8",     140.f,  0.40f, 0.45f, 0.90f, 0.25f, 0.f,   0.f,   0.10f, 1.f, 1.f, 0.f },
    { "05 Bite",        "Acid",    fxBit (5),           "5",    1100.f,  0.50f, 0.92f, 0.07f, 0.85f, 0.30f, 0.10f, 0.f,   0.f, 1.f, 0.f },
    { "06 Muffled",     "Bass",    fxBit (4) | fxBit (5), "5,4",   90.f,  0.12f, 0.18f, 0.28f, 0.15f, 0.f,   0.f,   0.f,   1.f, 1.f, 0.f },
    { "07 Laser",       "Lead",    fxBit (7),           "7",     360.f,  0.78f, 1.00f, 0.055f,0.65f, 0.20f, 0.15f, 0.f,   0.f, 1.f, 0.f },
    { "08 Growl",       "Dirty",   fxBit (3) | fxBit (6), "3,6",  480.f,  0.62f, 0.70f, 0.20f, 0.60f, 0.75f, 0.60f, 0.f,   0.f, 1.f, 0.f },
    { "09 Slide",       "Acid",    fxBit (2),           "2",     300.f,  0.74f, 0.68f, 0.32f, 0.50f, 0.15f, 0.f,   0.25f, 0.f, 1.f, 0.f },
    { "10 Open",        "Classic", fxBit (0) | fxBit (8), "0,8", 3200.f,  0.08f, 0.25f, 0.45f, 0.35f, 0.05f, 0.f,   0.f,   1.f, 1.f, 0.f },
    { "11 Acid Rain",   "Acid",    fxBit (2) | fxBit (8), "2,8",  240.f,  0.86f, 0.90f, 0.14f, 0.80f, 0.18f, 0.05f, 0.f,   0.f, 1.f, 0.f },
    { "12 Chrome Bass", "Bass",    fxBit (1) | fxBit (0), "1,0",  180.f,  0.55f, 0.40f, 0.40f, 0.35f, 0.10f, 0.f,   0.02f, 0.f, 1.f, 0.1f },
    { "13 Needle",      "Lead",    fxBit (3) | fxBit (2), "3,2",  900.f,  0.70f, 0.95f, 0.08f, 0.80f, 0.45f, 0.20f, 0.f,   0.f, 1.f, 0.f },
    { "14 Warehouse",   "Space",   fxBit (2) | fxBit (8) | fxBit (4), "4,2,8", 200.f, 0.35f, 0.50f, 0.70f, 0.30f, 0.05f, 0.f, 0.f, 1.f, 1.f, 0.f },
    { "15 Grit Saw",    "Dirty",   fxBit (3) | fxBit (4), "3,4",  350.f,  0.80f, 0.75f, 0.18f, 0.70f, 0.55f, 0.45f, 0.f,   0.f, 1.f, 0.f },
    { "16 Night Bus",   "Acid",    fxBit (5) | fxBit (2), "5,2",  220.f,  0.88f, 0.85f, 0.20f, 0.70f, 0.22f, 0.10f, 0.f,   0.f, 1.f, 0.12f },
    { "17 Velvet Sqr",  "Classic", fxBit (1) | fxBit (0), "1,0",  500.f,  0.40f, 0.50f, 0.50f, 0.40f, 0.f,   0.f,   0.03f, 1.f, 1.f, 0.f },
    { "18 Acid Fang",   "Acid",    fxBit (3) | fxBit (2), "3,2",  260.f,  0.90f, 0.92f, 0.12f, 0.85f, 0.35f, 0.15f, 0.f,   0.f, 1.f, 0.f },
    { "19 Sub Crawl",   "Bass",    fxBit (5) | fxBit (4), "5,4",  70.f,   0.25f, 0.22f, 0.55f, 0.20f, 0.05f, 0.f,   0.08f, 0.f, 1.f, 0.f },
    { "20 Ice Lead",    "Lead",    fxBit (7) | fxBit (2), "7,2",  1400.f, 0.45f, 0.80f, 0.22f, 0.55f, 0.15f, 0.f,   0.f,   1.f, 1.f, 0.f },
    { "21 Damp Room",   "Space",   fxBit (8) | fxBit (4), "4,8",  160.f,  0.30f, 0.35f, 0.80f, 0.25f, 0.f,   0.f,   0.f,   1.f, 1.f, 0.f },
    { "22 Broken Jack", "Dirty",   fxBit (3) | fxBit (2), "3,2",  400.f,  0.68f, 0.60f, 0.16f, 0.55f, 0.50f, 0.55f, 0.f,   0.f, 0.85f, 0.f },
    { "23 Rubberband",  "Acid",    fxBit (0) | fxBit (2), "0,2",  380.f,  0.75f, 0.62f, 0.28f, 0.50f, 0.10f, 0.f,   0.06f, 1.f, 1.f, 0.f },
    { "24 Peak Hour",   "Lead",    fxBit (3) | fxBit (1) | fxBit (2), "1,3,2", 700.f, 0.58f, 0.88f, 0.10f, 0.75f, 0.40f, 0.20f, 0.f, 0.f, 1.f, 0.f },
    { "25 Fog Bass",    "Bass",    fxBit (5) | fxBit (8), "5,8",  100.f,  0.20f, 0.28f, 0.65f, 0.22f, 0.08f, 0.05f, 0.04f, 0.f, 1.f, 0.f },
    { "26 Silverline",  "Classic", fxBit (0) | fxBit (7), "0,7",  800.f,  0.35f, 0.45f, 0.40f, 0.40f, 0.05f, 0.f,   0.f,   1.f, 1.f, 0.f },
    { "27 Acid Trash",  "Dirty",   fxBit (3) | fxBit (6) | fxBit (2), "3,6,2", 310.f, 0.84f, 0.80f, 0.15f, 0.72f, 0.60f, 0.50f, 0.f, 0.f, 1.f, 0.f },
    { "28 Wide Open",   "Lead",    fxBit (0) | fxBit (8), "0,8",  2400.f, 0.18f, 0.40f, 0.35f, 0.40f, 0.10f, 0.f,   0.f,   1.f, 1.f, 0.f },
    { "29 Tick Loop",   "Acid",    fxBit (2),           "2",     290.f,  0.80f, 0.78f, 0.11f, 0.78f, 0.20f, 0.f,   0.f,   0.f, 0.7f, 0.2f },
    { "30 Low Rider",   "Bass",    fxBit (1) | fxBit (4), "1,4",  120.f,  0.42f, 0.30f, 0.45f, 0.30f, 0.12f, 0.f,   0.05f, 0.f, 1.f, 0.f },
    { "31 Phaser Diet", "Lead",    fxBit (7) | fxBit (5), "5,7",  600.f,  0.55f, 0.70f, 0.25f, 0.50f, 0.18f, 0.08f, 0.f,   1.f, 1.f, 0.f },
    { "32 Cave Acid",   "Space",   fxBit (8) | fxBit (2), "2,8",  210.f,  0.72f, 0.60f, 0.50f, 0.40f, 0.10f, 0.f,   0.f,   0.f, 1.f, 0.f },
    { "33 Mean Street", "Dirty",   fxBit (3) | fxBit (1), "3,1",  330.f,  0.85f, 0.82f, 0.13f, 0.80f, 0.70f, 0.40f, 0.f,   0.f, 1.f, 0.f },
    { "34 Soft Acid",   "Acid",    fxBit (0) | fxBit (5), "5,0",  340.f,  0.70f, 0.72f, 0.24f, 0.55f, 0.08f, 0.f,   0.f,   0.f, 1.f, 0.f },
    { "35 Tape Echo",   "Classic", fxBit (2),           "2",     450.f,  0.48f, 0.55f, 0.38f, 0.45f, 0.10f, 0.f,   0.02f, 1.f, 1.f, 0.05f },
    { "36 Acid Choir",  "Acid",    fxBit (0) | fxBit (2) | fxBit (8), "0,2,8", 270.f, 0.78f, 0.80f, 0.22f, 0.65f, 0.15f, 0.05f, 0.f, 0.f, 1.f, 0.f },
    { "37 Reso Stab",   "Lead",    fxBit (5),           "5",     1500.f, 0.88f, 0.95f, 0.06f, 0.85f, 0.25f, 0.10f, 0.f,   0.f, 0.8f, 0.f },
    { "38 Mudslide",    "Bass",    fxBit (3) | fxBit (5), "3,5",  95.f,   0.50f, 0.35f, 0.48f, 0.40f, 0.45f, 0.30f, 0.12f, 0.f, 1.f, 0.f },
    { "39 Sky Hook",    "Space",   fxBit (7) | fxBit (2) | fxBit (8), "7,2,8", 480.f, 0.40f, 0.55f, 0.60f, 0.35f, 0.08f, 0.f, 0.f, 1.f, 1.f, 0.f },
    { "40 Factory Floor","Dirty",  fxBit (3) | fxBit (1) | fxBit (2), "3,1,2", 250.f, 0.77f, 0.70f, 0.18f, 0.68f, 0.55f, 0.35f, 0.f, 0.f, 1.f, 0.08f },
};

inline constexpr FactoryFxParam factoryFxParams[] = {
    { 0, ParamID::fxDlyMix, 0.28f }, { 0, ParamID::fxDlyFb, 0.32f }, { 0, ParamID::fxDlySync, 1.f },
    { 1, ParamID::fxDstDrive, 0.62f }, { 1, ParamID::fxDstMix, 0.55f }, { 1, ParamID::fxCmpThr, -16.f },
    { 1, ParamID::fxCmpRat, 6.f }, { 1, ParamID::fxCmpMix, 0.7f },
    { 2, ParamID::fxChoMix, 0.45f }, { 2, ParamID::fxChoDepth, 0.5f }, { 2, ParamID::fxChoWidth, 0.4f },
    { 3, ParamID::fxRevMix, 0.4f }, { 3, ParamID::fxRevSize, 0.7f }, { 3, ParamID::fxRevPre, 40.f },
    { 4, ParamID::fxFltCut, 1800.f }, { 4, ParamID::fxFltRes, 0.45f }, { 4, ParamID::fxFltMix, 1.f },
    { 5, ParamID::fxFltCut, 280.f }, { 5, ParamID::fxFltType, 0.f }, { 5, ParamID::fxEqGain[0], 3.f },
    { 6, ParamID::fxPhaMix, 0.55f }, { 6, ParamID::fxPhaRate, 0.35f }, { 6, ParamID::fxPhaStages, 2.f },
    { 7, ParamID::fxDstDrive, 0.7f }, { 7, ParamID::fxDstType, 1.f }, { 7, ParamID::fxFlaMix, 0.4f },
    { 7, ParamID::fxFlaFb, 0.6f },
    { 8, ParamID::fxDlyMix, 0.35f }, { 8, ParamID::fxDlyFb, 0.4f }, { 8, ParamID::fxDlyPing, 1.f },
    { 9, ParamID::fxChoMix, 0.3f }, { 9, ParamID::fxRevMix, 0.22f }, { 9, ParamID::fxRevWidth, 1.f },
    { 10, ParamID::fxDlyMix, 0.3f }, { 10, ParamID::fxRevMix, 0.28f }, { 10, ParamID::fxRevSize, 0.55f },
    { 11, ParamID::fxCmpThr, -18.f }, { 11, ParamID::fxCmpMake, 4.f }, { 11, ParamID::fxChoMix, 0.35f },
    { 12, ParamID::fxDstDrive, 0.5f }, { 12, ParamID::fxDlyMix, 0.32f }, { 12, ParamID::fxDlyDiv, 0.f },
    { 13, ParamID::fxDlyMix, 0.4f }, { 13, ParamID::fxRevMix, 0.45f }, { 13, ParamID::fxEqGain[4], -3.f },
    { 14, ParamID::fxDstDrive, 0.55f }, { 14, ParamID::fxDstTone, 0.45f }, { 14, ParamID::fxEqGain[3], 2.f },
    { 15, ParamID::fxFltCut, 900.f }, { 15, ParamID::fxDlyMix, 0.3f }, { 15, ParamID::fxDlyFb, 0.45f },
    { 16, ParamID::fxCmpMix, 0.8f }, { 16, ParamID::fxChoDepth, 0.3f }, { 16, ParamID::fxChoRate, 0.15f },
    { 17, ParamID::fxDstMix, 0.4f }, { 17, ParamID::fxDlyMix, 0.38f }, { 17, ParamID::fxDlyFb, 0.5f },
    { 18, ParamID::fxFltCut, 180.f }, { 18, ParamID::fxFltSlope, 1.f }, { 18, ParamID::fxEqGain[0], 4.f },
    { 19, ParamID::fxPhaMix, 0.5f }, { 19, ParamID::fxPhaCentre, 1800.f }, { 19, ParamID::fxDlyMix, 0.25f },
    { 20, ParamID::fxRevMix, 0.5f }, { 20, ParamID::fxRevDamp, 0.65f }, { 20, ParamID::fxEqGain[1], -2.f },
    { 21, ParamID::fxDstType, 2.f }, { 21, ParamID::fxDstDrive, 0.48f }, { 21, ParamID::fxDlyMix, 0.3f },
    { 22, ParamID::fxChoMix, 0.4f }, { 22, ParamID::fxDlyMix, 0.28f }, { 22, ParamID::fxChoWidth, 0.55f },
    { 23, ParamID::fxCmpThr, -14.f }, { 23, ParamID::fxDstMix, 0.45f }, { 23, ParamID::fxDlyMix, 0.3f },
    { 24, ParamID::fxFltCut, 220.f }, { 24, ParamID::fxRevMix, 0.35f }, { 24, ParamID::fxRevSize, 0.6f },
    { 25, ParamID::fxChoMix, 0.38f }, { 25, ParamID::fxPhaMix, 0.4f }, { 25, ParamID::fxPhaRate, 0.12f },
    { 26, ParamID::fxDstDrive, 0.65f }, { 26, ParamID::fxFlaMix, 0.35f }, { 26, ParamID::fxDlyMix, 0.28f },
    { 27, ParamID::fxChoMix, 0.4f }, { 27, ParamID::fxChoWidth, 0.7f }, { 27, ParamID::fxRevMix, 0.3f },
    { 28, ParamID::fxDlyPing, 1.f }, { 28, ParamID::fxDlyMix, 0.34f }, { 28, ParamID::fxDlyDiv, 0.f },
    { 29, ParamID::fxCmpThr, -12.f }, { 29, ParamID::fxCmpRat, 8.f }, { 29, ParamID::fxEqGain[0], 2.5f },
    { 30, ParamID::fxFltCut, 1200.f }, { 30, ParamID::fxPhaMix, 0.48f }, { 30, ParamID::fxPhaStages, 3.f },
    { 31, ParamID::fxRevSize, 0.8f }, { 31, ParamID::fxRevMix, 0.42f }, { 31, ParamID::fxDlyMix, 0.22f },
    { 32, ParamID::fxDstDrive, 0.72f }, { 32, ParamID::fxDstType, 1.f }, { 32, ParamID::fxCmpThr, -18.f },
    { 33, ParamID::fxChoMix, 0.32f }, { 33, ParamID::fxFltCut, 1400.f }, { 33, ParamID::fxFltRes, 0.3f },
    { 34, ParamID::fxDlyMix, 0.4f }, { 34, ParamID::fxDlyFb, 0.5f }, { 34, ParamID::fxDlyCut, 0.45f },
    { 35, ParamID::fxChoMix, 0.35f }, { 35, ParamID::fxDlyMix, 0.3f }, { 35, ParamID::fxRevMix, 0.25f },
    { 36, ParamID::fxFltCut, 2200.f }, { 36, ParamID::fxFltRes, 0.7f }, { 36, ParamID::fxFltSlope, 1.f },
    { 37, ParamID::fxDstMix, 0.4f }, { 37, ParamID::fxFltCut, 250.f }, { 37, ParamID::fxFltDrive, 0.35f },
    { 38, ParamID::fxPhaMix, 0.45f }, { 38, ParamID::fxDlyMix, 0.32f }, { 38, ParamID::fxRevMix, 0.38f },
    { 39, ParamID::fxDstDrive, 0.58f }, { 39, ParamID::fxCmpMake, 3.f }, { 39, ParamID::fxDlyMix, 0.3f },
};

inline constexpr int factoryPatchCount = (int) (sizeof factoryPatches / sizeof factoryPatches[0]);
inline constexpr int factoryFxParamCount = (int) (sizeof factoryFxParams / sizeof factoryFxParams[0]);

inline const char* factoryCategoryForName (const char* name)
{
    if (name == nullptr)
        return "User";
    for (int i = 0; i < factoryPatchCount; ++i)
        if (std::strcmp (factoryPatches[i].name, name) == 0)
            return factoryPatches[i].category;
    return "User";
}

// Next/prev among stems that share current's category. Missing current (Init) wraps the full list.
inline int stepPatchInCategory (const char* current, const char* const* names, int n, int delta)
{
    if (n <= 0)
        return -1;
    int cur = -1;
    if (current != nullptr)
        for (int i = 0; i < n; ++i)
            if (std::strcmp (names[i], current) == 0)
            {
                cur = i;
                break;
            }
    if (cur < 0)
        return delta > 0 ? 0 : n - 1;

    const char* cat = factoryCategoryForName (names[cur]);
    int first = -1, last = -1, after = -1, before = -1;
    for (int i = 0; i < n; ++i)
    {
        if (std::strcmp (factoryCategoryForName (names[i]), cat) != 0)
            continue;
        if (first < 0)
            first = i;
        if (i > cur && after < 0)
            after = i;
        if (i < cur)
            before = i;
        last = i;
    }
    if (delta > 0)
        return after >= 0 ? after : first;
    return before >= 0 ? before : last;
}

inline bool factoryHasParamId (const char* id)
{
    if (id == nullptr)
        return false;
    if (ParamID::fxModFor (id) != nullptr)
        return true;
    for (int t = 0; t < ParamID::fxCount; ++t)
        if (std::strcmp (ParamID::fxOnIds[t], id) == 0)
            return true;
    return std::strcmp (id, ParamID::fxDlySync) == 0
        || std::strcmp (id, ParamID::fxDlyDiv) == 0
        || std::strcmp (id, ParamID::fxDlyPing) == 0
        || std::strcmp (id, ParamID::fxDstType) == 0
        || std::strcmp (id, ParamID::fxFltType) == 0
        || std::strcmp (id, ParamID::fxFltSlope) == 0
        || std::strcmp (id, ParamID::fxPhaStages) == 0
        || std::strcmp (id, ParamID::fxRevFreeze) == 0
        || std::strcmp (id, ParamID::fxEqTrim) == 0;
}

inline std::string factoryPatchXml (int index)
{
    const auto& v = factoryPatches[index];
    std::vector<PatchParam> params;
    auto add = [&] (const char* id, float value)
    {
        params.push_back ({ id, value });
    };

    add (ParamID::waveform, v.waveform);
    add (ParamID::cutoff, v.cutoff);
    add (ParamID::resonance, v.resonance);
    add (ParamID::envMod, v.envMod);
    add (ParamID::decay, v.decay);
    add (ParamID::accent, v.accent);
    add (ParamID::drive, v.drive);
    add (ParamID::nasty, v.nasty);
    add (ParamID::glide, v.glide);
    add (ParamID::seqGate, v.gate);
    add (ParamID::seqSwing, v.swing);
    add (ParamID::volume, 0.25f);

    for (int d = 0; d < ParamID::destCount; ++d)
    {
        add (ParamID::destLfoIds[d], 0.f);
        add (ParamID::destAmtIds[d], 0.f);
    }
    for (int i = 0; i < ParamID::fxCount; ++i)
        add (ParamID::fxOnIds[i], (v.fxMask & fxBit (i)) ? 1.f : 0.f);
    add (ParamID::preEqOn, 0.f);
    add (ParamID::postEqOn, 0.f);

    for (int i = 0; i < factoryFxParamCount; ++i)
        if (factoryFxParams[i].patch == index)
            add (factoryFxParams[i].id, factoryFxParams[i].value);

    LfoShape shapes[numLfos];
    defaultLfoShapes (shapes);
    const char* order = v.fxOrder != nullptr ? v.fxOrder : "";
    return writePatchXml (params, shapes, order, v.category);
}

} // namespace tew
