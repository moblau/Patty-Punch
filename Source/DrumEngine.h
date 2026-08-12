#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

inline constexpr size_t lfoCount = 4;
inline constexpr size_t lfoTraceSize = 128;

float evaluateLfoWaveform (int shape, float phase) noexcept;

struct LfoParameters
{
    float rate=1, pitchDepth=0, decayDepth=0, warp=0;
    int shape=0, modSource=-1;
};

struct DrumParameters
{
    float kickTune=52, kickDecay=1.6f, kickSweep=30, kickSweepTime=.04f, kickClick=.35f;
    float kickClickTone=4500, kickDrive=2, kickLevel=.8f;
    float snareTune=185, snareDecay=.42f, snareSnappy=.6f, snareTone=3500;
    float snareLow=0, snareCrack=0, snareAir=0, snareLevel=.8f;
    float hatTune=0, hatDecay=.18f, hatTone=.55f, hatHP=6000, hatLevel=.56f;
    bool hatChoke=true;
    std::array<LfoParameters,lfoCount> lfos {};
    float master=.7f;
};

struct LfoDisplaySnapshot
{
    float phase=0, output=0;
    std::array<float,lfoTraceSize> trace {};
};

class OnePole
{
public:
    void reset() { z = 0; }
    void setLowPass (float hz, double sr) { a = std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sr); }
    float low (float x) { z = (1-a)*x + a*z; return z; }
    float high (float x) { return x - low (x); }
private: float z=0, a=.5f;
};

class KickVoice
{
public:
    void prepare (double); void trigger (float velocity, float pitchMultiplier, const DrumParameters&); float render();
    bool active() const { return env > 1.0e-5f; }
    float getFinalFrequencyForTests() const { return target; }
private:
    double sr=44100, phase=0; float env=0, envMul=0, freq=52, target=52, sweepMul=1;
    float clickEnv=0, clickMul=0, clickAmount=0, drive=1, gain=1, stealFade=1; uint32_t rng=1; OnePole clickLP;
};

class SnareVoice
{
public:
    void prepare (double); void trigger (float velocity, float pitchMultiplier, const DrumParameters&); float render();
    bool active() const { return toneEnv > 1.0e-5f || noiseEnv > 1.0e-5f; }
    std::pair<float, float> getTonalFrequenciesForTests() const { return { f1, f2 }; }
private:
    double sr=44100, p1=0, p2=0; float f1=185, f2=296, toneEnv=0, toneMul=0, tone2Mul=0;
    float noiseEnv=0, noiseMul=0, snappy=.6f, gain=1; uint32_t rng=0x1234567u; OnePole noiseHP, noiseLP;
};

class HatEngine
{
public:
    void prepare (double); void reset(); void trigger (float velocity, float pitchMultiplier,
                                                       const DrumParameters&, float lfo);
    float render (const DrumParameters&, float lfo);
    float getMidiPitchRatioForTests() const { return midiPitchCurrent; }
    float getMaximumOscillatorFrequencyForTests() const { return maximumOscillatorFrequency; }
private:
    struct Env { float level=0, mul=0, chokeMul=1; };
    std::array<double,6> phases{}; std::array<Env,8> envs{}; double sr=44100;
    OnePole lowBand, highBand, hp; int cursor=0;
    float midiPitchTarget=1, midiPitchCurrent=1, midiPitchSmooth=0.01f;
    float maximumOscillatorFrequency=0;
};

class DrumEngine
{
public:
    enum Instrument { kick, snare, hat };
    void prepare (double sampleRate, int maxBlock); void reset();
    void trigger (Instrument, float velocity, float pitchMultiplier, const DrumParameters&);
    void render (juce::AudioBuffer<float>&, int start, int count, const DrumParameters&);
    float getLfoValue() const { return lfoVisuals[0].output.load (std::memory_order_relaxed); }
    void getLfoSnapshot (size_t index, LfoDisplaySnapshot&) const noexcept;
    const HatEngine& getHatForTests() const { return hats; }
    float getPitchModulationForTests() const { return pitchModulation; }
    float getDecayModulationForTests() const { return decayModulation; }
    std::array<float,8> getKickFinalFrequenciesForTests() const
    {
        std::array<float,8> result {};
        for(size_t i=0;i<kicks.size();++i)result[i]=kicks[i].getFinalFrequencyForTests();
        return result;
    }
private:
    struct LfoVisualState
    {
        std::atomic<float> phase { 0 }, output { 0 };
        std::array<std::atomic<float>,lfoTraceSize> trace {};
    };

    void initialiseTrace (size_t index, int shape) noexcept;
    void advanceLfos (const DrumParameters&) noexcept;

    std::array<KickVoice,8> kicks; std::array<SnareVoice,8> snares; HatEngine hats;
    size_t kickCursor=0, snareCursor=0; double sr=44100;
    std::array<double,lfoCount> lfoPhases {};
    std::array<float,lfoCount> lfoOutputs {};
    std::array<int,lfoCount> lastTraceBins { -1, -1, -1, -1 };
    std::array<int,lfoCount> lastShapes { -1, -1, -1, -1 };
    std::array<LfoVisualState,lfoCount> lfoVisuals;
    float pitchModulation=0, decayModulation=0;
    OnePole dcBlock; float masterGain=0, meter=0;
};
