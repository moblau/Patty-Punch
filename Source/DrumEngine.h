#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include <array>
#include <atomic>

inline constexpr size_t lfoCount = 4;
inline constexpr size_t lfoTraceSize = 128;
inline constexpr size_t drumCount = 4;

float evaluateLfoWaveform (int shape, float phase) noexcept;
float repeatCurvePosition (int repeatIndex, int repeatCount, float shape) noexcept;
float repeatCurveGain (int repeatIndex, int repeatCount, float shape) noexcept;

struct ModRoute
{
    Params::ModDestination destination = Params::ModDestination::off;
    float depth = 0;
};

struct LfoParameters
{
    float rate = 1, legacyHatPitchDepth = 0, legacyHatDecayDepth = 0, warp = 0;
    int shape = 0, modSource = -1;
    std::array<ModRoute, 2> routes {};
};

struct RepeatParameters
{
    int count = 0;
    float timeSeconds = .09f, shape = 0;
};

struct DrumParameters
{
    float kickTune = 52, kickDecay = 1.6f, kickSweep = 30, kickSweepTime = .04f, kickClick = .35f;
    float kickClickTone = 4500, kickDrive = 2, kickLevelDb = -2;
    float snareTune = 185, snareDecay = .42f, snareSnappy = .6f, snareTone = 3500;
    float snareDrive = 3, snareLevelDb = -2;
    float hatTune = 0, hatDecay = .18f, hatTone = .55f, hatHP = 6000, hatLevelDb = -5;
    bool hatChoke = true;
    float tomTune = 140, tomSweep = 12, tomDecay = .52f, tomTone = .58f, tomAttack = .22f;
    float tomLevelDb = -3;
    std::array<RepeatParameters, drumCount> repeats {};
    std::array<LfoParameters, lfoCount> lfos {};
    float master = .7f;
};

struct LfoDisplaySnapshot
{
    float phase = 0, output = 0;
    std::array<float, lfoTraceSize> trace {};
};

class OnePole
{
public:
    void reset() { z = 0; }
    void setLowPass (float hz, double sr)
    {
        hz = juce::jlimit (1.0f, static_cast<float> (sr * .45), hz);
        a = std::exp (-juce::MathConstants<float>::twoPi * hz / static_cast<float> (sr));
    }
    float low (float x) { z = (1 - a) * x + a * z; return z; }
    float high (float x) { return x - low (x); }
private:
    float z = 0, a = .5f;
};

class KickVoice
{
public:
    void prepare (double);
    void trigger (float velocity, float pitchMultiplier, const DrumParameters&);
    float render();
    bool active() const { return env > 1.0e-5f; }
    float getFinalFrequencyForTests() const { return target; }
private:
    double sr = 44100, phase = 0;
    float env = 0, envMul = 0, freq = 52, target = 52, sweepMul = 1;
    float clickEnv = 0, clickMul = 0, clickAmount = 0, drive = 1, gain = 1;
    uint32_t rng = 1;
    OnePole clickLP;
};

class SnareVoice
{
public:
    void prepare (double);
    void trigger (float velocity, float pitchMultiplier, const DrumParameters&);
    float render();
    bool active() const { return toneEnv > 1.0e-5f || noiseEnv > 1.0e-5f; }
    std::pair<float, float> getTonalFrequenciesForTests() const { return { f1, f2 }; }
private:
    double sr = 44100, p1 = 0, p2 = 0;
    float f1 = 185, f2 = 296, toneEnv = 0, toneMul = 0, tone2Mul = 0;
    float noiseEnv = 0, noiseMul = 0, snappy = .6f, drive = 1, gain = 1;
    uint32_t rng = 0x1234567u;
    OnePole noiseHP, noiseLP;
};

class HatEngine
{
public:
    void prepare (double);
    void reset();
    void trigger (float velocity, float pitchMultiplier, const DrumParameters&);
    float render (const DrumParameters&);
    float getMidiPitchRatioForTests() const { return midiPitchCurrent; }
    float getMaximumOscillatorFrequencyForTests() const { return maximumOscillatorFrequency; }
private:
    struct Env { float level = 0, mul = 0, chokeMul = 1; };
    std::array<double, 6> phases {};
    std::array<Env, 8> envs {};
    double sr = 44100;
    OnePole lowBand, highBand, hp;
    int cursor = 0;
    float midiPitchTarget = 1, midiPitchCurrent = 1, midiPitchSmooth = .01f;
    float maximumOscillatorFrequency = 0;
};

