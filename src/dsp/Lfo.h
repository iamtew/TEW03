#pragma once

#include <algorithm>
#include <cmath>

namespace tew
{

enum LfoDest
{
    destCutoff = 0,
    destResonance,
    destEnvMod,
    destDecay,
    destAccent,
    destDrive,
    destGlide,
    destVolume,
    destNasty,
    destCount
};

inline constexpr int numLfos = 2;
inline constexpr int lfoMaxPoints = 8;

// Matches ParameterLayout ranges so modulation stays in the same 0-1 space as the knobs.
inline float destFrom01 (int dest, float n)
{
    n = std::clamp (n, 0.f, 1.f);
    auto skewed = [] (float p, float start, float end, float skew)
    {
        if (skew == 1.f)
            return start + (end - start) * p;
        return start + (end - start) * std::pow (p, 1.f / skew);
    };

    switch (dest)
    {
        case destCutoff:    return skewed (n, 20.f, 8000.f, 0.3f);
        case destDecay:     return skewed (n, 0.05f, 2.f, 0.4f);
        case destGlide:     return n * 0.5f;
        default:            return n;
    }
}

inline float destTo01 (int dest, float v)
{
    auto skewed = [] (float x, float start, float end, float skew)
    {
        const float p = std::clamp ((x - start) / (end - start), 0.f, 1.f);
        if (skew == 1.f)
            return p;
        return std::pow (p, skew);
    };

    switch (dest)
    {
        case destCutoff:    return skewed (v, 20.f, 8000.f, 0.3f);
        case destDecay:     return skewed (v, 0.05f, 2.f, 0.4f);
        case destGlide:     return std::clamp (v / 0.5f, 0.f, 1.f);
        default:            return std::clamp (v, 0.f, 1.f);
    }
}

// 1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars. Cycle length in beats.
inline float beatsPerCycle (int divIndex)
{
    static constexpr float beats[] = { 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f };
    return beats[std::clamp (divIndex, 0, 5)];
}

inline float syncedHz (float bpm, int divIndex)
{
    const float b = std::max (0.001f, bpm);
    return (b / 60.f) / beatsPerCycle (divIndex);
}

// Sequencer loop is 4 beats. Used by the bar overlay and the LFO grid.
inline float cyclesPerBar (bool sync, int divIndex, float rateHz, float bpm)
{
    const float barBeats = 4.f;
    if (sync)
        return barBeats / beatsPerCycle (divIndex);
    return std::max (0.01f, rateHz) * barBeats * 60.f / std::max (0.001f, bpm);
}

inline int sixteenthsPerCycle (bool sync, int divIndex, float rateHz, float bpm)
{
    if (sync)
        return std::clamp ((int) std::lround (beatsPerCycle (divIndex) * 4.f), 1, 32);
    const float sixDur = 60.f / std::max (0.001f, bpm) / 4.f;
    const float cycDur = 1.f / std::max (0.01f, rateHz);
    return std::clamp ((int) std::lround (cycDur / sixDur), 1, 32);
}

struct LfoShape
{
    float x[lfoMaxPoints] {};
    float y[lfoMaxPoints] {};
    int n = 0;

    void clear() { n = 0; }

    void add (float px, float py)
    {
        if (n >= lfoMaxPoints)
            return;
        x[n] = std::clamp (px, 0.f, 1.f);
        y[n] = std::clamp (py, 0.f, 1.f);
        ++n;
    }

    void setTriangle()
    {
        clear();
        add (0.f, 0.f);
        add (0.5f, 1.f);
        add (1.f, 0.f);
    }

    void setSin()
    {
        clear();
        add (0.f, 0.5f);
        add (0.25f, 1.f);
        add (0.5f, 0.5f);
        add (0.75f, 0.f);
        add (1.f, 0.5f);
    }

    void setSawUp()
    {
        clear();
        add (0.f, 0.f);
        add (1.f, 1.f);
    }

