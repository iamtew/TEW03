#pragma once

#include <algorithm>
#include <cmath>

namespace tew
{

// Fast attack / 303-style decay / short release. Instant 0↔1 clicks the output.
struct Envelope
{
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        attackStep = 1.f / std::max (1.f, 0.001f * sampleRate);
        releaseStep = 1.f / std::max (1.f, 0.005f * sampleRate);
        setDecaySeconds (decaySeconds);
    }

    void setDecaySeconds (float seconds)
    {
        decaySeconds = std::clamp (seconds, 0.02f, 4.f);
        // coeff = e^(-1 / tau). One time-constant of decay per decaySeconds.
        const float tauSamples = decaySeconds * sampleRate;
        coeff = std::exp (-1.f / tauSamples);
    }

    void noteOn() { phase = 0; }
    void noteOff() { phase = 2; }

    float process()
    {
        if (phase == 0)
        {
            level = std::min (1.f, level + attackStep);
            if (level >= 1.f)
            {
                level = 1.f;
                phase = 1;
            }
        }
        else if (phase == 1)
        {
            level *= coeff;
        }
        else
        {
            level = std::max (0.f, level - releaseStep);
        }
        return level;
    }

private:
    float sampleRate = 44100.f;
    float decaySeconds = 0.3f;
    float coeff = 0.f;
    float level = 0.f;
    float attackStep = 1.f / 44.1f;
    float releaseStep = 1.f / 220.5f;
    int phase = 2; // 0 attack, 1 decay, 2 release
};

} // namespace tew
