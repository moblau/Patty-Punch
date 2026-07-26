# Patty Punch

Patty Punch is a sample-free, three-voice, 808-inspired JUCE drum instrument. It
synthesizes a swept-sine kick, dual-oscillator/noise snare, and metallic six-square
oscillator hi-hat. No sample files are loaded at runtime.

## Requirements

- CMake 3.22 or newer
- A C++17 compiler (this checkout was verified with Visual Studio Community 18)
- JUCE at `C:/Users/blaus/Juce/JUCE`, matching the sibling templates
- A VST3 host such as REAPER for plug-in use

This project intentionally preserves the repository's direct `add_subdirectory`
JUCE dependency method. Visual Studio's bundled Ninja is used because the normal
VS generator does not locate the compiler correctly in a plain PowerShell session.

## Configure and build

Run from `Projects/PattyPunch` in a Visual Studio developer environment:

```powershell
cmake -S . -B build -G Ninja
cmake --build build --target PattyPunch_VST3 PattyPunch_Standalone PattyPunchTests
```

Run tests:

```powershell
ctest --test-dir build --output-on-failure
```

The VST3 post-build step copies the bundle to
`C:/Users/blaus/Juce/DevVST3/Patty Punch.vst3`. Build artifacts are under
`build/PattyPunch_artefacts/Debug/`; the Standalone executable is in its
`Standalone` directory and the original VST3 bundle is in `VST3`.

## MIDI

The DAW routes MIDI to Patty Punch; the plug-in editor therefore has no hardware
MIDI selector. The JUCE Standalone wrapper provides hardware audio/MIDI settings.

Default mappings:

- Kick: C3 / MIDI 60
- Snare: C#3 / MIDI 61
- Hi-hat: D3 / MIDI 62

Use the `-`/`+` controls or arm `LEARN` and play the next MIDI note to edit a
mapping. Incoming Note On messages trigger one-shots on every MIDI channel.
On-screen pads and the A/S/D computer keys trigger kick/snare/hat respectively.

## Synthesis

- **Kick:** a sine body begins above its final tuning and falls exponentially,
  with independent amplitude decay, a filtered-noise click, and compensated soft
  saturation. Eight fixed voices allow overlapping bass tails.
- **Snare:** two damped inharmonic sines form the shell while deterministic
  high-/low-filtered noise forms the wires. Snappy changes their balance.
- **Hi-hat:** six persistent PolyBLEP square oscillators feed two bright filtered
  paths and a high-pass stage. Eight envelopes overlap without restarting phases.
  Choke fades old envelopes; the free-running LFO continuously bends oscillator
  pitch and samples per-hit decay variation.

The summed output passes through a 20 Hz DC/high-pass stage, conservative soft
limiting, and smoothed master gain.
