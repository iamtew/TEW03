# DSP Notes – Acid Character

- Oscillator: band-limited saw/square (PolyBLEP preferred)
- Filter: diode-ladder inspired, stable at high resonance, accent raises cutoff + resonance
- Envelope: fast attack, adjustable decay, Env Mod sets how far the envelope opens the filter, accent intensifies
- Drive: osc gain into the ladder tanh (0 ≈ 0.35, 1 ≈ 8.35). Ladder diodes do the clip.
- Nasty: post-filter mix of tanh on the squelch peak. 0 is dry filter out.
- Keep everything real-time safe

# Signal path
- Osc → Drive gain → diode-ladder filter → Nasty mix → amp env. Accent raises level, cutoff, and resonance.
- Then the amp envelope, pre-EQ (off by default), the FX chain, post-EQ (off by default), and the master.

# Sequencer Notes
- Classic 303-style step sequencer (monophonic)
- Per step: pitch, accent flag, slide/tie flag
- Must generate clean note-on / note-off with correct timing
- Accent and slide must be applied both to the internal voice and reflected in the MIDI Output events where possible
