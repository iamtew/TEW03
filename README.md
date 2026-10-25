<img src="assets/icon.png" alt="TEW03" width="160">

# TEW03

**TEW03** is a monophonic acid synth. Classic 303 squelch (saw/square, resonant filter, accent, slide, drive) plus extra nastiness.

**Main** — the synth, a 3×12 step sequencer (16 or 32 steps, accent and slide, key lock), and two drawn LFOs you can drop on any knob. Patterns play the voice and go out as MIDI. Run modes: **Keyboard** (play it from MIDI), **Standalone** (loop the selected slot; DAW UI hides this), **Pattern** (hold a key to trigger a pattern).

**Effects** — chorus, compressor, delay, distortion, EQ, filter, flanger, phaser, reverb. Click to add, drag to reorder. All off until you turn them on.

**EQ** — five-band parametric before and after the effects, with the spectrum behind the curve.

VST3 and standalone. Audio Unit on macOS. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

## Release

Windows, Linux, and macOS files come from GitHub Actions (`.github/workflows/dist.yml`). Run it from the Actions tab, or push a numeric tag such as `0.0.6`. The workflow does not publish a release. `<version>` is the `version` in `TEW03.jucer`.

The `dist` artifact holds:

- `TEW03-<version>-win-x64-vst3.zip`
- `TEW03-<version>-win-x64-standalone.zip`
- `TEW03-<version>-win-x64.msi`
- `TEW03-<version>-linux-x64-vst3.zip`
- `TEW03-<version>-linux-x64-standalone.zip`
- `TEW03-<version>-mac-universal-vst3.zip`
- `TEW03-<version>-mac-universal-au.zip`
- `TEW03-<version>-mac-universal-standalone.zip`

Download that artifact into `dist/` and upload the files on a GitHub release:

```
gh run download <run-id> -n dist -D dist
```

The MSI installs the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\TEW03`; each folder can be changed. The standalone install adds a Start menu shortcut.

The Mac zips are ad-hoc signed, not notarized. Gatekeeper blocks them until you right-click Open, or run `xattr -dr com.apple.quarantine` on the `.vst3`, the `.component`, and the `.app`. The Linux build uses the system ALSA, FreeType, fontconfig, and X11 libraries.

`just build-dist` on Windows still writes the three Windows files only. Building the MSI needs the .NET 8 SDK.