class TomVoice
{
public:
    void prepare (double);
    void trigger (float velocity, float pitchMultiplier, const DrumParameters&);
    float render();
    bool active() const { return env > 1.0e-5f || attackEnv > 1.0e-5f; }
    float getFinalFrequencyForTests() const { return target; }
private:
    double sr = 44100, phase1 = 0, phase2 = 0;
    float env = 0, envMul = 0, attackEnv = 0, attackMul = 0;
    float freq = 140, target = 140, sweepMul = 1, secondRatio = 1.47f;
    float modeMix = .12f, attackAmount = .2f, drive = 2.2f, gain = 1;
    uint32_t rng = 0x5a17b31u;
    OnePole attackLP;
};

class DrumEngine
{
public:
    enum Instrument { kick, snare, hat, tom };
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void trigger (Instrument, float velocity, float pitchMultiplier, const DrumParameters&);
    void render (juce::AudioBuffer<float>&, int start, int count, const DrumParameters&);
    float getLfoValue() const { return lfoVisuals[0].output.load (std::memory_order_relaxed); }
    void getLfoSnapshot (size_t index, LfoDisplaySnapshot&) const noexcept;
    const HatEngine& getHatForTests() const { return hats; }
    std::array<float, 8> getKickFinalFrequenciesForTests() const;
    std::array<float, 8> getSnareFinalFrequenciesForTests() const;
    std::array<float, 8> getTomFinalFrequenciesForTests() const;
    float getModulationForTests (Params::ModDestination destination) const noexcept;
    uint32_t getRepeatTriggerCountForTests (Instrument instrument) const noexcept
    {
        return repeatTriggerCounts[static_cast<size_t> (instrument)];
    }
    int64_t getRepeatOffsetForTests (Instrument instrument, int index) const noexcept
    {
        return index >= 0 && index < 10
            ? repeatSchedulers[static_cast<size_t> (instrument)].offsets[static_cast<size_t> (index)] : 0;
    }

private:
    struct LfoVisualState
    {
        std::atomic<float> phase { 0 }, output { 0 };
        std::array<std::atomic<float>, lfoTraceSize> trace {};
    };

    struct RepeatScheduler
    {
        std::array<int64_t, 10> offsets {};
        std::array<float, 10> gains {};
        int count = 0, next = 0;
        int64_t elapsed = 0;
        float velocity = 1, pitchMultiplier = 1;
        void clear() noexcept { count = next = 0; elapsed = 0; }
    };

    void initialiseTrace (size_t index, int shape) noexcept;
    void advanceLfos (const DrumParameters&) noexcept;
    DrumParameters applyModulation (const DrumParameters&) const noexcept;
    void triggerSynth (Instrument, float velocity, float pitchMultiplier, const DrumParameters&);
    void scheduleRepeats (Instrument, float velocity, float pitchMultiplier, const RepeatParameters&) noexcept;
    void processRepeats (const DrumParameters&);

    std::array<KickVoice, 8> kicks;
    std::array<SnareVoice, 8> snares;
    std::array<TomVoice, 8> toms;
    HatEngine hats;
    size_t kickCursor = 0, snareCursor = 0, tomCursor = 0;
    double sr = 44100;
    std::array<double, lfoCount> lfoPhases {};
    std::array<float, lfoCount> lfoOutputs {};
    std::array<float, Params::modDestinationCount> modulation {};
    std::array<int, lfoCount> lastTraceBins { -1, -1, -1, -1 };
    std::array<int, lfoCount> lastShapes { -1, -1, -1, -1 };
    std::array<LfoVisualState, lfoCount> lfoVisuals;
    std::array<RepeatScheduler, drumCount> repeatSchedulers {};
    std::array<uint32_t, drumCount> repeatTriggerCounts {};
    OnePole dcBlock;
    float masterGain = 0;
};
