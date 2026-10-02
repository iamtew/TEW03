# TEW03 – Cursor Rules

You are building a VST3 software synthesizer focused on aggressive TB-303-style acid lines (“mean lines”).
Not a pure clone. Main purpose = classic resonant squelch + extra nastiness.
Must also support a “Classic 303” mode.

An internal step sequencer is the primary performance source.
The sequencer drives the internal voice and also emits MIDI Output
so patterns can be recorded into the DAW or used to sequence other plugins.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17/20
- Real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for all parameters
- Just + justfile for commands

## Coding style
- Separate Oscillator, Filter, Envelope, Voice, SynthEngine, Sequencer
- Prefer composition
- Header-only DSP where practical
- Atomic / smoothed parameter reads on the audio thread
- Comment non-obvious DSP constants

## Project layout
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/
    Oscillator.h
    DiodeLadderFilter.h   # or Moog-style, but aim for diode character
    Envelope.h
    Voice.h
    SynthEngine.h
    Sequencer.h           # step sequencer (pattern, slide, accent per step)
  midi/
    MidiHandler.h         # incoming MIDI + outgoing MIDI generation
  parameters/
    ParameterIDs.h
    ParameterLayout.h
docs/
  PRD.md
  ARCHITECTURE.md
  DSP_NOTES.md
justfile
TEW03.jucer
.cursorrules

## MIDI & Sequencer rules
- Accept standard note on/off + pitch bend
- Internal step sequencer is the primary performance source (like a 303)
- Sequencer must be able to:
  - Run in sync with host transport (when available)
  - Generate note-on/off + accent + slide events
  - Emit those events both to the internal Voice and as MIDI Output
- MIDI Output is used for:
  1. Recording / exporting the sequence into the DAW
  2. Optionally driving other plugins (future-proof)
- All MIDI generation must be real-time safe (no allocations in processBlock)

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Use blocking calls in processBlock
- Copy large amounts of commercial plugin code
