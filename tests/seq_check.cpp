#include "dsp/Sequencer.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using tew::Sequencer;
    using tew::SeqEvent;

    const auto packedRest = Sequencer::pack ({ -1, true, true });
    const auto rest = Sequencer::unpack (packedRest);
    assert (rest.note < 0);
    assert (rest.accent);
    assert (rest.slide);

    const auto packedNote = Sequencer::pack ({ 40, false, false });
    const auto note = Sequencer::unpack (packedNote);
    assert (note.note == 40);
    assert (! note.accent);
    assert (! note.slide);

    Sequencer::Step steps[Sequencer::maxSteps];
    for (int i = 0; i < Sequencer::maxSteps; ++i)
        steps[i] = { -1, false, false };
    steps[0] = { 36, false, true };
    steps[1] = { 40, true, false };

    Sequencer seq;
    seq.prepare (44100.0);
    seq.loadAll (steps);
    assert (seq.playhead() == -1);

    SeqEvent ev[8];
    const int n0 = seq.advance (1, 120.f, true, ev, 8);
    assert (n0 == 1);
    assert (ev[0].on);
    assert (ev[0].note == 36);
    assert (! ev[0].slide);
    assert (ev[0].offset == 0);
    assert (seq.playhead() == 0);

    // 16th at 120 BPM, 44100 Hz.
    const int stepSamples = (int) std::ceil (44100.0 * 60.0 / 120.0 / 4.0);
    const int n1 = seq.advance (stepSamples, 120.f, true, ev, 8);
    assert (n1 == 2);
    assert (ev[0].on);
    assert (ev[0].note == 40);
    assert (ev[0].slide);
    assert (ev[0].accent);
    assert (! ev[1].on);
    assert (ev[1].note == 36);
    assert (seq.playhead() == 1);

    int bank = -1, pat = -1;
    assert (Sequencer::mapKeyToSlot (24, bank, pat) && bank == 0 && pat == 0);  // C1
    assert (Sequencer::mapKeyToSlot (28, bank, pat) && bank == 0 && pat == 4);  // E1 = bank 1 pat 5
    assert (Sequencer::mapKeyToSlot (36, bank, pat) && bank == 1 && pat == 0);  // C2
    assert (Sequencer::mapKeyToSlot (59, bank, pat) && bank == 2 && pat == 11); // B3
    assert (! Sequencer::mapKeyToSlot (23, bank, pat));
    assert (! Sequencer::mapKeyToSlot (60, bank, pat));

    Sequencer packedCopy;
    packedCopy.prepare (44100.0);
    packedCopy.loadAll (steps);
    std::atomic<uint32_t> slot[Sequencer::maxSteps];
    packedCopy.storePacked (slot);
    Sequencer loaded;
    loaded.loadPacked (slot);
    assert (loaded.getStep (0).note == 36);
    assert (loaded.getStep (0).slide);
    assert (loaded.getStep (1).note == 40);
    assert (loaded.getStep (1).accent);

    tew::Sequencer::Step a[Sequencer::maxSteps];
    tew::Sequencer::Step b[Sequencer::maxSteps];
    Sequencer::fillDefault (a);
    assert (a[0].note == 36);
    assert (a[0].accent);
    assert (a[3].slide);
    Sequencer::fillSlot (b, 0, 1);
    assert (b[0].note == 36);
    assert (b[1].note == 48);
    Sequencer::fillSlot (b, 1, 0);
    assert (b[0].note == 41);
    Sequencer::fillSlot (b, 2, 0);
    assert (b[0].note == 48);
    Sequencer::fillSlot (a, 0, 0);
    Sequencer::fillSlot (b, 0, 0);
    for (int p = 1; p < Sequencer::patternsPerBank; ++p)
    {
        Sequencer::fillSlot (b, 0, p);
        bool same = true;
        for (int i = 0; i < Sequencer::numSteps; ++i)
            if (a[i].note != b[i].note)
                same = false;
        assert (! same);
    }

    const int nStop = seq.advance (1, 120.f, false, ev, 8);
    assert (nStop == 1);
    assert (! ev[0].on);
    assert (ev[0].note == 40);
    assert (seq.playhead() == -1);

    Sequencer wrap16;
    wrap16.prepare (44100.0);
    wrap16.loadAll (steps);
    wrap16.advance (1, 120.f, true, ev, 8);
    for (int i = 0; i < 15; ++i)
        wrap16.advance (stepSamples, 120.f, true, ev, 8);
    assert (wrap16.playhead() == 15);
    wrap16.advance (stepSamples, 120.f, true, ev, 8);
    assert (wrap16.playhead() == 0);

    Sequencer split;
    split.prepare (44100.0);
    split.loadAll (steps);
    split.reshapeTo (Sequencer::maxSteps);
    assert (split.length() == Sequencer::maxSteps);
    assert (split.getStep (0).note == 36);
    assert (split.getStep (0).slide);
    assert (split.getStep (1).note == 36);
    assert (split.getStep (1).slide);
    assert (split.getStep (2).note == 40);
    assert (split.getStep (2).slide);
    assert (split.getStep (3).note == 40);
    assert (! split.getStep (3).slide);
    split.reshapeTo (Sequencer::numSteps);
    assert (split.length() == Sequencer::numSteps);
    assert (split.getStep (0).note == 36);
    assert (split.getStep (0).slide);
    assert (split.getStep (1).note == 40);
    assert (! split.getStep (1).slide);

    Sequencer wrap32;
    wrap32.prepare (44100.0);
    wrap32.loadAll (steps);
    wrap32.reshapeTo (Sequencer::maxSteps);
    wrap32.advance (1, 120.f, true, ev, 8);
    assert (wrap32.playhead() == 0);
    const int step32 = (int) std::ceil (44100.0 * 60.0 / 120.0 / 8.0);
    for (int i = 0; i < 16; ++i)
        wrap32.advance (step32, 120.f, true, ev, 8);
    assert (wrap32.playhead() == 16);

    std::puts ("seq_check ok");
    return 0;
}
