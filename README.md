<img src="assets/icon.png" alt="TEW03" width="160">

# TEW03

**TEW03** is a monophonic acid synth. Classic 303 squelch (saw/square, resonant filter, accent, slide, drive) plus extra nastiness.

**Main** — the synth, a 3×12 step sequencer (16 or 32 steps, accent and slide, key lock), and two drawn LFOs you can drop on any knob. Patterns play the voice and go out as MIDI. Run modes: **Keyboard** (play it from MIDI), **Standalone** (loop the selected slot; DAW UI hides this), **Pattern** (hold a key to trigger a pattern).

**Effects** — chorus, compressor, delay, distortion, EQ, filter, flanger, phaser, reverb. Click to add, drag to reorder. All off until you turn them on.

**EQ** — five-band parametric before and after the effects, with the spectrum behind the curve.

VST3 and standalone. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

## Release

`just build-dist` writes `dist/TEW03-<version>-win-x64-vst3.zip`, `dist/TEW03-<version>-win-x64-standalone.zip`, and `dist/TEW03-<version>-win-x64.msi`. The MSI lets you install the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\TEW03`; each folder can be changed. The standalone install adds a Start menu shortcut. Building the MSI needs the .NET 8 SDK.
