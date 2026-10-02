#pragma once

#include "ParameterIDs.h"

#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    auto hz = NormalisableRange<float> (20.f, 8000.f, 0.01f, 0.3f);

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::cutoff, 1 },
                                                        "Cutoff", hz, 800.f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::resonance, 1 },
                                                        "Resonance",
                                                        NormalisableRange<float> (0.f, 1.f), 0.3f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::decay, 1 },
                                                        "Decay",
                                                        NormalisableRange<float> (0.05f, 2.f, 0.001f, 0.4f),
                                                        0.25f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::accent, 1 },
                                                        "Accent",
                                                        NormalisableRange<float> (0.f, 1.f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::drive, 1 },
                                                        "Drive",
                                                        NormalisableRange<float> (0.f, 1.f), 0.f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::volume, 1 },
                                                        "Volume",
                                                        NormalisableRange<float> (0.f, 1.f), 0.25f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::classicMode, 1 },
                                                       "Classic 303", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::waveform, 1 },
                                                       "Square", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::glide, 1 },
                                                        "Glide",
                                                        NormalisableRange<float> (0.f, 0.5f), 0.f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::seqPlay, 1 },
                                                       "Seq Play", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::seqTempo, 1 },
                                                        "Seq Tempo",
                                                        NormalisableRange<float> (40.f, 300.f, 0.1f),
                                                        130.f));

    return layout;
}
