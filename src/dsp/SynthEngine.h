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

    void setTone (float decaySeconds, float accent, float glideSeconds, float volume, bool square)
    {
        voice.setDecaySeconds (decaySeconds);
        voice.setAccentAmount (accent);
        voice.setGlideSeconds (glideSeconds);
        voice.setGain (volume);
        voice.setSquare (square);
    }

    void setFilter (float cutoffHz, float resonanceAmount, float driveAmount, float envModAmount)
    {
        voice.setFilter (cutoffHz, resonanceAmount, driveAmount, envModAmount);
    }

    Voice& getVoice() { return voice; }

    void render (juce::AudioBuffer<float>& buffer, int start, int numSamples, const VoiceMod* mod = nullptr)
    {
        if (numSamples <= 0 || buffer.getNumChannels() == 0)
            return;

        auto* left = buffer.getWritePointer (0, start);
        voice.render (left, numSamples, mod);

        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (ch, start), left, numSamples);
    }

private:
    Voice voice;
};

} // namespace tew
