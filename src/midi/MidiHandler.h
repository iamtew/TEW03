#pragma once

#include "../dsp/Voice.h"

#include <JuceHeader.h>

namespace tew
{

// Notes and pitch bend from the host. No allocation: walks the buffer in place.
struct MidiHandler
{
    static void applyPitchBend (const juce::MidiBuffer& midi, Voice& voice)
    {
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isPitchWheel())
                voice.setPitchBend (msg.getPitchWheelValue());
        }
    }

    // Last note wins. Note-off only releases the note we are holding.
    static void applyNotes (const juce::MidiBuffer& midi, Voice& voice)
    {
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn())
                voice.noteOn (msg.getNoteNumber(), msg.getVelocity() >= 110);
            else if (msg.isNoteOff())
                voice.noteOff (msg.getNoteNumber());
        }
    }
};

} // namespace tew
