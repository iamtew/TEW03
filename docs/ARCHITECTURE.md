# Architecture

PluginProcessor owns:
- APVTS
- SynthEngine
- Sequencer
- MidiHandler (incoming + outgoing)

Voice contains:
- Oscillator(s)
- Resonant filter
- Amp + Filter envelopes
- Accent + Glide state

Data flow (audio thread):

1. Host MIDI in  →  MidiHandler
2. Sequencer (if running) generates note / accent / slide events
3. Events → Voice (for sound) **and** → MIDI Output buffer (for DAW / other plugins)
4. Voice: Osc → Drive gain → Filter (env + accent) → Nasty mix → Amp Env → Output
5. Pre-FX parametric EQ (optional, default off) → FX chain (insert, user order) → Post-FX parametric EQ (optional, default off) → master out

FX chain (Effects page):
- One instance each of chorus, compressor, delay, distortion, EQ, filter, flanger, phaser, reverb.
- Enable from the left rail; drag rows to reorder. Bypass is APVTS; order is `FX_ORDER` on `apvts.state` (also in `.tew3p`).
- Default all off. DSP is `juce::dsp` in `src/dsp/FxChain.h`, allocation-free after `prepareToPlay`.
- The insert Equalizer is the same 5-band parametric (`ParametricEq`) as Pre/Post, still reorderable in the chain. Pre/Post sit around the chain and are not reorderable.
- EQ page draws a post-FX FFT behind the response curve (`src/dsp/EqAnalyser.h`). Hop is 2048, lock-free double buffer.

Play modes (`playMode`):
- Keyboard: host MIDI notes play the voice.
- Pattern: loops the selected bank/pattern when Run is on. Follows host transport in a DAW.
- Key: MIDI C1–B3 (24–59) maps chromatically onto the 3×12 bank. Hold plays that slot while Run is on; note-off of the current key stops. Unmapped notes are ignored. Uses host BPM when present, does not require host `isPlaying`.
- Factory defaults: Standalone = Pattern + Run off; DAW = Key + Run on. LFO 1 Sync on; LFO 2 Sync off.

Pattern bank:
- 3 banks × 12 patterns stored as packed atomics (audio-thread safe) and in APVTS `BANKS` state.
- Live sequencer is the current slot. Edits dual-write. Slot switch copies packed steps only.

Patch vs bank files (message thread only):
- Patch (`.tew3p`) = APVTS knobs / play settings / LFO shapes / FX params + order. Never includes `BANKS`, `seqBank`, or `seqPattern`.
- Bank (`.tew3b`) = all 36 patterns. Never includes patch knobs.
- Library folder: `%APPDATA%/Stupid Systems LLC/TEW03/Patches` and `.../Banks`.
- Current names live as `PATCH_NAME` / `BANK_NAME` on `apvts.state` for DAW recall.
- Loading one library does not overwrite the other. `Init Patch` / `Init Bank` restore defaults / factory fills.

Two LFOs sit at the bottom of the editor. Shapes are editable breakpoints stored in `LFOS` on APVTS state (and in `.tew3p`). Rate / sync / mode / smooth / routing amounts are APVTS params. Each rotary (except BPM) can take one LFO plus a bipolar amount. Audio thread copies the published shape bank and runs the LFO per sample onto cutoff / resonance / envMod / drive / nasty / volume. Decay / accent / glide take the LFO at block rate.

Key design points:
- Sequencer is the primary performance engine (303-style).
- MIDI Output always mirrors what the sequencer (and/or incoming notes) is playing so the user can record the pattern into the DAW.
- Everything on the audio thread must be allocation-free and lock-free.
