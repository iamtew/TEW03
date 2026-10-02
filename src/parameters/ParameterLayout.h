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
                                                       "Sequencer", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::seqTempo, 1 },
                                                        "Seq Tempo",
                                                        NormalisableRange<float> (40.f, 300.f, 0.1f),
                                                        130.f));

    return layout;
}
