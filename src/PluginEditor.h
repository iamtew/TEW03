#pragma once

#include "PluginProcessor.h"

class TEW03AudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit TEW03AudioProcessorEditor (TEW03AudioProcessor& processor)
        : juce::GenericAudioProcessorEditor (processor)
    {
    }
};
