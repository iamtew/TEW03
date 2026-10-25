#pragma once

#include "Library.h"

namespace tew
{

// Built-in sounds for the patch menu. Columns are the Main knobs:
// cutoff Hz, resonance, env mod, decay seconds, accent, drive, nasty, glide seconds, waveform (0 saw, 1 square).
struct FactoryPatch
{
    const char* name;
    float cutoff;
    float resonance;
    float envMod;
    float decay;
    float accent;
    float drive;
    float nasty;
    float glide;
    float waveform;
};

// Numbered so the menu stays in this order (natural sort).
inline constexpr FactoryPatch factoryPatches[] = {
    { "01 Squelch",  280.f,  0.82f, 0.88f, 0.16f, 0.75f, 0.12f, 0.f,   0.f,   0.f }, // classic resonant saw
    { "02 Scream",   160.f,  0.96f, 0.98f, 0.10f, 0.90f, 0.40f, 0.25f, 0.f,   0.f }, // high res, short, driven
    { "03 Rubber",   420.f,  0.72f, 0.55f, 0.55f, 0.45f, 0.08f, 0.f,   0.04f, 1.f }, // square, longer decay
    { "04 Hollow",   140.f,  0.40f, 0.45f, 0.90f, 0.25f, 0.f,   0.f,   0.10f, 1.f }, // dark square, a little glide
    { "05 Bite",    1100.f,  0.50f, 0.92f, 0.07f, 0.85f, 0.30f, 0.10f, 0.f,   0.f }, // bright, very short
    { "06 Muffled",   90.f,  0.12f, 0.18f, 0.28f, 0.15f, 0.f,   0.f,   0.f,   1.f }, // closed filter, almost no env
    { "07 Laser",    360.f,  0.78f, 1.00f, 0.055f,0.65f, 0.20f, 0.15f, 0.f,   0.f }, // full env, snap decay
    { "08 Growl",    480.f,  0.62f, 0.70f, 0.20f, 0.60f, 0.75f, 0.60f, 0.f,   0.f }, // drive and nasty up
    { "09 Slide",    300.f,  0.74f, 0.68f, 0.32f, 0.50f, 0.15f, 0.f,   0.25f, 0.f }, // squelch with glide
    { "10 Open",    3200.f,  0.08f, 0.25f, 0.45f, 0.35f, 0.05f, 0.f,   0.f,   1.f }, // filter mostly open
};

inline constexpr int factoryPatchCount = (int) (sizeof factoryPatches / sizeof factoryPatches[0]);

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
    add (ParamID::volume, 0.25f);

    // A load must be this sound, not leftover modulation or effects from the previous patch.
    for (int d = 0; d < ParamID::destCount; ++d)
    {
        add (ParamID::destLfoIds[d], 0.f);
        add (ParamID::destAmtIds[d], 0.f);
    }
    for (int i = 0; i < ParamID::fxCount; ++i)
        add (ParamID::fxOnIds[i], 0.f);
    add (ParamID::preEqOn, 0.f);
    add (ParamID::postEqOn, 0.f);

    LfoShape shapes[numLfos];
    defaultLfoShapes (shapes);
    return writePatchXml (params, shapes);
}

} // namespace tew
