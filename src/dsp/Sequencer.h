#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

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
// Rest is note < 0. packed[] is the audio-thread copy; UI writes via setStep.
struct Sequencer
{
    static constexpr int numSteps = 16;
    static constexpr int restCode = 255; // packed note byte for a rest

    struct Step
    {
        int note = 36; // < 0 is a rest
        bool accent = false;
        bool slide = false;
    };

    static uint32_t pack (Step s)
    {
        const uint32_t note = s.note < 0 ? (uint32_t) restCode
                                         : (uint32_t) std::clamp (s.note, 0, 127);
        return note
             | (s.accent ? 0x100u : 0u)
             | (s.slide  ? 0x200u : 0u);
    }

    static Step unpack (uint32_t bits)
    {
        const int noteByte = (int) (bits & 0xffu);
        return { noteByte == restCode ? -1 : noteByte,
                 (bits & 0x100u) != 0,
                 (bits & 0x200u) != 0 };
    }

    static void fillDefault (Step out[numSteps])
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
            out[i] = baked[i];
    }

    Sequencer()
    {
        Step baked[numSteps];
        fillDefault (baked);
        loadAll (baked);
    }

    void prepare (double sr) { sampleRate = sr; }

    void setStep (int i, Step s)
    {
        if (i < 0 || i >= numSteps)
            return;
        packed[i].store (pack (s), std::memory_order_relaxed);
    }

    Step getStep (int i) const
    {
        if (i < 0 || i >= numSteps)
            return {};
        return unpack (packed[i].load (std::memory_order_relaxed));
    }

    void loadAll (const Step in[numSteps])
    {
        for (int i = 0; i < numSteps; ++i)
            packed[i].store (pack (in[i]), std::memory_order_relaxed);
    }

    int playhead() const { return playheadIndex.load (std::memory_order_relaxed); }

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
                const Step s = unpack (packed[step].load (std::memory_order_relaxed));
                const bool rest = s.note < 0;
                const bool slideIn = pendingSlide && gate;
                const int need = rest ? (gate ? 1 : 0)
                                      : (gate ? 2 : 1);

                if (produced + need > maxEvents)
                {
                    clock += (double) (numSamples - consumed);
                    break;
                }

                playheadIndex.store (step, std::memory_order_relaxed);

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
        playheadIndex.store (-1, std::memory_order_relaxed);

        if (! gate || maxEvents < 1)
        {
            gate = false;
            return 0;
        }

        gate = false;
        out[0] = { 0, lastNote, false, false, false };
        return 1;
    }

    std::atomic<uint32_t> packed[numSteps] {};
    std::atomic<int> playheadIndex { -1 };
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
