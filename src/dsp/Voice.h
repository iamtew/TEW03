#pragma once

#include "DiodeLadderFilter.h"
#include "Envelope.h"
#include "Oscillator.h"

#include <algorithm>
#include <cmath>

namespace tew
{

// One note. Osc -> tanh drive -> diode ladder -> amp env.
// The same decay envelope opens the filter. Accent bumps level, cutoff, and resonance.
struct Voice
{
    void prepare (double sr)
    {
        sampleRate = sr;
        osc.prepare (sr);
        env.prepare (sr);
        filter.prepare (sr);
    }

    void setDecaySeconds (float seconds) { env.setDecaySeconds (seconds); }
    void setGain (float g) { gain = std::clamp (g, 0.f, 1.f); }
    void setAccentAmount (float amount) { accentAmount = std::clamp (amount, 0.f, 1.f); }

    void setGlideSeconds (float seconds)
    {
        glideSeconds = std::max (0.f, seconds);
    }

    void setFilter (float cutoffHz, float resonanceAmount, float driveAmount, bool classic)
    {
        cutoff = cutoffHz;
        resonance = std::clamp (resonanceAmount, 0.f, 1.f);
        drive = classic ? 0.f : std::clamp (driveAmount, 0.f, 1.f);
        // Classic 303: keep saw, kill extra drive, stop short of screaming resonance.
        if (classic)
            resonance = std::min (resonance, 0.65f);
    }

    // value is the 14-bit MIDI pitch wheel. ±2 semitones, centre 8192.
    void setPitchBend (int value)
    {
        bendSemis = ((float) value - 8192.f) / 8192.f * 2.f;
    }

    void noteOn (int note, bool accent)
    {
        const float target = (float) note;

        // hasPitch survives note-off, so a retrigger still glides from the last pitch.
        if (hasPitch && glideSeconds > 0.f)
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
        env.noteOn();
    }

    void noteOff (int note)
    {
        if (note != heldNote)
            return;

        env.noteOff();
    }

    void render (float* out, int numSamples)
    {
        // drive 0 is unity. drive 1 is a hard tanh shove.
        const float driveGain = 1.f + drive * 8.f;
        const float resNow = std::clamp (resonance + (accented ? 0.25f * accentAmount : 0.f), 0.f, 1.f);

        for (int i = 0; i < numSamples; ++i)
        {
            if (glideSamples > 0)
            {
                currentMidi += glideStep;
                if (--glideSamples == 0)
                    currentMidi = targetMidi;
            }

            const float note = currentMidi + bendSemis;
            osc.setFrequency (midiToHz (note));

            const float e = env.process();
            // Envelope lifts cutoff up to four octaves. Accent adds two more.
            const float octaves = 4.f + (accented ? 2.f * accentAmount : 0.f);
            const float fc = cutoff * std::pow (2.f, e * octaves);
            filter.set (fc, resNow);

            float s = osc.process();
            s = std::tanh (s * driveGain);
            s = filter.process (s);
            out[i] = s * e * accentGain * gain;
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
    float cutoff = 800.f;
    float resonance = 0.3f;
    float drive = 0.f;
    int glideSamples = 0;
    int heldNote = -1;
    bool hasPitch = false;
    bool accented = false;
};

} // namespace tew
