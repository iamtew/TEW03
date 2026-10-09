#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace tew
{

inline bool inScale (int note, int root, bool minor)
{
    const int pc = ((note % 12) - root + 12) % 12;
    if (minor)
        return pc == 0 || pc == 2 || pc == 3 || pc == 5 || pc == 7 || pc == 8 || pc == 10;
    return pc == 0 || pc == 2 || pc == 4 || pc == 5 || pc == 7 || pc == 9 || pc == 11;
}

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
    static constexpr int numSteps = 16;  // default loop length (16ths)
    static constexpr int maxSteps = 32;  // 2x splits each 16th into two 32nds
    static constexpr int restCode = 255; // packed note byte for a rest
    static constexpr int numBanks = 3;
    static constexpr int patternsPerBank = 12;
    static constexpr int keyMapBase = 24; // C1; E1 = bank 1 pattern 5

    static bool mapKeyToSlot (int note, int& bank, int& pat)
    {
        const int idx = note - keyMapBase;
        if (idx < 0 || idx >= numBanks * patternsPerBank)
            return false;
        bank = idx / patternsPerBank;
        pat = idx % patternsPerBank;
        return true;
    }

    struct Step
    {
        int note = 36; // < 0 is a rest
        bool accent = false;
        bool slide = false;
        bool superSlide = false; // slide that holds through rests
    };

    static uint32_t pack (Step s)
    {
        if (s.superSlide)
            s.slide = true;
        const uint32_t note = s.note < 0 ? (uint32_t) restCode
                                         : (uint32_t) std::clamp (s.note, 0, 127);
        return note
             | (s.accent     ? 0x100u : 0u)
             | (s.slide      ? 0x200u : 0u)
             | (s.superSlide ? 0x400u : 0u);
    }

    static Step unpack (uint32_t bits)
    {
        const int noteByte = (int) (bits & 0xffu);
        const bool super = (bits & 0x400u) != 0;
        return { noteByte == restCode ? -1 : noteByte,
                 (bits & 0x100u) != 0,
                 (bits & 0x200u) != 0 || super,
                 super };
    }

    static void fillDefault (Step out[maxSteps]) { fillSlot (out, 0, 0); }

    // 12 motifs × 3 roots (C2 / F2 / C3). Offsets from the bank root; -1 is a rest.
    static void fillSlot (Step out[maxSteps], int bank, int pat)
    {
        bank = std::clamp (bank, 0, numBanks - 1);
        pat = std::clamp (pat, 0, patternsPerBank - 1);

        static constexpr int8_t kMotif[patternsPerBank][numSteps] = {
            {  0,  0,  3,  0,  7, -1,  5,  3,  0, 12, 10,  7,  5, -1,  3,  0 },
            {  0, 12,  0, 12,  0, -1,  7, 12,  0, 12, 10,  7, -1,  5,  3,  0 },
            {  0, -1,  0, -1,  7, -1,  5, -1,  0, -1, 12, -1,  7, -1,  3,  0 },
            {  0,  1,  2,  3,  5,  7,  8, 10, 12, 10,  8,  7,  5,  3,  2,  0 },
            { 12, 10,  7,  5,  3,  0, -1,  0, 12,  7,  5,  3,  0, -1,  7,  0 },
            {  0,  7,  0,  7, 12,  7,  0, -1,  0,  7, 12,  7,  5,  7,  3,  0 },
            {  0,  0,  3,  3,  7,  7, 12, 12, 10, 10,  7,  7,  5,  5,  0,  0 },
            {  0,  3,  5,  7,  3, -1,  8,  7,  0, 12,  8,  7,  5, -1,  3,  0 },
            {  0,  7, -1, 12,  0,  7, -1, 12,  3,  7, -1, 10,  5,  7, -1,  0 },
            {  0,  0,  0,  3,  0,  0,  7,  0,  0,  0, 12,  0,  5,  0,  3,  0 },
            { 12,  7, 12, 10, 12, -1, 10,  7, 12,  7, 10,  5,  7, -1,  3, 12 },
            {  0, -1, -1,  7,  0, -1, 12, 10,  7, -1,  5,  3,  0, -1, -1,  0 },
        };
        static constexpr uint16_t kAccent[patternsPerBank] = {
            0x0101, 0x1111, 0x0055, 0x0001, 0x0101, 0x0145,
            0x1111, 0x0081, 0x00aa, 0x0041, 0x0111, 0x0049
        };
        static constexpr uint16_t kSlide[patternsPerBank] = {
            0x0408, 0x0002, 0x0000, 0x1554, 0x0004, 0x0022,
            0x5555, 0x0408, 0x0000, 0x0008, 0x0444, 0x00c0
        };
        static constexpr int kRoot[numBanks] = { 36, 41, 48 }; // C2, F2, C3

        const int root = kRoot[bank];
        const uint16_t acc = kAccent[pat];
        const uint16_t sld = kSlide[pat];
        for (int i = 0; i < numSteps; ++i)
        {
            const int off = kMotif[pat][i];
            out[i] = { off < 0 ? -1 : std::clamp (root + off, 24, 60),
                       (acc & (1u << i)) != 0,
                       (sld & (1u << i)) != 0 };
        }
        for (int i = numSteps; i < maxSteps; ++i)
            out[i] = { -1, false, false };
    }

    Sequencer()
    {
        Step baked[maxSteps];
        fillDefault (baked);
        loadAll (baked);
    }

    void prepare (double sr) { sampleRate = sr; }

    void setStep (int i, Step s)
    {
        if (i < 0 || i >= maxSteps)
            return;
        packed[i].store (pack (s), std::memory_order_relaxed);
    }

    Step getStep (int i) const
    {
        if (i < 0 || i >= maxSteps)
            return {};
        return unpack (packed[i].load (std::memory_order_relaxed));
    }

    void loadAll (const Step in[maxSteps])
    {
        for (int i = 0; i < maxSteps; ++i)
            packed[i].store (pack (in[i]), std::memory_order_relaxed);
    }

    void loadPacked (const std::atomic<uint32_t>* src)
    {
        for (int i = 0; i < maxSteps; ++i)
            packed[i].store (src[i].load (std::memory_order_relaxed), std::memory_order_relaxed);
    }

    void storePacked (std::atomic<uint32_t>* dest) const
    {
        for (int i = 0; i < maxSteps; ++i)
            dest[i].store (packed[i].load (std::memory_order_relaxed), std::memory_order_relaxed);
    }

    void setKeyFilter (bool lock, int root, bool minor)
    {
        keyLock.store (lock, std::memory_order_relaxed);
        keyRoot.store (root, std::memory_order_relaxed);
        keyMinor.store (minor, std::memory_order_relaxed);
    }

    // Length only. Use reshapeTo when 2x toggles so the grid splits instead of appending.
    void setLength (int n)
    {
        const int len = n >= maxSteps ? maxSteps : numSteps;
        lengthSteps.store (len, std::memory_order_relaxed);
        if (step >= len)
            step %= len;
    }

    // 16→32: each cell becomes two 32nds of the same note (tied). 32→16: take the pair.
    void reshapeTo (int n)
    {
        const int want = n >= maxSteps ? maxSteps : numSteps;
        int have = lengthSteps.load (std::memory_order_relaxed);
        if (want == have)
            return;
        if (! lengthSteps.compare_exchange_strong (have, want, std::memory_order_relaxed))
            return;

        Step cur[maxSteps];
        for (int i = 0; i < maxSteps; ++i)
            cur[i] = unpack (packed[i].load (std::memory_order_relaxed));

        Step out[maxSteps];
        if (want == maxSteps)
        {
            for (int i = 0; i < numSteps; ++i)
            {
                out[2 * i] = cur[i];
                out[2 * i + 1] = cur[i];
                if (out[2 * i].note >= 0)
                    out[2 * i].slide = true;
            }
        }
        else
        {
            for (int i = 0; i < numSteps; ++i)
            {
                out[i] = cur[2 * i];
                if (out[i].note >= 0)
                {
                    out[i].slide = cur[2 * i + 1].slide;
                    out[i].superSlide = cur[2 * i + 1].superSlide;
                    if (out[i].superSlide)
                        out[i].slide = true;
                }
            }
            for (int i = numSteps; i < maxSteps; ++i)
                out[i] = { -1, false, false, false };
        }

        loadAll (out);
        if (step >= want)
            step %= want;
    }

    int length() const { return lengthSteps.load (std::memory_order_relaxed); }
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
            pendingSuper = false;
        }

        // Knob range is 40-300. Host tempos can sit outside that, so only reject nonsense.
        bpm = std::clamp (bpm, 1.f, 999.f);
        const int len = lengthSteps.load (std::memory_order_relaxed);
        // 16th grid, or 32nds when 2x. Floor of 1 sample so a bad rate cannot spin.
        const double perBeat = len >= maxSteps ? 8.0 : 4.0;
        double stepSamples = sampleRate * 60.0 / (double) bpm / perBeat;
        if (stepSamples < 1.0)
            stepSamples = 1.0;

        int produced = 0;
        int consumed = 0;

        while (consumed < numSamples)
        {
            if (nextEdge <= clock)
            {
                const Step s = unpack (packed[step].load (std::memory_order_relaxed));
                const bool rest = s.note < 0
                    || (keyLock.load (std::memory_order_relaxed)
                        && ! inScale (s.note,
                                      keyRoot.load (std::memory_order_relaxed),
                                      keyMinor.load (std::memory_order_relaxed)));
                const bool slideIn = pendingSlide && gate;
                const bool holdRest = rest && pendingSuper && gate;
                const int need = rest ? (holdRest ? 0 : (gate ? 1 : 0))
                                      : (gate ? 2 : 1);

                if (produced + need > maxEvents)
                {
                    clock += (double) (numSamples - consumed);
                    break;
                }

                playheadIndex.store (step, std::memory_order_relaxed);

                if (rest)
                {
                    if (! holdRest)
                    {
                        if (gate)
                            out[produced++] = { consumed, lastNote, false, false, false };
                        gate = false;
                        pendingSlide = false;
                        pendingSuper = false;
                    }
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
                    pendingSuper = s.superSlide;
                }

                step = (step + 1) % len;
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
        pendingSuper = false;
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

    std::atomic<uint32_t> packed[maxSteps] {};
    std::atomic<int> lengthSteps { numSteps };
    std::atomic<int> playheadIndex { -1 };
    std::atomic<bool> keyLock { false };
    std::atomic<int> keyRoot { 0 };
    std::atomic<bool> keyMinor { false };
    double sampleRate = 44100.0;
    double clock = 0.0;
    double nextEdge = 0.0;
    int step = 0;
    int lastNote = 36;
    bool running = false;
    bool gate = false;
    bool pendingSlide = false;
    bool pendingSuper = false;
};

} // namespace tew
