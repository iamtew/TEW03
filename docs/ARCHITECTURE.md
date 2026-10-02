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

Key design points:
- Sequencer is the primary performance engine (303-style).
- External MIDI notes can still play the voice (or optionally be ignored when sequencer is active – decide later).
- MIDI Output always mirrors what the sequencer (and/or incoming notes) is playing so the user can record the pattern into the DAW.
- Everything on the audio thread must be allocation-free and lock-free.
