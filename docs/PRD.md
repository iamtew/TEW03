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
- Two assignable LFOs with custom breakpoint shapes, free-running or synced to the host tempo

### MIDI & Performance
- Full MIDI note input + pitch bend
- Internal monophonic step sequencer (classic 303-style):
  - 3 banks × 12 patterns (36 slots)
  - Pattern length 16 or 32 (2x splits each step in half, global)
  - Per-step: note, accent, slide/tie
  - Play/pause (Run) plus play modes: Keyboard (MIDI → voice), Pattern (loop selected slot), Key (hold C1–B3 to play the mapped slot)
  - Tempo sync with host transport when available; free-running fallback
- Sequencer drives the internal Voice **and** emits MIDI Output
- MIDI Output purpose:
  - Export / record the sequence into the DAW as real MIDI notes
  - Optionally sequence other plugins later
- All MIDI I/O must be real-time safe

### Plugin
- APVTS
- Editor pages: Main (synth, sequencer, LFOs), Effects, EQ
- Effects chain: chorus, compressor, delay, distortion, equalizer, filter, flanger, phaser, reverb. Off until enabled. Drag to reorder.
- Pre-FX and post-FX five-band parametric EQ, both off by default
- VST3 + Standalone

## Non-goals
- Full polyphony
- Exact circuit-level emulation of every component
- Sample-based engine
- Song mode / chained pattern playback
