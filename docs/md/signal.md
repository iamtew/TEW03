# How sound is made

One voice. One note at a time. That is the whole joke, and it is also the point.

The path is: oscillator (saw or square) into **Drive**, into the diode-ladder filter, then **Nasty**, then the amp envelope, optional **Pre EQ**, the FX chain, optional **Post EQ**, then **Volume**.

- **Oscillator** — saw or square, band-limited. Toggle **Waveform** on Main.
- **Drive** — gain into the diode-ladder filter. The ladder clips; this is the grit before cutoff, not a post-FX distortion.
- **Filter** — resonant low-pass. **Cutoff** and **Resonance** set the squelch. The same decay envelope that shapes the amp also opens the filter; **Env Mod** is how far.
- **Accent** — on accented steps (and MIDI notes that landed that way), extra level, cutoff, and resonance. MIDI out uses velocity 127 for accent, 100 otherwise.
- **Nasty** — after the filter. Mixes a saturated copy of the resonant peak back in. Zero is dry ladder output.
- **Amp envelope** — fast attack, **Decay** sets the fall.
- **Pitch bend** — ±2 semitones from MIDI.

**Slide vs Glide.** Per-step **S** ties into the next note and always ramps pitch. Second click is super slide: the gate holds through rests until the next playing note, then ramps as usual. **Glide** also ramps when you play overlapping notes without slide. If Glide is at zero, a slide still gets a short default ramp (~50 ms) so the tie is audible.
