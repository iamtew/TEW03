# TEW03 – Bootstrap for Cursor

**Goal**  
Build a monophonic software synthesizer VST3 (+ Standalone) focused on aggressive TB-303-style acid lines (“mean lines”).  
Not a 1:1 clone. Primary goal = classic resonant squelch + extra nastiness (drive, filter aggression, optional sub, extra modulation).  
It must also have a “Classic 303” mode that folds parameters into typical TB-303 behaviour.

**Core performance model**  
An internal step sequencer is the primary way to play the instrument (classic 303 workflow).  
The sequencer drives the internal voice **and** emits MIDI Output so the user can:
- Record / export the sequence into the DAW as real MIDI notes
- Optionally sequence other plugins later

**Tech choices (do not change unless asked)**  
- JUCE (already installed at `~/JUCE` or `%USERPROFILE%\JUCE`)  
- **Projucer only** – no CMake  
- **Just** + `justfile` as the command runner  
- C++17/20, real-time safe code  
- Target formats: VST3 + Standalone  
- Windows first (MSVC via Visual Studio 2022 Build Tools)

---

## 0. Prerequisites the human must have done

- JUCE folder lives at `~/JUCE` (or `C:\Users\<user>\JUCE`)
- Visual Studio 2022 Build Tools installed with C++ workload  
  (winget command was:  
  `winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"`)
- `just` installed (`winget install --id Casey.Just --exact`)

---

## 1. Project creation steps Cursor must perform

1. Create a new Audio Plug-In project with the Projucer that points to the local JUCE modules.
2. Name the project **TEW03**.
3. Settings inside Projucer:
   - Plugin Formats: VST3 + Standalone
   - Plugin Characteristics:
     - Plugin is a Synth
     - Plugin MIDI Input
     - Plugin MIDI Output          ← required for sequencer → DAW / other plugins
4. Save the `.jucer` file in the root of this folder.
5. Let Projucer generate the Visual Studio 2022 exporter.

---

## 2. Files Cursor must create immediately

### `justfile`
```just
# TEW03 – Justfile (Projucer + MSBuild)

set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

JUCE_PATH := env_var_or_default("JUCE_PATH", env_var("USERPROFILE") + "\\JUCE")
PROJUCER  := JUCE_PATH + "\\Projucer.exe"
PROJECT   := "TEW03.jucer"
SOLUTION  := "Builds\\VisualStudio2022\\TEW03.sln"

default:
    @just --list

projucer:
    {{PROJUCER}} {{PROJECT}}

resave:
    {{PROJUCER}} --resave {{PROJECT}}

build-debug:
    msbuild {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m

build-release:
    msbuild {{SOLUTION}} /p:Configuration=Release /p:Platform=x64 /m

clean:
    msbuild {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    msbuild {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}
```

### `.cursorrules` (or `AGENTS.md`)
```markdown
# TEW03 – Cursor Rules

You are building a VST3 software synthesizer focused on aggressive TB-303-style acid lines (“mean lines”).
Not a pure clone. Main purpose = classic resonant squelch + extra nastiness.
Must also support a “Classic 303” mode.

An internal step sequencer is the primary performance source.
The sequencer drives the internal voice and also emits MIDI Output
so patterns can be recorded into the DAW or used to sequence other plugins.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17/20
- Real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for all parameters
- Just + justfile for commands

## Coding style
- Separate Oscillator, Filter, Envelope, Voice, SynthEngine, Sequencer
- Prefer composition
- Header-only DSP where practical
- Atomic / smoothed parameter reads on the audio thread
- Comment non-obvious DSP constants

## Project layout
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/
    Oscillator.h
    DiodeLadderFilter.h   # or Moog-style, but aim for diode character
    Envelope.h
    Voice.h
    SynthEngine.h
    Sequencer.h           # step sequencer (pattern, slide, accent per step)
  midi/
    MidiHandler.h         # incoming MIDI + outgoing MIDI generation
  parameters/
    ParameterIDs.h
    ParameterLayout.h
docs/
  PRD.md
  ARCHITECTURE.md
  DSP_NOTES.md
justfile
TEW03.jucer
.cursorrules

## MIDI & Sequencer rules
- Accept standard note on/off + pitch bend
- Internal step sequencer is the primary performance source (like a 303)
- Sequencer must be able to:
  - Run in sync with host transport (when available)
  - Generate note-on/off + accent + slide events
  - Emit those events both to the internal Voice and as MIDI Output
- MIDI Output is used for:
  1. Recording / exporting the sequence into the DAW
  2. Optionally driving other plugins (future-proof)
- All MIDI generation must be real-time safe (no allocations in processBlock)

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Use blocking calls in processBlock
- Copy large amounts of commercial plugin code
```

