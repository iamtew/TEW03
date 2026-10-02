#pragma once

#include <cassert>
#include <cmath>

namespace tew
{

// Cycles per sample. 1.0 means one cycle every sample.
constexpr float phaseIncrement (float hz, float sampleRate)
{
    return hz / sampleRate;
}

// 44100 Hz at 44100 Hz sample rate is exactly one cycle per sample.
static_assert (phaseIncrement (44100.f, 44100.f) == 1.f);

// Band-limited saw. Phase runs 0..1.
struct Oscillator
{
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        reset();
    }

    void reset()
    {
        phase = 0.f;
        inc = 0.f;
    }

    void setFrequency (float hz)
    {
        inc = phaseIncrement (hz, sampleRate);
    }

    float process()
    {
        // Naive saw is -1..1, rising. PolyBLEP knocks the step down.
        float saw = (2.f * phase) - 1.f;
        saw -= polyBlep (phase, inc);

        phase += inc;
        if (phase >= 1.f)
            phase -= 1.f;

        return saw;
    }

private:
    static float polyBlep (float t, float dt)
    {
        if (dt <= 0.f)
            return 0.f;

        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.f;
        }

        if (t > 1.f - dt)
        {
            t = (t - 1.f) / dt;
            return t * t + t + t + 1.f;
        }

        return 0.f;
    }

    float sampleRate = 44100.f;
    float phase = 0.f;
    float inc = 0.f;
};

} // namespace tew
