#pragma once

#include "ParameterIDs.h"

#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    // Pots show 0-100% of travel. DSP ranges stay as-is.
    auto pct = [] (NormalisableRange<float> range)
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([range] (float v, int)
            {
                return String (roundToInt (range.convertTo0to1 (v) * 100.f)) + "%";
            })
            .withValueFromStringFunction ([range] (const String& t)
            {
                return range.convertFrom0to1 (jlimit (0.f, 1.f, t.getFloatValue() / 100.f));
            });
    };

    auto hz = NormalisableRange<float> (20.f, 8000.f, 0.01f, 0.3f);
    auto decayR = NormalisableRange<float> (0.05f, 2.f, 0.001f, 0.4f);
    auto unit = NormalisableRange<float> (0.f, 1.f);
    auto glideR = NormalisableRange<float> (0.f, 0.5f);

    auto cutoffAttrs = AudioParameterFloatAttributes()
        .withStringFromValueFunction ([hz] (float v, int)
        {
            return String (roundToInt (hz.convertTo0to1 (v) * 100.f)) + "% "
                 + String (roundToInt (v)) + "Hz";
        })
        .withValueFromStringFunction ([hz] (const String& t)
        {
            const bool asHz = t.containsIgnoreCase ("hz")
                              || (! t.contains ("%") && t.getFloatValue() > 100.f);
            if (asHz)
                return jlimit (hz.start, hz.end, t.getFloatValue());
            return hz.convertFrom0to1 (jlimit (0.f, 1.f, t.getFloatValue() / 100.f));
        });

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::cutoff, 1 },
                                                        "Cutoff", hz, 800.f, cutoffAttrs));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::resonance, 1 },
                                                        "Resonance", unit, 0.3f, pct (unit)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::decay, 1 },
                                                        "Decay", decayR, 0.25f, pct (decayR)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::envMod, 1 },
                                                        "Env Mod", unit, 0.7f, pct (unit)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::accent, 1 },
                                                        "Accent", unit, 0.5f, pct (unit)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::drive, 1 },
                                                        "Drive", unit, 0.f, pct (unit)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::volume, 1 },
                                                        "Volume", unit, 0.25f, pct (unit)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::waveform, 1 },
                                                       "Square", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::glide, 1 },
                                                        "Glide", glideR, 0.f, pct (glideR)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::seqPlay, 1 },
                                                       "Run", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::playMode, 1 },
                                                         "Play",
                                                         StringArray { "Keyboard", "Pattern", "Key" },
                                                         0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::seqTempo, 1 },
                                                        "Seq Tempo",
                                                        NormalisableRange<float> (40.f, 300.f, 0.1f),
                                                        130.f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::seq2x, 1 },
                                                       "2x", false));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::seqBank, 1 },
                                                      "Bank", 0, 2, 0));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::seqPattern, 1 },
                                                      "Pattern", 0, 11, 0));

    auto addLfo = [&] (int i)
    {
        const String n = "LFO " + String (i + 1);
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { ParamID::lfoRateIds[i], 1 }, n + " Rate",
            NormalisableRange<float> (0.05f, 30.f, 0.01f, 0.4f), 1.f));
        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { ParamID::lfoSyncIds[i], 1 }, n + " Sync", false));
        layout.add (std::make_unique<AudioParameterChoice> (
            ParameterID { ParamID::lfoDivIds[i], 1 }, n + " Div",
            StringArray { "1/16", "1/8", "1/4", "1/2", "1", "2" }, 2));
        layout.add (std::make_unique<AudioParameterChoice> (
            ParameterID { ParamID::lfoModeIds[i], 1 }, n + " Mode",
            StringArray { "Free", "Trigger" }, 0));
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { ParamID::lfoSmoothIds[i], 1 }, n + " Smooth", unit, 0.f, pct (unit)));
    };
    addLfo (0);
    addLfo (1);

    auto amtR = NormalisableRange<float> (-1.f, 1.f);
    for (int d = 0; d < 8; ++d)
    {
        layout.add (std::make_unique<AudioParameterInt> (
            ParameterID { ParamID::destLfoIds[d], 1 },
            String (ParamID::destIds[d]) + " LFO", 0, 2, 0));
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { ParamID::destAmtIds[d], 1 },
            String (ParamID::destIds[d]) + " LFO Amt", amtR, 0.f));
    }

    auto addOn = [&] (const char* id, const char* name)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, false));
    };
    auto addUnit = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, unit, def, pct (unit)));
    };

    addOn (ParamID::fxOnIds[0], "Chorus On");
    addUnit (ParamID::fxChoRate, "Chorus Rate", 0.25f);
    addUnit (ParamID::fxChoDepth, "Chorus Depth", 0.4f);
    addUnit (ParamID::fxChoMix, "Chorus Mix", 0.4f);
    addUnit (ParamID::fxChoFb, "Chorus Fb", 0.1f);

    addOn (ParamID::fxOnIds[1], "Comp On");
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxCmpThr, 1 }, "Comp Thresh",
        NormalisableRange<float> (-40.f, 0.f), -12.f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxCmpRat, 1 }, "Comp Ratio",
        NormalisableRange<float> (1.f, 20.f, 0.1f, 0.4f), 4.f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxCmpAtk, 1 }, "Comp Attack",
        NormalisableRange<float> (0.1f, 100.f, 0.01f, 0.4f), 10.f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxCmpRel, 1 }, "Comp Release",
        NormalisableRange<float> (10.f, 500.f, 0.1f, 0.4f), 80.f));
    addUnit (ParamID::fxCmpMix, "Comp Mix", 1.f);

    addOn (ParamID::fxOnIds[2], "Delay On");
    addUnit (ParamID::fxDlyTime, "Delay Time", 0.35f);
    addUnit (ParamID::fxDlyFb, "Delay Fb", 0.35f);
    addUnit (ParamID::fxDlyMix, "Delay Mix", 0.3f);
    addUnit (ParamID::fxDlyCut, "Delay Cut", 0.7f);
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::fxDlySync, 1 }, "Delay Sync", true));
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::fxDlyDiv, 1 }, "Delay Div",
        StringArray { "1/16", "1/8", "1/4", "1/2" }, 1));

    addOn (ParamID::fxOnIds[3], "Dist On");
    addUnit (ParamID::fxDstDrive, "Dist Drive", 0.4f);
    addUnit (ParamID::fxDstMix, "Dist Mix", 0.5f);

    addOn (ParamID::fxOnIds[4], "EQ On");
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxEqLow, 1 }, "EQ Low",
        NormalisableRange<float> (-12.f, 12.f), 0.f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxEqMid, 1 }, "EQ Mid",
        NormalisableRange<float> (-12.f, 12.f), 0.f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxEqHigh, 1 }, "EQ High",
        NormalisableRange<float> (-12.f, 12.f), 0.f));

    addOn (ParamID::fxOnIds[5], "FX Filter On");
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxFltCut, 1 }, "FX Cutoff",
        NormalisableRange<float> (20.f, 12000.f, 0.01f, 0.3f), 2000.f));
    addUnit (ParamID::fxFltRes, "FX Res", 0.2f);
    addUnit (ParamID::fxFltMix, "FX Filt Mix", 1.f);
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::fxFltType, 1 }, "FX Filt Type",
        StringArray { "Low", "Band", "High" }, 0));

    addOn (ParamID::fxOnIds[6], "Flanger On");
    addUnit (ParamID::fxFlaRate, "Flanger Rate", 0.2f);
    addUnit (ParamID::fxFlaDepth, "Flanger Depth", 0.5f);
    addUnit (ParamID::fxFlaFb, "Flanger Fb", 0.45f);
    addUnit (ParamID::fxFlaMix, "Flanger Mix", 0.4f);

    addOn (ParamID::fxOnIds[7], "Phaser On");
    addUnit (ParamID::fxPhaRate, "Phaser Rate", 0.2f);
    addUnit (ParamID::fxPhaDepth, "Phaser Depth", 0.6f);
    addUnit (ParamID::fxPhaFb, "Phaser Fb", 0.3f);
    addUnit (ParamID::fxPhaMix, "Phaser Mix", 0.5f);
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::fxPhaCentre, 1 }, "Phaser Centre",
        NormalisableRange<float> (200.f, 4000.f, 0.01f, 0.3f), 1300.f));

    addOn (ParamID::fxOnIds[8], "Reverb On");
    addUnit (ParamID::fxRevSize, "Reverb Size", 0.45f);
    addUnit (ParamID::fxRevDamp, "Reverb Damp", 0.4f);
    addUnit (ParamID::fxRevMix, "Reverb Mix", 0.25f);
    addUnit (ParamID::fxRevWidth, "Reverb Width", 0.8f);

    return layout;
}
