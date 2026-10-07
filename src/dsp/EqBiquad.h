#pragma once

#include <cmath>

namespace tew
{

inline constexpr int eqBandCount = 5;
inline constexpr int eqTypeOff = 0;
inline constexpr int eqTypePeak = 1;
inline constexpr int eqTypeLowShelf = 2;
inline constexpr int eqTypeHighShelf = 3;
inline constexpr int eqTypeHP = 4;
inline constexpr int eqTypeLP = 5;

struct Biquad
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
};

inline Biquad eqBiquad (int type, float sr, float hz, float q, float gainDb)
{
    if (type <= eqTypeOff || sr < 1.f)
        return {};

    const float ny = (float) sr * 0.45f;
    hz = hz < 20.f ? 20.f : (hz > ny ? ny : hz);
    q = q < 0.05f ? 0.05f : (q > 30.f ? 30.f : q);

    const float pi = 3.14159265358979323846f;
    const float w0 = 2.f * pi * hz / (float) sr;
    const float cosw = std::cos (w0);
    const float sinw = std::sin (w0);
    const float alpha = sinw / (2.f * q);
    const float A = std::pow (10.f, gainDb / 40.f);
    const float twoSqrtA = 2.f * std::sqrt (A) * alpha;

    auto norm = [] (float b0, float b1, float b2, float a0, float a1, float a2) -> Biquad
    {
        if (std::abs (a0) < 1.0e-12f)
            return {};
        const float inv = 1.f / a0;
        return { b0 * inv, b1 * inv, b2 * inv, a1 * inv, a2 * inv };
    };

    switch (type)
    {
        case eqTypePeak:
            return norm (1.f + alpha * A, -2.f * cosw, 1.f - alpha * A,
                         1.f + alpha / A, -2.f * cosw, 1.f - alpha / A);
        case eqTypeLowShelf:
            return norm (A * ((A + 1.f) - (A - 1.f) * cosw + twoSqrtA),
                         2.f * A * ((A - 1.f) - (A + 1.f) * cosw),
                         A * ((A + 1.f) - (A - 1.f) * cosw - twoSqrtA),
                         (A + 1.f) + (A - 1.f) * cosw + twoSqrtA,
                         -2.f * ((A - 1.f) + (A + 1.f) * cosw),
                         (A + 1.f) + (A - 1.f) * cosw - twoSqrtA);
        case eqTypeHighShelf:
            return norm (A * ((A + 1.f) + (A - 1.f) * cosw + twoSqrtA),
                         -2.f * A * ((A - 1.f) + (A + 1.f) * cosw),
                         A * ((A + 1.f) + (A - 1.f) * cosw - twoSqrtA),
                         (A + 1.f) - (A - 1.f) * cosw + twoSqrtA,
                         2.f * ((A - 1.f) - (A + 1.f) * cosw),
                         (A + 1.f) - (A - 1.f) * cosw - twoSqrtA);
        case eqTypeHP:
        {
            const float b0 = (1.f + cosw) * 0.5f;
            return norm (b0, -(1.f + cosw), b0, 1.f + alpha, -2.f * cosw, 1.f - alpha);
        }
        case eqTypeLP:
        {
            const float b0 = (1.f - cosw) * 0.5f;
            return norm (b0, 1.f - cosw, b0, 1.f + alpha, -2.f * cosw, 1.f - alpha);
        }
        default:
            return {};
    }
}

inline float eqBiquadMag (const Biquad& c, float w)
{
    const float cosw = std::cos (w);
    const float cos2 = std::cos (2.f * w);
    const float sinw = std::sin (w);
    const float sin2 = std::sin (2.f * w);
    const float nr = c.b0 + c.b1 * cosw + c.b2 * cos2;
    const float ni = -(c.b1 * sinw + c.b2 * sin2);
    const float dr = 1.f + c.a1 * cosw + c.a2 * cos2;
    const float di = -(c.a1 * sinw + c.a2 * sin2);
    const float den = dr * dr + di * di;
    if (den < 1.0e-20f)
        return 1.f;
    return std::sqrt ((nr * nr + ni * ni) / den);
}

inline float eqMagnitude (const Biquad* bands, int n, float sr, float hz)
{
    if (bands == nullptr || n <= 0 || sr < 1.f || hz <= 0.f)
        return 1.f;
    const float w = 2.f * 3.14159265358979323846f * hz / sr;
    float mag = 1.f;
    for (int i = 0; i < n; ++i)
        mag *= eqBiquadMag (bands[i], w);
    return mag;
}

} // namespace tew
