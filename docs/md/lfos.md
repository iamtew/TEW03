# LFOs

Four lanes stacked on the right of the sequencer. **LFO 1** is green, **LFO 2** is orange, **LFO 3** is blue, **LFO 4** is pink. Each has a drawable breakpoint shape (up to 8 points). Draw something ugly. It will still modulate.

## Shape

The name bar cycles presets: **Triangle**, **Sin**, **Saw Up**, **Saw Down**, **Square**. A hand-drawn shape shows **Custom**.

- Drag a point to move it. Endpoints stay at the left and right of the cycle.
- Double-click empty space to add a point.
- Right-click an interior point to delete it (need at least two points).
- A playhead line moves when the LFO is running.

## Timing

- **Mode — Free** — runs continuously.
- **Mode — Trigger** — resets to the start of the shape on each note-on (keyboard or sequencer).
- **Sync** off — **Rate** is 0.05–30 Hz.
- **Sync** on — **Div** sets cycle length: 1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars (at the current tempo). All four LFOs default to Sync on (1/4).
- **Smooth** — lags the shape. 0 is raw breakpoints; 100% is a ~250 ms lag.

## Assigning

Drag an **LFO 1–4** handle onto a knob (or the volume fader). Right-click a knob and pick one of the four LFOs, or None. The lanes stay visible on the Effects page. Continuous effect knobs take an LFO the same way. Switches and type menus do not. The EQ page does not.

A numbered badge appears on the control. Drag that badge vertically for **amount** (−100% to +100%). The LFO is bipolar around the knob’s current value. An arc around the knob shows that sweep, and a dot on the arc tracks the live LFO position.

Destinations: cutoff, resonance, env mod, decay, accent, drive, glide, volume, nasty. **Tempo** cannot take an LFO.

Each destination takes **one** LFO.