### `docs/PRD.md`
```markdown
# TEW03 – Product Requirements

## Vision
A software synthesizer that delivers the classic TB-303 acid squelch and then goes further into aggressive, mean, modern acid lines.  
It can behave like a typical 303 when desired, but its main purpose is harder, dirtier, more flexible lines.

An internal step sequencer is the primary performance tool (classic 303 workflow).  
The sequencer drives the internal synth and also emits MIDI Output so patterns can be recorded into the DAW or used to sequence other plugins.

## MVP Must-haves

### Sound
- Oscillator: saw + square (band-limited)
- Resonant low-pass filter with high resonance (diode-ladder character preferred)
- Envelope that strongly modulates cutoff
- Accent (boosts level + cutoff + resonance)
- Slide / portamento
- Drive / overdrive
- Master volume + soft limiting
- “Classic 303” toggle that restricts behaviour toward original 303

### MIDI & Performance
- Full MIDI note input + pitch bend
- Internal monophonic step sequencer (classic 303-style):
  - Pattern length (16 steps default, configurable later)
  - Per-step: note, accent, slide/tie
  - Tempo sync with host transport when available; free-running fallback
- Sequencer drives the internal Voice **and** emits MIDI Output
- MIDI Output purpose:
  - Export / record the sequence into the DAW as real MIDI notes
  - Optionally sequence other plugins later
- All MIDI I/O must be real-time safe

### Plugin
- APVTS + basic GUI (GenericEditor is fine at start)
- VST3 + Standalone

## Non-goals
- Full polyphony
- Exact circuit-level emulation of every component
- Sample-based engine
- Complex multi-pattern song mode (keep it simple for MVP)
```

### `docs/ARCHITECTURE.md`
```markdown
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
```

### `docs/DSP_NOTES.md`
```markdown
# DSP Notes – Acid Character

- Oscillator: band-limited saw/square (PolyBLEP preferred)
- Filter: diode-ladder inspired, stable at high resonance, accent raises cutoff + resonance
- Envelope: fast attack, adjustable decay, accent intensifies
- Drive: soft saturation before and/or after filter
- Keep everything real-time safe

# Sequencer Notes
- Classic 303-style step sequencer (monophonic)
- Per step: pitch, accent flag, slide/tie flag
- Must generate clean note-on / note-off with correct timing
- Accent and slide must be applied both to the internal voice and reflected in the MIDI Output events where possible
```

---

## 3. First coding tasks Cursor should do after the structure exists

1. Make sure the generated PluginProcessor / PluginEditor compile with a silent (or very basic) voice.
2. Add a minimal set of APVTS parameters:
   - cutoff, resonance, decay, accent, drive, volume, classicMode, glide
   - sequencer: play/stop, tempo (or host-sync toggle), step count (later)
3. Implement a simple mono Voice that plays a band-limited saw wave and responds to MIDI notes.
4. Wire basic MIDI Input → Voice.
5. Add MIDI Output support (empty buffer is fine at first, but the plumbing must exist and be called every processBlock).
6. Skeleton Sequencer that can generate a simple repeating note and push it both to the Voice and to the MIDI Output buffer.
7. Keep processBlock allocation-free.

---

## 4. How the human will work with Cursor

- Drop this `bootstrap.md` into an empty folder.
- Open the folder in Cursor.
- Say: “Read bootstrap.md and execute it. Create the project and the first files.”
- Afterwards use normal prompts such as:
  - “Implement the diode-ladder filter according to DSP_NOTES.md”
  - “Add accent and glide behaviour”
  - “Make the Classic 303 mode work”
  - “Finish the step sequencer and make MIDI Output mirror the pattern”
  - “Add host transport sync to the sequencer”

---

## 5. Useful Just commands the human will run

```bash
just projucer      # open Projucer
just resave        # regenerate VS project after changes
just build-debug   # compile
just vs            # open Visual Studio
```

That is the complete bootstrap.  
Cursor should now be able to take over from an empty folder.
```
