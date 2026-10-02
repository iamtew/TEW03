#pragma once

#include <algorithm>
#include <cmath>

namespace tew
{

struct SeqEvent
{
    int offset = 0;
    int note = 36;
    bool on = false;
    bool accent = false;
    bool slide = false;
};

// 16th-note grid. Slide on a step ties into the next (no gap, pitch ramps).
// Rest is note < 0. Pattern is baked until a step UI exists.
struct Sequencer
{
    static constexpr int numSteps = 16;

    struct Step
    {
        int note = 36; // < 0 is a rest
        bool accent = false;
        bool slide = false;
    };

    Sequencer()
    {
        // C2 acid line: accents, two slides, two rests.
        const Step baked[numSteps] = {
            { 36, true,  false },
            { 36, false, false },
            { 39, false, false },
            { 36, false, true  },
            { 43, false, false },
            { -1, false, false },
            { 41, false, false },
            { 39, false, false },
            { 36, true,  false },
            { 48, false, false },
            { 46, false, true  },
            { 43, false, false },
            { 41, false, false },
            { -1, false, false },
            { 39, false, false },
            { 36, false, false },
        };

        for (int i = 0; i < numSteps; ++i)
            steps[i] = baked[i];
    }

    void prepare (double sr) { sampleRate = sr; }

    // Fills out[] with note edges inside this block. Returns how many.
    int advance (int numSamples, float bpm, bool playing, SeqEvent* out, int maxEvents)
    {
        if (! playing)
            return stop (out, maxEvents);

        if (! running)
        {
            running = true;
            step = 0;
            clock = 0.0;
            nextEdge = 0.0;
            pendingSlide = false;
        }

        // Knob range is 40-300. Host tempos can sit outside that, so only reject nonsense.
        bpm = std::clamp (bpm, 1.f, 999.f);
        // 16th note. 4 steps per beat. Floor of 1 sample so a bad rate cannot spin.
        double stepSamples = sampleRate * 60.0 / (double) bpm / 4.0;
        if (stepSamples < 1.0)
            stepSamples = 1.0;

        int produced = 0;
        int consumed = 0;

        while (consumed < numSamples)
        {
            if (nextEdge <= clock)
            {
                const Step& s = steps[step];
                const bool rest = s.note < 0;
                const bool slideIn = pendingSlide && gate;
                const int need = rest ? (gate ? 1 : 0)
                                      : (gate ? 2 : 1);

                if (produced + need > maxEvents)
                {
                    clock += (double) (numSamples - consumed);
                    break;
                }

                if (rest)
                {
                    if (gate)
                        out[produced++] = { consumed, lastNote, false, false, false };
                    gate = false;
                    pendingSlide = false;
                }
                else
                {
                    if (gate && ! slideIn)
                        out[produced++] = { consumed, lastNote, false, false, false };

                    out[produced++] = { consumed, s.note, true, s.accent, slideIn };

                    // Overlap: new note-on first, then old note-off. Voice ignores the off.
                    if (slideIn)
                        out[produced++] = { consumed, lastNote, false, false, false };

                    lastNote = s.note;
                    gate = true;
                    pendingSlide = s.slide;
                }

                step = (step + 1) % numSteps;
                nextEdge += stepSamples;
                continue;
            }

            const int room = numSamples - consumed;
            int chunk = (int) std::ceil (nextEdge - clock);
            if (chunk < 1)
                chunk = 1;
            chunk = std::min (chunk, room);

            clock += chunk;
            consumed += chunk;
        }

        return produced;
    }

private:
    int stop (SeqEvent* out, int maxEvents)
    {
        running = false;
        clock = 0.0;
        nextEdge = 0.0;
        pendingSlide = false;

        if (! gate || maxEvents < 1)
        {
            gate = false;
            return 0;
        }

        gate = false;
        out[0] = { 0, lastNote, false, false, false };
        return 1;
    }

    Step steps[numSteps] {};
    double sampleRate = 44100.0;
    double clock = 0.0;
    double nextEdge = 0.0;
    int step = 0;
    int lastNote = 36;
    bool running = false;
    bool gate = false;
    bool pendingSlide = false;
};

} // namespace tew
