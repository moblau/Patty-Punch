#include "DrumEngine.h"

static float decayMul (float seconds, double sr, float end=1.0e-4f)
{ return static_cast<float> (std::exp (std::log (end) / juce::jmax (1.0, seconds * sr))); }
static float noise (uint32_t& s) { s ^= s<<13; s ^= s>>17; s ^= s<<5; return (float) (int32_t) s / 2147483648.0f; }

void KickVoice::prepare (double s) { sr=s; clickLP.reset(); env=0; }
void KickVoice::trigger (float v, const DrumParameters& p)
{
    target=p.kickTune; freq=target*std::pow(2.0f,p.kickSweep/12.0f);
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
void SnareVoice::trigger(float v,const DrumParameters&p)
{
    f1=p.snareTune; f2=f1*1.607f; toneEnv=juce::jmap(v,.18f,1.0f);
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

void HatEngine::prepare(double s){sr=s;reset();}
void HatEngine::reset(){phases.fill(0);for(auto&e:envs)e={};lowBand.reset();highBand.reset();hp.reset();}
void HatEngine::trigger(float v,const DrumParameters&p,float lfo)
{
    if(p.hatChoke) for(auto&e:envs) if(e.level>0)e.chokeMul=decayMul(.004f,sr,.001f);
    auto&e=envs[(size_t)(cursor++%(int)envs.size())];
    const auto mult=std::pow(4.0f,lfo*p.lfoDecay);
    const auto d=juce::jlimit(.015f,2.5f,p.hatDecay*mult);
    e.level=juce::jmap(v,.18f,1.0f);e.mul=decayMul(d,sr);e.chokeMul=1;
}
static float polyBlep(float t,float dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
float HatEngine::render(const DrumParameters&p,float lfo)
{
    static constexpr float base[]{205.3f,304.4f,369.6f,522.7f,540,800};
    const auto ratio=std::pow(2.0f,(p.hatTune+lfo*p.lfoPitch)/12.0f);float metal=0;
    for(size_t i=0;i<6;++i){auto dt=base[i]*ratio/sr;auto t=(float)phases[i];
        metal += ((t<.5f?1.0f:-1.0f)+polyBlep(t,(float)dt)-polyBlep(std::fmod(t+.5f,1.0f),(float)dt))/6;
        phases[i]=std::fmod(phases[i]+dt,1.0);}
    lowBand.setLowPass(3440,sr); highBand.setLowPass(7100,sr); hp.setLowPass(p.hatHP,sr);
    auto lo=lowBand.high(metal);auto hi=highBand.high(metal);auto source=juce::jmap(p.hatTone,lo,hi);
    source=hp.high(source);float sum=0;
    for(auto&e:envs){if(e.level>1e-5f){sum+=source*e.level;e.level*=e.mul*e.chokeMul;}}
    return sum*p.hatLevel;
}

void DrumEngine::prepare(double s,int){sr=s;for(auto&v:kicks)v.prepare(s);for(auto&v:snares)v.prepare(s);hats.prepare(s);dcBlock.setLowPass(20,s);reset();}
void DrumEngine::reset(){for(auto&v:kicks)v.prepare(sr);for(auto&v:snares)v.prepare(sr);hats.reset();lfoPhase=0;masterGain=0;dcBlock.reset();}
void DrumEngine::trigger(Instrument i,float v,const DrumParameters&p)
{
    if(i==kick)kicks[kickCursor++%kicks.size()].trigger(v,p);
    else if(i==snare)snares[snareCursor++%snares.size()].trigger(v,p);
    else hats.trigger(v,p,lfoValue.load());
}
void DrumEngine::render(juce::AudioBuffer<float>&b,int start,int count,const DrumParameters&p)
{
    const auto target=p.master;const auto smooth=1-std::exp(-1.0f/(.01f*(float)sr));
    for(int n=0;n<count;++n){
        lfoPhase=std::fmod(lfoPhase+p.lfoRate/sr,1.0);float l;
        if(p.lfoShape==1)l=1.0f-4.0f*std::abs((float)lfoPhase-.5f);
        else if(p.lfoShape==2)l=lfoPhase<.5?1.0f:-1.0f;
        else l=static_cast<float> (std::sin(juce::MathConstants<double>::twoPi*lfoPhase));
        lfoValue.store(l,std::memory_order_relaxed);float x=0;for(auto&v:kicks)x+=v.render();for(auto&v:snares)x+=v.render();x+=hats.render(p,l);
        x=dcBlock.high(x);masterGain+=(target-masterGain)*smooth;x=std::tanh(x*1.25f)/std::tanh(1.25f)*masterGain;
        for(int ch=0;ch<b.getNumChannels();++ch)b.addSample(ch,start+n,x);
    }
}
