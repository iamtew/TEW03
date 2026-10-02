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
};

// 16th-note grid, internal tempo. Fixed 16-step pattern.
// ponytail: one repeating C2, accent on the downbeats. Per-step pitch/slide when the pattern UI exists.
struct Sequencer
{
    static constexpr int numSteps = 16;

    struct Step
    {
        int note = 36;
        bool accent = false;
        bool slide = false; // stored, not applied yet
    };

    Sequencer()
    {
        for (int i = 0; i < numSteps; ++i)
        {
            steps[i].note = 36;
            steps[i].accent = (i % 4) == 0;
        }
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
        }

        bpm = std::clamp (bpm, 40.f, 300.f);
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
                const int need = gate ? 2 : 1;
                if (produced + need > maxEvents)
                {
                    clock += (double) (numSamples - consumed);
                    break;
                }

                if (gate)
                    out[produced++] = { consumed, lastNote, false, false };

                const Step& s = steps[step];
                out[produced++] = { consumed, s.note, true, s.accent };
                lastNote = s.note;
                gate = true;
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

        if (! gate || maxEvents < 1)
        {
            gate = false;
            return 0;
        }

        gate = false;
        out[0] = { 0, lastNote, false, false };
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
};

} // namespace tew
