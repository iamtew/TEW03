#pragma once

#include "Voice.h"

#include <JuceHeader.h>

namespace tew
{

struct SynthEngine
{
    void prepare (double sampleRate)
    {
        voice.prepare (sampleRate);
    }

    void setTone (float decaySeconds, float accent, float glideSeconds, float volume)
    {
        voice.setDecaySeconds (decaySeconds);
        voice.setAccentAmount (accent);
        voice.setGlideSeconds (glideSeconds);
        voice.setGain (volume);
    }

    void setFilter (float cutoffHz, float resonanceAmount, float driveAmount, bool classic)
    {
        voice.setFilter (cutoffHz, resonanceAmount, driveAmount, classic);
    }

    Voice& getVoice() { return voice; }

    void render (juce::AudioBuffer<float>& buffer, int start, int numSamples)
    {
        if (numSamples <= 0 || buffer.getNumChannels() == 0)
            return;

        auto* left = buffer.getWritePointer (0, start);
        voice.render (left, numSamples);

        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (ch, start), left, numSamples);
    }

private:
    Voice voice;
};

} // namespace tew
