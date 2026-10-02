#pragma once

#include "Envelope.h"
#include "Oscillator.h"

#include <algorithm>
#include <cmath>

namespace tew
{

// One note. Glide ramps pitch in semitones. Accent is a level bump.
struct Voice
{
    void prepare (double sr)
    {
        sampleRate = sr;
        osc.prepare (sr);
        env.prepare (sr);
    }

    void setDecaySeconds (float seconds) { env.setDecaySeconds (seconds); }
    void setGain (float g) { gain = std::clamp (g, 0.f, 1.f); }
    void setAccentAmount (float amount) { accentAmount = std::clamp (amount, 0.f, 1.f); }

    void setGlideSeconds (float seconds)
    {
        glideSeconds = std::max (0.f, seconds);
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
            out[i] = osc.process() * env.process() * accentGain * gain;
        }
    }

private:
    static float midiToHz (float note)
    {
        return 440.f * std::pow (2.f, (note - 69.f) / 12.f);
    }

    Oscillator osc;
    Envelope env;
    double sampleRate = 44100.0;
    float currentMidi = 36.f;
    float targetMidi = 36.f;
    float glideStep = 0.f;
    float glideSeconds = 0.f;
    float bendSemis = 0.f;
    float accentAmount = 0.f;
    float accentGain = 1.f;
    float gain = 0.25f;
    int glideSamples = 0;
    int heldNote = -1;
    bool hasPitch = false;
};

} // namespace tew
