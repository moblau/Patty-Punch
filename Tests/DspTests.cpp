#include <juce_core/juce_core.h>
#include "DrumEngine.h"
#include "Parameters.h"
#include <iostream>
#include <stdexcept>

namespace
{
void require (bool ok, const char* message) { if (!ok) throw std::runtime_error (message); }
struct Stats { double energy=0, early=0, late=0, hf=0; float peak=0; };
Stats renderHit (DrumEngine::Instrument which, DrumParameters p, double sr, int block, float velocity=1,
                 int offset=0, double seconds=3)
{
    DrumEngine e;e.prepare(sr,block);juce::AudioBuffer<float>b(2,(int)(sr*seconds));b.clear();
    e.render(b,0,offset,p);e.trigger(which,velocity,p);e.render(b,offset,b.getNumSamples()-offset,p);
    Stats s;float prev=0;
    for(int i=0;i<b.getNumSamples();++i){auto x=b.getSample(0,i);require(std::isfinite(x),"non-finite sample");
        s.energy+=x*x;if(i<(int)(.02*sr))s.early+=x*x;if(i>(int)(.7*sr))s.late+=x*x;s.hf+=(x-prev)*(x-prev);prev=x;s.peak=juce::jmax(s.peak,std::abs(x));}
    return s;
}
void runRate (double sr, int block)
{
    DrumParameters p;
    for(auto i:{DrumEngine::kick,DrumEngine::snare,DrumEngine::hat})
    {auto s=renderHit(i,p,sr,block);require(s.energy>1e-5,"voice was silent");require(s.peak<1.1f,"runaway output");}
}
}

int main()
{
    try
    {
        for(auto sr:{44100.0,48000.0,96000.0})for(auto block:{1,64,257,1024})runRate(sr,block);
        DrumParameters p;
        auto quiet=renderHit(DrumEngine::kick,p,48000,128,.2f);auto loud=renderHit(DrumEngine::kick,p,48000,128,1);
        require(loud.energy>quiet.energy*2,"velocity did not alter level");
        p.kickDecay=.12f;auto shortKick=renderHit(DrumEngine::kick,p,48000,128);p.kickDecay=3;auto longKick=renderHit(DrumEngine::kick,p,48000,128);
        require(longKick.late>shortKick.late*10,"kick decay did not lengthen tail");
        p=DrumParameters{};p.kickClick=0;auto round=renderHit(DrumEngine::kick,p,48000,128);p.kickClick=1;auto clicky=renderHit(DrumEngine::kick,p,48000,128);
        require(clicky.hf>round.hf,"kick click did not add transient energy");
        p=DrumParameters{};p.snareSnappy=.05f;auto tonal=renderHit(DrumEngine::snare,p,48000,128);p.snareSnappy=1;auto snappy=renderHit(DrumEngine::snare,p,48000,128);
        require(snappy.hf>tonal.hf,"snappy did not add high-frequency energy");
        p=DrumParameters{};auto hat=renderHit(DrumEngine::hat,p,48000,128);require(hat.hf/hat.energy>.05,"hat lacks metallic high-frequency content");
        auto offset=renderHit(DrumEngine::kick,p,48000,128,1,777,1);require(offset.energy>0,"offset hit silent");

        struct Dummy final:juce::AudioProcessor{
            Dummy():AudioProcessor(BusesProperties()){}const juce::String getName()const override{return"test";}void prepareToPlay(double,int)override{}void releaseResources()override{}
            void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override{}bool isBusesLayoutSupported(const BusesLayout&)const override{return true;}
            juce::AudioProcessorEditor*createEditor()override{return nullptr;}bool hasEditor()const override{return false;}double getTailLengthSeconds()const override{return 0;}
            bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
            int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return{};}
            void changeProgramName(int,const juce::String&)override{}void getStateInformation(juce::MemoryBlock&)override{}void setStateInformation(const void*,int)override{}
        } dummy;
        juce::AudioProcessorValueTreeState state(dummy,nullptr,"state",Params::createLayout());Params::ensureMidiProperties(state.state);
        state.state.setProperty(Params::kickNote,36,nullptr);auto xml=state.copyState().createXml();auto restored=juce::ValueTree::fromXml(*xml);
        require((int)restored.getProperty(Params::kickNote)==36,"MIDI mapping state failed");
        for(auto*id:{Params::kickTune,Params::kickDecay,Params::snareTune,Params::hatDecay,Params::masterLevel})
        {auto*param=state.getParameter(id);require(param!=nullptr,"missing parameter");param->setValueNotifyingHost(-10);require(param->getValue()>=0,"parameter failed low clamp");param->setValueNotifyingHost(10);require(param->getValue()<=1,"parameter failed high clamp");}
        std::cout<<"Patty Punch DSP tests passed\n";return 0;
    }
    catch(const std::exception&e){std::cerr<<"FAILED: "<<e.what()<<'\n';return 1;}
}
