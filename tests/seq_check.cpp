#include "dsp/Envelope.h"
#include "dsp/EqBiquad.h"
#include "dsp/Lfo.h"
#include "dsp/Sequencer.h"
#include "parameters/Library.h"
#include "ui/VersionCompare.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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

    assert (tew::inScale (40, 0, false));
    assert (! tew::inScale (42, 0, false));

    Sequencer::Step lockedSteps[Sequencer::maxSteps];
    for (int i = 0; i < Sequencer::maxSteps; ++i)
        lockedSteps[i] = { -1, false, false };
    lockedSteps[0] = { 40, false, false }; // E, in C major
    lockedSteps[1] = { 42, false, false }; // F#, hidden when locked

    Sequencer lockedSeq;
    lockedSeq.prepare (44100.0);
    lockedSeq.loadAll (lockedSteps);
    lockedSeq.setKeyFilter (true, 0, false);

    SeqEvent lockEv[8];
    const int ln0 = lockedSeq.advance (1, 120.f, true, lockEv, 8);
    assert (ln0 == 1);
    assert (lockEv[0].on);
    assert (lockEv[0].note == 40);

    const int ln1 = lockedSeq.advance (stepSamples, 120.f, true, lockEv, 8);
    assert (ln1 == 1);
    assert (! lockEv[0].on);
    assert (lockEv[0].note == 40);
    assert (lockedSeq.getStep (1).note == 42);

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

    std::vector<tew::PatchParam> patchIn { { "cutoff", 1234.5f },
                                           { "seqBank", 2.f },
                                           { "seqPattern", 7.f },
                                           { "drive", 0.4f } };
    const auto patchXml = tew::writePatchXml (patchIn);
    assert (patchXml.find ("TEW03PATCH") != std::string::npos);
    assert (patchXml.find ("BANKS") == std::string::npos);
    assert (patchXml.find ("seqBank") == std::string::npos);
    assert (patchXml.find ("seqPattern") == std::string::npos);
    assert (patchXml.find ("cutoff") != std::string::npos);

    std::vector<tew::PatchParam> patchOut;
    assert (tew::readPatchXml (patchXml, patchOut));
    assert (patchOut.size() == 2);
    assert (patchOut[0].id == "cutoff");
    assert (std::abs (patchOut[0].value - 1234.5f) < 0.01f);
    assert (patchOut[1].id == "drive");
    assert (std::abs (patchOut[1].value - 0.4f) < 1e-5f);
    assert (! tew::readPatchXml ("<nope/>", patchOut));

    tew::Sequencer::Step banks[Sequencer::numBanks][Sequencer::patternsPerBank][Sequencer::maxSteps] {};
    for (int b = 0; b < Sequencer::numBanks; ++b)
        for (int p = 0; p < Sequencer::patternsPerBank; ++p)
            Sequencer::fillSlot (banks[b][p], b, p);
    banks[2][11][31] = { 48, true, true };

    const auto bankXml = tew::writeBankXml (banks);
    assert (bankXml.find ("TEW03BANK") != std::string::npos);
    assert (bankXml.find ("seqBank") == std::string::npos);

    tew::Sequencer::Step bankLoaded[Sequencer::numBanks][Sequencer::patternsPerBank][Sequencer::maxSteps];
    assert (tew::readBankXml (bankXml, bankLoaded));
    assert (bankLoaded[0][0][0].note == banks[0][0][0].note);
    assert (bankLoaded[2][11][31].note == 48);
    assert (bankLoaded[2][11][31].accent);
    assert (bankLoaded[2][11][31].slide);
    assert (! tew::readBankXml (patchXml, bankLoaded));

    tew::LfoShape tri;
    tri.setTriangle();
    assert (std::abs (tri.lookup (0.f) - 0.f) < 1e-5f);
    assert (std::abs (tri.lookup (0.5f) - 1.f) < 1e-5f);
    assert (std::abs (tri.lookup (1.f) - 0.f) < 1e-5f);
    assert (std::abs (tri.lookup (1.25f) - 0.5f) < 1e-5f);

    tew::Lfo lfo;
    lfo.shape.setTriangle();
    lfo.prepare (100.0);
    lfo.retrigger();
    assert (std::abs (lfo.peek() - 0.f) < 1e-5f);
    // 1 Hz at 100 Hz sr = 0.01 phase per sample. 50 samples -> phase 0.5.
    float y = 0.f;
    for (int i = 0; i < 50; ++i)
        y = lfo.process (1.f, 0.f);
    assert (std::abs (y - 1.f) < 0.02f);
    lfo.retrigger();
    assert (std::abs (lfo.peek() - 0.f) < 1e-5f);
    assert (lfo.phase < 1.0e-6f);

    tew::LfoShape shapes[tew::numLfos];
    tew::defaultLfoShapes (shapes);
    shapes[1].setSawUp();
    std::vector<tew::PatchParam> withLfo { { "cutoff", 800.f } };
    const auto lfoXml = tew::writePatchXml (withLfo, shapes);
    assert (lfoXml.find ("<LFO i=\"0\"") != std::string::npos);
    assert (lfoXml.find ("<LFO i=\"1\"") != std::string::npos);
    tew::LfoShape lfoLoaded[tew::numLfos];
    std::vector<tew::PatchParam> lfoParams;
    assert (tew::readPatchXml (lfoXml, lfoParams, lfoLoaded));
    assert (lfoLoaded[0].presetIndex() == 0);
    assert (lfoLoaded[1].presetIndex() == 2);

    int orderIn[] = { 3, 0, 8 };
    const auto packed = tew::packFxOrder (orderIn, 3);
    int orderOut[tew::fxCount];
    assert (tew::unpackFxOrder (packed, orderOut) == 3);
    assert (orderOut[0] == 3 && orderOut[1] == 0 && orderOut[2] == 8);
    char orderBuf[32];
    tew::formatFxOrder (packed, orderBuf, 32);
    const auto fxXml = tew::writePatchXml (withLfo, shapes, orderBuf);
    assert (fxXml.find ("<FX order=\"3,0,8\"") != std::string::npos);
    std::string fxOrd;
    tew::LfoShape fxShapes[tew::numLfos];
    std::vector<tew::PatchParam> fxParams;
    assert (tew::readPatchXml (fxXml, fxParams, fxShapes, &fxOrd));
    int parsed[tew::fxCount];
    assert (tew::parseFxOrder (fxOrd.c_str(), parsed) == 3);
    assert (parsed[0] == 3 && parsed[1] == 0 && parsed[2] == 8);
    assert (tew::parseFxOrder ("", parsed) == 0);
    int partial[tew::fxCount] {};
    partial[0] = 3;
    partial[1] = 0;
    assert (tew::completeFxOrder (partial, 2) == tew::fxCount);
    assert (partial[0] == 3 && partial[1] == 0 && partial[2] == 1);

    {
        tew::Envelope env;
        env.prepare (44100.0);
        env.noteOn();
        const float attack0 = env.process();
        assert (attack0 > 0.f && attack0 < 1.f);
        float last = attack0;
        for (int i = 0; i < 80; ++i)
            last = env.process();
        assert (last > 0.9f);
        env.noteOff();
        const float rel0 = env.process();
        assert (rel0 > 0.5f);
        for (int i = 0; i < (int) (0.012f * 44100.f); ++i)
            last = env.process();
        assert (last < 0.01f);
    }

    {
        const auto peak = tew::eqBiquad (tew::eqTypePeak, 44100.f, 1000.f, 0.7f, 6.f);
        tew::Biquad one[1] { peak };
        assert (tew::eqMagnitude (one, 1, 44100.f, 1000.f) > 1.5f);
        const auto off = tew::eqBiquad (tew::eqTypeOff, 44100.f, 1000.f, 0.7f, 6.f);
        tew::Biquad flat[1] { off };
        const float mag = tew::eqMagnitude (flat, 1, 44100.f, 1000.f);
        assert (mag > 0.99f && mag < 1.01f);
    }

    assert (! tew::versionNewer ("0.0.4", "0.0.4"));
    assert (! tew::versionNewer ("v0.0.4", "0.0.4"));
    assert (tew::versionNewer ("0.0.5", "0.0.4"));
    assert (tew::versionNewer ("v0.1.0", "0.0.9"));
    assert (! tew::versionNewer ("0.0.3", "0.0.4"));
    assert (! tew::versionNewer ("nope", "0.0.4"));
    assert (tew::versionNewer ("0.0.4.1", "0.0.4"));
    assert (tew::newestNewerTag ({ "v0.0.3", "0.0.5", "v0.1.0" }, "0.0.4") == "v0.1.0");
    assert (tew::newestNewerTag ({ "v0.0.4", "v0.0.3" }, "0.0.4").empty());

    std::puts ("seq_check ok");
    return 0;
}
