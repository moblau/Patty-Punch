#include "DrumEngine.h"

static float decayMul (float seconds, double sr, float end=1.0e-4f)
{ return static_cast<float> (std::exp (std::log (end) / juce::jmax (1.0, seconds * sr))); }
static float noise (uint32_t& s) { s ^= s<<13; s ^= s>>17; s ^= s<<5; return (float) (int32_t) s / 2147483648.0f; }

float evaluateLfoWaveform (int shape, float phase) noexcept
{
    if (! std::isfinite (phase))
        return 0;
    phase -= std::floor (phase);
    if (shape == 1) return 1.0f - 4.0f * std::abs (phase - .5f);
    if (shape == 2) return phase < .5f ? 1.0f : -1.0f;
    return std::sin (juce::MathConstants<float>::twoPi * phase);
}

void KickVoice::prepare (double s) { sr=s; clickLP.reset(); env=0; }
void KickVoice::trigger (float v, float pitchMultiplier, const DrumParameters& p)
{
    target=p.kickTune*pitchMultiplier; freq=target*std::pow(2.0f,p.kickSweep/12.0f);
    sweepMul=static_cast<float> (std::pow(target/freq, 1.0/juce::jmax(1.0, p.kickSweepTime*sr)));
    env=juce::jmap(v,.18f,1.0f); envMul=decayMul(p.kickDecay,sr);
    clickEnv=env; clickMul=decayMul(.008f,sr); clickAmount=p.kickClick;
    clickLP.setLowPass(p.kickClickTone,sr); drive=juce::Decibels::decibelsToGain(p.kickDrive); gain=p.kickLevel;
    phase=0; stealFade=1;
}
float KickVoice::render()
{
    if(!active()) return 0;
    freq=juce::jmax(target,freq*sweepMul); phase += juce::MathConstants<double>::twoPi*freq/sr;
    if(phase>juce::MathConstants<double>::twoPi) phase-=juce::MathConstants<double>::twoPi;
    auto x=(float)std::sin(phase)*env + clickLP.low(noise(rng))*clickEnv*clickAmount;
    env*=envMul; clickEnv*=clickMul;
    const auto y=std::tanh(x*drive)/juce::jmax(1.0f,std::tanh(drive));
    return y*gain;
}

void SnareVoice::prepare(double s){sr=s; toneEnv=noiseEnv=0; noiseHP.reset();noiseLP.reset();}
void SnareVoice::trigger(float v,float pitchMultiplier,const DrumParameters&p)
{
    f1=p.snareTune*pitchMultiplier; f2=f1*1.607f; toneEnv=juce::jmap(v,.18f,1.0f);
    noiseEnv=toneEnv; toneMul=decayMul(p.snareDecay*.72f,sr); tone2Mul=decayMul(p.snareDecay*.53f,sr);
    noiseMul=decayMul(p.snareDecay,sr); snappy=p.snareSnappy; gain=p.snareLevel;
    noiseHP.setLowPass(700,sr); noiseLP.setLowPass(p.snareTone,sr); p1=p2=0;
}
float SnareVoice::render()
{
    if(!active()) return 0;
    p1+=juce::MathConstants<double>::twoPi*f1/sr; p2+=juce::MathConstants<double>::twoPi*f2/sr;
    auto tonal=((float)std::sin(p1)*.62f+(float)std::sin(p2)*.38f)*toneEnv*(1-snappy*.65f);
    auto n=noiseLP.low(noiseHP.high(noise(rng)))*noiseEnv*snappy;
    toneEnv*=toneMul; noiseEnv*=noiseMul; return (tonal+n)*gain;
}

