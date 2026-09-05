# Patty Punch

Patty Punch is a sample-free, four-voice, analog-inspired JUCE drum instrument. It
synthesizes a swept-sine kick, dual-oscillator/noise snare, metallic six-square
oscillator hi-hat, and swept modal electronic tom. No samples are loaded at runtime.

## Requirements

- CMake 3.22 or newer
- A C++17 compiler
- A JUCE source checkout supplied with `-DJUCE_PATH=/path/to/JUCE`
- A VST3, AU, or Standalone host as appropriate

CMake also accepts a repository-local `JUCE` directory or an installed JUCE package.
The current macOS development checkout uses `/Users/faux/Dev/JUCE`.

## Configure and build

Windows, from a Visual Studio developer environment:

```powershell
cmake -S . -B build -G Ninja -DJUCE_PATH=C:/Users/blaus/Juce/JUCE
cmake --build build --target PattyPunch_VST3 PattyPunch_Standalone PattyPunchTests
```

macOS:

```sh
cmake -S . -B build -DJUCE_PATH=/Users/faux/Dev/JUCE -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target PattyPunch_VST3 PattyPunch_AU PattyPunch_Standalone PattyPunchTests
```

Run tests with `ctest --test-dir build --output-on-failure`. On macOS, a successful
VST3 build copies the bundle to `~/Library/Audio/Plug-Ins/VST3/`. Windows retains its
existing `C:/Users/blaus/Juce/DevVST3/` development copy step.

## MIDI

Fixed pitched zones:

- Kick: C0-B1 / MIDI 24-47 (root C0 / 24)
- Snare: C2-B2 / MIDI 48-59 (root C2 / 48)
- Hi-hat: C3-B3 / MIDI 60-71 (root C3 / 60)
- Tom: C4-B4 / MIDI 72-83 (root C4 / 72)

Each semitone above a zone root transposes that hit by one semitone without changing
its base Pitch parameter. Notes outside MIDI 24-83 are ignored. Pads and A/S/D/F
trigger kick/snare/hat/tom at their zone roots.

## Synthesis and modulation

- **Kick:** swept sine body, exponential decay, filtered-noise click, and compensated
  soft saturation. Eight fixed voices permit bounded tail overlap.
- **Snare:** two damped inharmonic shell modes plus deterministic filtered noise,
  body/noise balance, tone, and compensated drive.
- **Hi-hat:** six persistent PolyBLEP square oscillators feed two metallic filtered
  paths and a high-pass stage. Eight envelopes overlap; Choke fades prior envelopes.
- **Tom:** a phase-accumulating high-Q fundamental and weak inharmonic mode receive an
  exponential downward pitch bend, body decay, short filtered-noise stick transient,
  and compensated saturation. Eight fixed voices permit bounded overlap.
- **WARBLER:** four sample-accurate free-running LFOs, each with two readable
  destination/depth routes plus cross-LFO phase warp. Pitch uses semitones, time uses
  exponential ratios, gain/drive uses dB, and timbre uses bounded normalized movement.
  Repeated hits sample the current LFO for trigger-domain controls. The legacy hat
  pitch/decay depth IDs remain active but hidden for session compatibility.

## Repeat bursts

Every drum has Repeat Count (`Off`, then 1-10 repeats), Repeat Time (10-500 ms), and
Repeat Shape (0-100%). Incoming hits restart that drum's fixed-capacity burst. For
repeat `n` of `N`, with `x = n/N` and `s = Shape/100`:

```text
position(n) = x^(1 + 3s)
gain(n)     = 0.78^n * exp(-2sx)
time(n)     = N * RepeatTime * position(n)
```

At Shape zero, spacing is exactly Repeat Time and gain follows a conventional
exponential fade. Higher Shape values pull early repeats toward the original and fade
the burst more aggressively. Repeats retrigger synthesis rather than replaying audio.

## Compatibility

Existing automation IDs and ordering are preserved. `kickSweepTime`, `kickClickTone`,
`snareLow`, `snareCrack`, and `snareAir` remain serialized for old sessions but are
hidden from the simplified UI. The three legacy snare EQ parameters remain inert, as
they were in the original DSP. Missing tom, repeat, and routing fields are inserted
with neutral defaults when an older state is loaded.

The summed output passes through a 20 Hz DC/high-pass stage, conservative soft limiting,
and smoothed master gain.
