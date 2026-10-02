#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>

namespace tew
{

// Stay off Nyquist. tan(pi * fc / fs) blows up as fc approaches fs/2.
constexpr float kNyquistHeadroom = 0.45f;
static_assert (kNyquistHeadroom < 0.5f);

// Four one-poles with tanh at the input and in the feedback. Diode-ish, not a circuit model.
struct DiodeLadderFilter
{
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        reset();
    }

    void reset()
    {
        y1 = y2 = y3 = y4 = 0.f;
        g = 0.f;
        k = 0.f;
    }

    void set (float cutoffHz, float resonance)
    {
        const float maxFc = kNyquistHeadroom * sampleRate;
        assert (maxFc > 0.f);
        const float fc = std::clamp (cutoffHz, 20.f, maxFc);
        g = std::tan (3.14159265f * fc / sampleRate);

        // Linear 4-pole self-oscillates near k = 4. tanh needs a little less.
        k = std::clamp (resonance, 0.f, 1.f) * 3.8f;
    }

    float process (float x)
    {
        const float input = std::tanh (x - k * y4);
        const float gi = g / (1.f + g);

        y1 += gi * (input - y1);
        y2 += gi * (std::tanh (y1) - y2);
        y3 += gi * (y2 - y3);
        y4 += gi * (y3 - y4);
        return y4;
    }

private:
    float sampleRate = 44100.f;
    float y1 = 0.f;
    float y2 = 0.f;
    float y3 = 0.f;
    float y4 = 0.f;
    float g = 0.f;
    float k = 0.f;
};

} // namespace tew
