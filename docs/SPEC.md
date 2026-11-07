# TEW03 spec

This file is the product spec.

Monophonic acid synth. Classic 303 squelch plus extra nastiness. Stupid Audio. MIT. Not a circuit clone.

- Internal step sequencer is the primary performance source
- Sequencer drives the voice and emits MIDI out
- Not a full poly synth, not a sample engine, not an exact 303 clone

## Product

- VST3 + Standalone (Audio Unit on macOS)
- Synth plugin: MIDI in, MIDI out
- About + GitHub tag check (title-bar name + version)
- Window scale 40–150%, saved with the session
- Jucer `version=` is the binary, About dialog, git tag, and `just build-dist` name. Tags match that string (no `v` prefix).

Patches (`.tew3p`) and banks (`.tew3b`) are separate files. Loading one does not overwrite the other. They live in `%APPDATA%/Stupid Audio/TEW03/Patches` and `.../Banks`. A patch is sound knobs, play settings, LFO shapes, FX parameters and order — never the 36 patterns. A bank is all 36 patterns — never patch knobs.

## Voice

One voice. One note at a time.

- Oscillator: saw or square (band-limited)
- Drive into a resonant diode-ladder low-pass
- Decay envelope on amp and cutoff (`Env Mod`)
- Accent boosts level, cutoff, and resonance
- Nasty: post-filter saturation mix
- Slide / super slide on the sequencer; Glide on overlapping notes
- Pitch bend ±2 semitones
- Master volume + soft limiting

## Sequencer

Classic 303-style monophonic step sequencer.

- 3 banks × 12 patterns (36 slots)
- Last step 1–16 (1–32 with 2x). 2x splits each step in half, global. Unused steps stay stored.
- Per-step: note, accent, slide/tie (second click is super slide)
- Play modes: Keyboard (MIDI → voice), Standalone (loop selected slot; hidden in DAW UI), Pattern (hold C1–B3 to play the mapped slot)
- Tempo sync with host transport when available (BPM + PPQ lock while playing); free-running fallback
- Sequencer drives the internal Voice **and** emits MIDI Output (accent = velocity 127, otherwise 100) so the DAW can record the pattern, or another plugin can play it

## LFOs

Four assignable LFOs with custom breakpoint shapes. Free-running or synced to host tempo. Destinations take one LFO each. EQ page does not take an LFO.

## Pages

Main (synth, sequencer, LFOs), Effects, EQ.

- Effects chain: chorus, compressor, delay, distortion, equalizer, filter, flanger, phaser, reverb. Off until enabled. Drag to reorder.
- Pre-FX and post-FX five-band parametric EQ, both off by default

## Do not

- Full polyphony
- Exact circuit-level emulation of every component
- Sample-based engine
- Song mode / chained pattern playback
- CMake (Projucer only)
- Allocations, locks, or system calls in `processBlock`
- Commercial plugin code

## Stack

- JUCE from `%USERPROFILE%\JUCE`, Projucer workflow only
- C++17/20, APVTS for all parameters
- Just + justfile
- Windows first (MSVC / VS 2022 Build Tools)
- Real-time safe: no allocations, no locks, no system calls in processBlock

## UI

Yellow / cream chassis. Do not invent another palette.

- `#111111` ink
- `#1a1a1a` knob
- `#e8c200` yellow
- `#f4ead0` cream

## Layout

```
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/
    Oscillator.h
    DiodeLadderFilter.h
    Envelope.h
    Voice.h
    SynthEngine.h
    Sequencer.h
  midi/
    MidiHandler.h
  parameters/
    ParameterIDs.h
    ParameterLayout.h
docs/
justfile
TEW03.jucer
```

README stays small. LICENSE is MIT. `docs/` is the gh-pages site.
