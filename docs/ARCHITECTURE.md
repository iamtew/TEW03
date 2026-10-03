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
4. Voice: Osc → (pre-drive) → Filter (env + accent) → (post-drive) → Amp Env → Output

Play modes (`playMode`):
- Keyboard: host MIDI notes play the voice.
- Pattern: loops the selected bank/pattern when Run is on. Follows host transport in a DAW.
- Key: MIDI C1–B3 (24–59) maps chromatically onto the 3×12 bank. Hold plays that slot while Run is on; note-off of the current key stops. Unmapped notes are ignored. Uses host BPM when present, does not require host `isPlaying`.

Pattern bank:
- 3 banks × 12 patterns stored as packed atomics (audio-thread safe) and in APVTS `BANKS` state.
- Live sequencer is the current slot. Edits dual-write. Slot switch copies packed steps only.

Key design points:
- Sequencer is the primary performance engine (303-style).
- MIDI Output always mirrors what the sequencer (and/or incoming notes) is playing so the user can record the pattern into the DAW.
- Everything on the audio thread must be allocation-free and lock-free.