    void setSawDown()
    {
        clear();
        add (0.f, 1.f);
        add (1.f, 0.f);
    }

    void setSquare()
    {
        clear();
        add (0.f, 1.f);
        add (0.5f, 1.f);
        add (0.5f, 0.f);
        add (1.f, 0.f);
    }

    int presetIndex() const
    {
        auto same = [this] (const LfoShape& o)
        {
            if (n != o.n)
                return false;
            for (int i = 0; i < n; ++i)
                if (std::abs (x[i] - o.x[i]) > 0.02f || std::abs (y[i] - o.y[i]) > 0.02f)
                    return false;
            return true;
        };

        LfoShape p;
        p.setTriangle();
        if (same (p)) return 0;
        p.setSin();
        if (same (p)) return 1;
        p.setSawUp();
        if (same (p)) return 2;
        p.setSawDown();
        if (same (p)) return 3;
        p.setSquare();
        if (same (p)) return 4;
        return -1;
    }

    void applyPreset (int index)
    {
        switch (index)
        {
            case 1:  setSin(); break;
            case 2:  setSawUp(); break;
            case 3:  setSawDown(); break;
            case 4:  setSquare(); break;
            default: setTriangle(); break;
        }
    }

    static const char* presetName (int index)
    {
        switch (index)
        {
            case 1:  return "Sin";
            case 2:  return "Saw Up";
            case 3:  return "Saw Down";
            case 4:  return "Square";
            default: return "Triangle";
        }
    }

    static constexpr int numPresets = 5;

    float lookup (float phase) const
    {
        if (n <= 0)
            return 0.f;
        if (n == 1)
            return y[0];

        const float p = phase - std::floor (phase);
        if (p <= x[0])
            return y[0];

        for (int i = 0; i < n - 1; ++i)
        {
            if (p <= x[i + 1])
            {
                const float dx = x[i + 1] - x[i];
                if (dx <= 1.0e-8f)
                    return y[i + 1];
                const float t = (p - x[i]) / dx;
                return y[i] + t * (y[i + 1] - y[i]);
            }
        }

        return y[n - 1];
    }
};

struct Lfo
{
    LfoShape shape;
    float phase = 0.f;
    float z = 0.f;
    double sampleRate = 44100.0;

    void prepare (double sr)
    {
        sampleRate = sr > 0.0 ? sr : 44100.0;
        if (shape.n < 2)
            shape.setTriangle();
        z = shape.lookup (phase);
    }

    void retrigger()
    {
        phase = 0.f;
        z = shape.lookup (0.f);
    }

    float peek() const { return z; }

    float process (float rateHz, float smooth)
    {
        const float sr = (float) sampleRate;
        phase += std::max (0.f, rateHz) / sr;
        phase -= std::floor (phase);
        const float y = shape.lookup (phase);

        smooth = std::clamp (smooth, 0.f, 1.f);
        if (smooth <= 0.0001f)
        {
            z = y;
            return z;
        }

        // smooth 1 = 250 ms. 0 = bypass.
        const float tau = smooth * 0.25f * sr + 1.f;
        const float coeff = 1.f - std::exp (-1.f / tau);
        z += coeff * (y - z);
        return z;
    }
};

struct VoiceMod
{
    Lfo* lfo[numLfos] {};
    float rateHz[numLfos] {};
    float smooth[numLfos] {};
    int src[destCount] {};
    float amt[destCount] {};
    float base01[destCount] {};

    float sample01 (int dest, float lfoY[numLfos]) const
    {
        const int s = src[dest];
        if (s <= 0 || s > numLfos)
            return std::clamp (base01[dest], 0.f, 1.f);
        const float bipolar = lfoY[s - 1] * 2.f - 1.f;
        return std::clamp (base01[dest] + amt[dest] * bipolar, 0.f, 1.f);
    }
};

} // namespace tew
