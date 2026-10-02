#pragma once

#include "Voice.h"

#include <JuceHeader.h>

namespace tew
{

// Owns the voice. Cutoff / resonance / drive / classic are latched for the filter pass.
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

    // ponytail: stored only. Diode ladder reads these when that pass lands.
    void latchFilter (float cutoffHz, float resonanceAmount, float driveAmount, bool classic)
    {
        cutoff = cutoffHz;
        resonance = resonanceAmount;
        drive = driveAmount;
        classicMode = classic;
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
    float cutoff = 800.f;
    float resonance = 0.f;
    float drive = 0.f;
    bool classicMode = false;
};

} // namespace tew
