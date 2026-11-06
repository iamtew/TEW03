#pragma once

#include <algorithm>
#include <cmath>

namespace tew
{

// Feed-forward GR in dB. Knee is the full width; 0 is hard.
inline float compressorGainReductionDb (float envDb, float thrDb, float ratio, float kneeDb)
{
    const float r = std::max (1.f, ratio);
    const float slope = 1.f - 1.f / r;
    const float over = envDb - thrDb;
    if (kneeDb <= 0.05f)
        return over > 0.f ? over * slope : 0.f;
    const float half = kneeDb * 0.5f;
    if (over > half)
        return over * slope;
    if (over > -half)
    {
        const float t = over + half;
        return slope * t * t / (2.f * kneeDb);
    }
    return 0.f;
}

} // namespace tew
