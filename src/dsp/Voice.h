#pragma once

#include "DiodeLadderFilter.h"
#include "Envelope.h"
#include "Lfo.h"
#include "Oscillator.h"

#include <algorithm>
#include <cmath>

namespace tew
{

// One note. Osc -> Drive gain -> diode ladder -> Nasty mix -> amp env -> tanh.
// The same decay envelope opens the filter. Accent bumps level, cutoff, and resonance.
struct Voice
{
    void prepare (double sr)
    {
        sampleRate = sr;
        osc.prepare (sr);
        env.prepare (sr);
        filter.prepare (sr);
        // ~10 ms so the header fader does not zipper.
        gainSlew = 1.f / std::max (1.f, 0.01f * (float) sr);
    }

    void setDecaySeconds (float seconds) { env.setDecaySeconds (seconds); }
    void setGain (float g) { gainTarget = std::clamp (g, 0.f, 1.f); }
    void setAccentAmount (float amount) { accentAmount = std::clamp (amount, 0.f, 1.f); }

    void setGlideSeconds (float seconds)
    {
        glideSeconds = std::max (0.f, seconds);
    }

    void setSquare (bool sq)
    {
        wantSquare = sq;
        osc.setSquare (wantSquare);
    }

    void setFilter (float cutoffHz, float resonanceAmount, float driveAmount,
                    float envModAmount, float nastyAmount)
    {
        cutoff = cutoffHz;
        resonance = std::clamp (resonanceAmount, 0.f, 1.f);
        drive = std::clamp (driveAmount, 0.f, 1.f);
        envMod = std::clamp (envModAmount, 0.f, 1.f);
        nasty = std::clamp (nastyAmount, 0.f, 1.f);
    }

    // value is the 14-bit MIDI pitch wheel. ±2 semitones, centre 8192.
    void setPitchBend (int value)
    {
        bendSemis = ((float) value - 8192.f) / 8192.f * 2.f;
    }

    void noteOn (int note, bool accent, bool slide = false)
    {
        const float target = (float) note;

        if (slide)
        {
            // Slide always ramps. Glide knob of 0 still gets a short default so the tie is heard.
            const float seconds = glideSeconds > 0.f ? glideSeconds : 0.05f;
            glideSamples = std::max (1, (int) std::lround (seconds * sampleRate));
            glideStep = (target - currentMidi) / (float) glideSamples;
        }
        else if (hasPitch && glideSeconds > 0.f)
        {
            glideSamples = std::max (1, (int) std::lround (glideSeconds * sampleRate));
            glideStep = (target - currentMidi) / (float) glideSamples;
        }
        else
        {
            currentMidi = target;
            glideSamples = 0;
        }

        targetMidi = target;
        accented = accent;
        accentGain = accent ? (1.f + accentAmount) : 1.f;
        heldNote = note;
        hasPitch = true;

        if (! slide)
            env.noteOn();
    }

    void noteOff (int note)
    {
        if (note != heldNote)
            return;

        env.noteOff();
    }

    void render (float* out, int numSamples, const VoiceMod* mod = nullptr)
    {
        auto sample01 = [&] (int dest, float lfoY[numLfos], float fallback01) -> float
        {
            if (mod == nullptr || mod->src[dest] <= 0)
                return fallback01;
            return mod->sample01 (dest, lfoY);
        };

        for (int i = 0; i < numSamples; ++i)
        {
            float lfoY[numLfos] { 0.5f, 0.5f };
            if (mod != nullptr)
            {
                for (int l = 0; l < numLfos; ++l)
                    if (mod->lfo[l] != nullptr)
                        lfoY[l] = mod->lfo[l]->process (mod->rateHz[l], mod->smooth[l]);
            }

            const float cutoffNow = destFrom01 (destCutoff,
                sample01 (destCutoff, lfoY, destTo01 (destCutoff, cutoff)));
            const float resAmt = sample01 (destResonance, lfoY, resonance);
            const float driveAmt = sample01 (destDrive, lfoY, drive);
            const float nastyAmt = sample01 (destNasty, lfoY, nasty);
            const float envAmt = sample01 (destEnvMod, lfoY, envMod);
            if (gain < gainTarget)
                gain = std::min (gainTarget, gain + gainSlew);
            else if (gain > gainTarget)
                gain = std::max (gainTarget, gain - gainSlew);
            const float gainNow = sample01 (destVolume, lfoY, gain);

            // drive 0 ≈ 0.35 into the ladder (mostly clean). drive 1 ≈ 8.35 (diodes slam).
            const float inGain = 0.35f + driveAmt * 8.f;
            const float resNow = std::clamp (resAmt + (accented ? 0.25f * accentAmount : 0.f), 0.f, 1.f);

            if (glideSamples > 0)
            {
                currentMidi += glideStep;
                if (--glideSamples == 0)
                    currentMidi = targetMidi;
            }

            const float note = currentMidi + bendSemis;
            osc.setFrequency (midiToHz (note));

            const float e = env.process();
            // envMod 1 = four octaves of cutoff sweep. Accent still adds extra lift.
            const float octaves = envAmt * 4.f + (accented ? 2.f * accentAmount : 0.f);
            const float fc = cutoffNow * std::pow (2.f, e * octaves);
            filter.set (fc, resNow);

            float s = osc.process() * inGain;
            s = filter.process (s);
            // nasty 0 = filter output unchanged. nasty 1 = tanh(s * 5) on the squelch peak.
            s += nastyAmt * (std::tanh (s * (1.f + nastyAmt * 4.f)) - s);
            s *= e * accentGain * gainNow;
            out[i] = std::tanh (s);
        }
    }

private:
    static float midiToHz (float note)
    {
        return 440.f * std::pow (2.f, (note - 69.f) / 12.f);
    }

    Oscillator osc;
    Envelope env;
    DiodeLadderFilter filter;
    double sampleRate = 44100.0;
    float currentMidi = 36.f;
    float targetMidi = 36.f;
    float glideStep = 0.f;
    float glideSeconds = 0.f;
    float bendSemis = 0.f;
    float accentAmount = 0.f;
    float accentGain = 1.f;
    float gain = 0.25f;
    float gainTarget = 0.25f;
    float gainSlew = 1.f / 441.f;
    float cutoff = 800.f;
    float resonance = 0.3f;
    float drive = 0.f;
    float nasty = 0.f;
    float envMod = 0.7f;
    int glideSamples = 0;
    int heldNote = -1;
    bool hasPitch = false;
    bool accented = false;
    bool wantSquare = false;
};

} // namespace tew