void HatEngine::prepare(double s)
{
    sr=s;
    midiPitchSmooth=1.0f-static_cast<float>(std::exp(-1.0/(.005*s)));
    reset();
}
void HatEngine::reset()
{
    phases.fill(0);for(auto&e:envs)e={};lowBand.reset();highBand.reset();hp.reset();
    midiPitchTarget=midiPitchCurrent=1;maximumOscillatorFrequency=0;
}
void HatEngine::trigger(float v,float pitchMultiplier,const DrumParameters&p,float decayModulation)
{
    if(p.hatChoke) for(auto&e:envs) if(e.level>0)e.chokeMul=decayMul(.004f,sr,.001f);
    auto&e=envs[(size_t)(cursor++%(int)envs.size())];
    const auto mult=std::pow(4.0f,juce::jlimit(-1.0f,1.0f,decayModulation));
    const auto d=juce::jlimit(.015f,2.5f,p.hatDecay*mult);
    e.level=juce::jmap(v,.18f,1.0f);e.mul=decayMul(d,sr);e.chokeMul=1;
    midiPitchTarget=juce::jlimit(0.125f,8.0f,pitchMultiplier);
}
static float polyBlep(float t,float dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
float HatEngine::render(const DrumParameters&p,float pitchModulation)
{
    static constexpr float base[]{205.3f,304.4f,369.6f,522.7f,540,800};
    midiPitchCurrent+=(midiPitchTarget-midiPitchCurrent)*midiPitchSmooth;
    const auto ratio=midiPitchCurrent*std::pow(2.0f,(p.hatTune+pitchModulation)/12.0f);
    const auto safeMaximum=static_cast<float>(sr*.45);float metal=0;maximumOscillatorFrequency=0;
    for(size_t i=0;i<6;++i){const auto frequency=juce::jmin(base[i]*ratio,safeMaximum);
        maximumOscillatorFrequency=juce::jmax(maximumOscillatorFrequency,frequency);
        const auto dt=frequency/static_cast<float>(sr);auto t=(float)phases[i];
        metal += ((t<.5f?1.0f:-1.0f)+polyBlep(t,(float)dt)-polyBlep(std::fmod(t+.5f,1.0f),(float)dt))/6;
        phases[i]=std::fmod(phases[i]+dt,1.0);}
    lowBand.setLowPass(3440,sr); highBand.setLowPass(7100,sr); hp.setLowPass(p.hatHP,sr);
    auto lo=lowBand.high(metal);auto hi=highBand.high(metal);auto source=juce::jmap(p.hatTone,lo,hi);
    source=hp.high(source);float sum=0;
    for(auto&e:envs){if(e.level>1e-5f){sum+=source*e.level;e.level*=e.mul*e.chokeMul;}}
    return sum*p.hatLevel;
}

void DrumEngine::prepare(double s,int){sr=s;for(auto&v:kicks)v.prepare(s);for(auto&v:snares)v.prepare(s);hats.prepare(s);dcBlock.setLowPass(20,s);reset();}
void DrumEngine::reset()
{
    for(auto&v:kicks)v.prepare(sr);for(auto&v:snares)v.prepare(sr);hats.reset();
    lfoPhases.fill(0);lfoOutputs.fill(0);lastTraceBins.fill(-1);lastShapes.fill(-1);
    pitchModulation=decayModulation=0;masterGain=0;dcBlock.reset();
    for(size_t i=0;i<lfoCount;++i){initialiseTrace(i,0);lfoVisuals[i].phase.store(0);lfoVisuals[i].output.store(0);}
}
void DrumEngine::trigger(Instrument i,float v,float pitchMultiplier,const DrumParameters&p)
{
    if(i==kick)kicks[kickCursor++%kicks.size()].trigger(v,pitchMultiplier,p);
    else if(i==snare)snares[snareCursor++%snares.size()].trigger(v,pitchMultiplier,p);
    else hats.trigger(v,pitchMultiplier,p,decayModulation);
}

void DrumEngine::initialiseTrace(size_t index,int shape) noexcept
{
    for(size_t bin=0;bin<lfoTraceSize;++bin)
        lfoVisuals[index].trace[bin].store(
            evaluateLfoWaveform(shape,static_cast<float>(bin)/static_cast<float>(lfoTraceSize)),
            std::memory_order_relaxed);
}

void DrumEngine::advanceLfos(const DrumParameters&p) noexcept
{
    const auto previousOutputs=lfoOutputs;
    std::array<float,lfoCount> nextOutputs {};
    for(size_t i=0;i<lfoCount;++i)
    {
        const auto& parameters=p.lfos[i];
        if(parameters.shape!=lastShapes[i])
        {
            if(lastShapes[i]<0)initialiseTrace(i,parameters.shape);
            lastShapes[i]=parameters.shape;lastTraceBins[i]=-1;
        }

        const auto rate=std::isfinite(parameters.rate)?juce::jlimit(.05f,20.0f,parameters.rate):1.0f;
        lfoPhases[i]+=static_cast<double>(rate)/sr;
        if(lfoPhases[i]>=1.0)lfoPhases[i]-=std::floor(lfoPhases[i]);

        float sourceOutput=0;
        if(parameters.modSource>=0&&parameters.modSource<static_cast<int>(lfoCount)
           &&parameters.modSource!=static_cast<int>(i))
            sourceOutput=previousOutputs[static_cast<size_t>(parameters.modSource)];
        const auto warp=std::isfinite(parameters.warp)?juce::jlimit(-1.0f,1.0f,parameters.warp):0.0f;
        const auto warpedPhase=static_cast<float>(lfoPhases[i])+sourceOutput*warp*.25f;
        auto output=evaluateLfoWaveform(parameters.shape,warpedPhase);
        if(!std::isfinite(output))output=0;
        nextOutputs[i]=juce::jlimit(-1.0f,1.0f,output);

        const auto bin=juce::jlimit(0,static_cast<int>(lfoTraceSize)-1,
                                    static_cast<int>(lfoPhases[i]*static_cast<double>(lfoTraceSize)));
        if(bin!=lastTraceBins[i])
        {
            lfoVisuals[i].trace[static_cast<size_t>(bin)].store(nextOutputs[i],std::memory_order_relaxed);
            lastTraceBins[i]=bin;
        }
        lfoVisuals[i].phase.store(static_cast<float>(lfoPhases[i]),std::memory_order_relaxed);
        lfoVisuals[i].output.store(nextOutputs[i],std::memory_order_relaxed);
    }
    lfoOutputs=nextOutputs;

    float pitch=0,decay=0;
    for(size_t i=0;i<lfoCount;++i)
    {
        pitch+=lfoOutputs[i]*p.lfos[i].pitchDepth;
        decay+=lfoOutputs[i]*p.lfos[i].decayDepth;
    }
    pitchModulation=std::isfinite(pitch)?juce::jlimit(-24.0f,24.0f,pitch):0;
    decayModulation=std::isfinite(decay)?juce::jlimit(-1.0f,1.0f,decay):0;
}

void DrumEngine::getLfoSnapshot(size_t index,LfoDisplaySnapshot& snapshot) const noexcept
{
    if(index>=lfoCount)return;
    snapshot.phase=lfoVisuals[index].phase.load(std::memory_order_relaxed);
    snapshot.output=lfoVisuals[index].output.load(std::memory_order_relaxed);
    for(size_t bin=0;bin<lfoTraceSize;++bin)
        snapshot.trace[bin]=lfoVisuals[index].trace[bin].load(std::memory_order_relaxed);
}

void DrumEngine::render(juce::AudioBuffer<float>&b,int start,int count,const DrumParameters&p)
{
    const auto target=p.master;const auto smooth=1-std::exp(-1.0f/(.01f*(float)sr));
    for(int n=0;n<count;++n){
        advanceLfos(p);float x=0;for(auto&v:kicks)x+=v.render();for(auto&v:snares)x+=v.render();x+=hats.render(p,pitchModulation);
        x=dcBlock.high(x);masterGain+=(target-masterGain)*smooth;x=std::tanh(x*1.25f)/std::tanh(1.25f)*masterGain;
        for(int ch=0;ch<b.getNumChannels();++ch)b.addSample(ch,start+n,x);
    }
}
