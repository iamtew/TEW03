#pragma once

#include <algorithm>
#include <cmath>

namespace tew
{

// Attack is instant. Level falls toward 0 with a time constant of decaySeconds.
struct Envelope
{
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        setDecaySeconds (decaySeconds);
    }

    void setDecaySeconds (float seconds)
    {
        decaySeconds = std::clamp (seconds, 0.02f, 4.f);
        // coeff = e^(-1 / tau). One time-constant of decay per decaySeconds.
        const float tauSamples = decaySeconds * sampleRate;
        coeff = std::exp (-1.f / tauSamples);
    }

    void noteOn() { level = 1.f; }
    void noteOff() { level = 0.f; }

    float process()
    {
        level *= coeff;
        return level;
    }

private:
    float sampleRate = 44100.f;
    float decaySeconds = 0.3f;
    float coeff = 0.f;
    float level = 0.f;
};

} // namespace tew
