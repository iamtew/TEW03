# Sequencer

Left column is transport and pattern. Centre is the piano roll. This is the bit that makes it feel like a 303 and not a lonely oscillator.

## Play

Three modes (the DAW Play list is **Keyboard** and **Pattern** only):

- **Keyboard** — incoming MIDI notes play the voice. If **Run** is on, the sequencer still clocks for MIDI out and the playhead, but keys own the sound. In a DAW that clock also needs the host to be playing (same as Standalone).
- **Standalone** — loops the selected bank/pattern while **Run** is on. In a DAW it follows host transport: Run on *and* the host playing. The standalone app uses Run as the clock. Hidden from the Play list in a DAW.
- **Pattern** — MIDI notes **C1–B3** (MIDI 24–59) map chromatically onto the 36 slots (3 banks × 12 patterns). Hold a key to play that slot. Note-off of the current key stops. Unmapped notes are ignored. Uses host BPM when the host provides it; does **not** require the host transport to be playing.

Factory default: the standalone app opens in **Standalone** with **Run** off; in a DAW it opens in **Pattern** with **Run** on.

## Transport and slots

- **Run** — play / pause the sequencer.
- **Tempo** — 40–300 BPM. In a DAW the host BPM wins when the playhead reports one. Standalone (and hosts with no BPM) use this knob. LFOs and delay sync use the same tempo. While the host is playing, the sequencer locks to host PPQ so loop and locate stay on the grid. Pattern mode with the transport stopped still free-runs from the held key.
- **Swing** — delays every offbeat step. 0% is straight. 100% delays the offbeat by 60% of a step; the next downbeat lands back on the grid. With **2x** on, that grid is 32nd notes.
- **2x** — splits each step in half (16 sixteenths vs 32 thirty-seconds). Global grid, not a per-step flag. Last step doubles or halves with it.
- **Len** — last step (1–16, or 1–32 with **2x**). The loop wraps there. Columns after Len stay in the pattern but do not play; drawing on them extends Len. Per pattern, saved with the bank.
- **Bank** — pattern bank 1–3.
- **Pat** — pattern 1–12 in that bank. Click the number for a menu, arrows to step.
- **Key** / **Scale** — root (C–B) and Major / Minor. Used when **Lock** is on.
- **Up** / **Dn** — shift the current pattern up or down one semitone. With **Lock** on, each note moves one scale step. Double-click either button to move a full octave. Rests stay rests. Notes stop at C1 and B4.
- **Lock** — snaps sequencer pitches to that key and scale. The piano-roll rows are the in-scale notes, stretched to fill the roll.
- **Clear** — empties the current pattern (notes, accent, slide).

Edits write into the live slot. Switching bank/pattern copies that slot into the live sequencer.

## Piano roll

Range is **C1–B4**. Two octaves are on screen (opens on C2–B3); scroll for the rest.

- **Click** a cell to put that pitch on that step, or to clear it if it was already that pitch.
- **Drag** a note up/down to change pitch (snaps to scale when Lock is on).
- **Mouse wheel** on a note nudges pitch (scale steps when Lock is on).
- **Left gutter** — drag or wheel to scroll when the range is taller than the window. Top and bottom key chevrons show only then.
- **A** under a column — accent that step.
- **S** under a column — click once for slide / tie into the next step, twice for **super slide** (holds through rests until the next playing note, LFO1 green chevron).
- **Playhead** — red column while the sequencer runs.
- **LFO overlay** — when an LFO is assigned to a knob, its shape is drawn across the bar (phase, not pitch).
