# TEW03 – Product Requirements

## Vision
A software synthesizer that delivers the classic TB-303 acid squelch and then goes further into aggressive, mean, modern acid lines.

An internal step sequencer is the primary performance tool (classic 303 workflow).  
The sequencer drives the internal synth and also emits MIDI Output so patterns can be recorded into the DAW or used to sequence other plugins.

## MVP Must-haves

### Sound
- Oscillator: saw + square (band-limited)
- Resonant low-pass filter with high resonance (diode-ladder character preferred)
- Envelope that strongly modulates cutoff (Env Mod amount)
- Accent (boosts level + cutoff + resonance)
- Slide / portamento
- Drive / overdrive
- Master volume + soft limiting

### MIDI & Performance
- Full MIDI note input + pitch bend
- Internal monophonic step sequencer (classic 303-style):
  - Pattern length (16 steps default, configurable later)
  - Per-step: note, accent, slide/tie
  - Tempo sync with host transport when available; free-running fallback
- Sequencer drives the internal Voice **and** emits MIDI Output
- MIDI Output purpose:
  - Export / record the sequence into the DAW as real MIDI notes
  - Optionally sequence other plugins later
- All MIDI I/O must be real-time safe

### Plugin
- APVTS + basic GUI (GenericEditor is fine at start)
- VST3 + Standalone

## Non-goals
- Full polyphony
- Exact circuit-level emulation of every component
- Sample-based engine
- Complex multi-pattern song mode (keep it simple for MVP)
