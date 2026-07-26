#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

struct DrumParameters
{
    float kickTune=52, kickDecay=1.6f, kickSweep=30, kickSweepTime=.04f, kickClick=.35f;
    float kickClickTone=4500, kickDrive=2, kickLevel=.8f;
    float snareTune=185, snareDecay=.42f, snareSnappy=.6f, snareTone=3500;
    float snareLow=0, snareCrack=0, snareAir=0, snareLevel=.8f;
    float hatTune=0, hatDecay=.18f, hatTone=.55f, hatHP=6000, hatLevel=.56f;
    bool hatChoke=true;
    float lfoRate=1, lfoPitch=0, lfoDecay=0;
    int lfoShape=0;
    float master=.7f;
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
    void prepare (double); void trigger (float velocity, const DrumParameters&); float render();
    bool active() const { return env > 1.0e-5f; }
private:
    double sr=44100, phase=0; float env=0, envMul=0, freq=52, target=52, sweepMul=1;
    float clickEnv=0, clickMul=0, clickAmount=0, drive=1, gain=1, stealFade=1; uint32_t rng=1; OnePole clickLP;
};

class SnareVoice
{
public:
    void prepare (double); void trigger (float velocity, const DrumParameters&); float render();
    bool active() const { return toneEnv > 1.0e-5f || noiseEnv > 1.0e-5f; }
private:
    double sr=44100, p1=0, p2=0; float f1=185, f2=296, toneEnv=0, toneMul=0, tone2Mul=0;
    float noiseEnv=0, noiseMul=0, snappy=.6f, gain=1; uint32_t rng=0x1234567u; OnePole noiseHP, noiseLP;
};

class HatEngine
{
public:
    void prepare (double); void reset(); void trigger (float velocity, const DrumParameters&, float lfo);
    float render (const DrumParameters&, float lfo);
private:
    struct Env { float level=0, mul=0, chokeMul=1; };
    std::array<double,6> phases{}; std::array<Env,8> envs{}; double sr=44100;
    OnePole lowBand, highBand, hp; int cursor=0;
};

class DrumEngine
{
public:
    enum Instrument { kick, snare, hat };
    void prepare (double sampleRate, int maxBlock); void reset();
    void trigger (Instrument, float velocity, const DrumParameters&);
    void render (juce::AudioBuffer<float>&, int start, int count, const DrumParameters&);
    float getLfoValue() const { return lfoValue.load(); }
private:
    std::array<KickVoice,8> kicks; std::array<SnareVoice,8> snares; HatEngine hats;
    size_t kickCursor=0, snareCursor=0; double sr=44100, lfoPhase=0; std::atomic<float> lfoValue { 0 };
    OnePole dcBlock; float masterGain=0, meter=0;
};
