#include <juce_core/juce_core.h>
#include "DrumEngine.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

namespace
{
void require (bool ok, const char* message) { if (!ok) throw std::runtime_error (message); }
struct Stats { double energy=0, early=0, late=0, hf=0; float peak=0; };
Stats renderHit (DrumEngine::Instrument which, DrumParameters p, double sr, int block, float velocity=1,
                 int offset=0, double seconds=3, float pitchMultiplier=1)
{
    DrumEngine e;e.prepare(sr,block);juce::AudioBuffer<float>b(2,(int)(sr*seconds));b.clear();
    e.render(b,0,offset,p);e.trigger(which,velocity,pitchMultiplier,p);e.render(b,offset,b.getNumSamples()-offset,p);
    Stats s;float prev=0;
    for(int i=0;i<b.getNumSamples();++i){auto x=b.getSample(0,i);require(std::isfinite(x),"non-finite sample");
        s.energy+=x*x;if(i<(int)(.02*sr))s.early+=x*x;if(i>(int)(.7*sr))s.late+=x*x;s.hf+=(x-prev)*(x-prev);prev=x;s.peak=juce::jmax(s.peak,std::abs(x));}
    return s;
}

struct ProcessorResult
{
    double energy=0;
    std::array<uint32_t,3> activity {};
    juce::AudioBuffer<float> audio { 2, 4096 };
};

ProcessorResult processNote (int note, float velocity=1.0f, int sampleOffset=0)
{
    PattyPunchAudioProcessor processor;
    processor.setPlayConfigDetails(0,2,48000.0,4096);
    processor.prepareToPlay(48000.0,4096);
    ProcessorResult result;
    result.audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,note,velocity),sampleOffset);
    processor.processBlock(result.audio,midi);
    for(int i=0;i<result.audio.getNumSamples();++i)
    {
        const auto x=result.audio.getSample(0,i);
        require(std::isfinite(x),"processor emitted non-finite sample");
        result.energy+=x*x;
    }
    for(int i=0;i<3;++i)result.activity[(size_t)i]=processor.getActivityCounter((DrumEngine::Instrument)i);
    return result;
}

double estimateFrequency (KickVoice& voice, double sr, int skipSamples, int count)
{
    float previous=0;int crossings=0;
    for(int i=0;i<skipSamples+count;++i)
    {
        const auto x=voice.render();
        if(i>=skipSamples&&previous<=0&&x>0)++crossings;
        previous=x;
    }
    return crossings*sr/count;
}

float renderedHatFrequencyCeiling (float multiplier)
{
    DrumParameters p;p.master=1;p.lfos[0].pitchDepth=0;p.hatTune=0;
    DrumEngine engine;engine.prepare(48000,512);juce::AudioBuffer<float>b(1,8192);b.clear();
    engine.trigger(DrumEngine::hat,1,multiplier,p);engine.render(b,0,b.getNumSamples(),p);
    return engine.getHatForTests().getMaximumOscillatorFrequencyForTests();
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
        juce::ScopedJuceInitialiser_GUI juceInitialiser;
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

        for(int shape=0;shape<3;++shape)for(int step=-32;step<=160;++step)
        {
            const auto value=evaluateLfoWaveform(shape,step/128.0f);
            require(std::isfinite(value)&&value>=-1.0f&&value<=1.0f,"waveform evaluator escaped its bounds");
        }
        require(std::abs(evaluateLfoWaveform(0,.25f)-1.0f)<1.0e-6f,"sine evaluator mismatch");
        require(std::abs(evaluateLfoWaveform(1,.5f)-1.0f)<1.0e-6f,"triangle evaluator mismatch");
        require(evaluateLfoWaveform(2,.49f)==1.0f&&evaluateLfoWaveform(2,.5f)==-1.0f,"square evaluator mismatch");

        DrumParameters legacyLfo;legacyLfo.lfos[0].rate=1;legacyLfo.lfos[0].shape=1;
        legacyLfo.lfos[0].pitchDepth=12;legacyLfo.lfos[0].decayDepth=.4f;
        DrumEngine legacyEngine;legacyEngine.prepare(100,1);juce::AudioBuffer<float>singleSample(1,1);singleSample.clear();
        legacyEngine.render(singleSample,0,1,legacyLfo);LfoDisplaySnapshot legacySnapshot;legacyEngine.getLfoSnapshot(0,legacySnapshot);
        require(std::abs(legacySnapshot.output-evaluateLfoWaveform(1,.01f))<1.0e-6f,"LFO 1 no longer follows the legacy phase timing");
        require(std::abs(legacyEngine.getPitchModulationForTests()-legacySnapshot.output*12)<1.0e-5f,"LFO 1 pitch behavior changed");
        require(std::abs(legacyEngine.getDecayModulationForTests()-legacySnapshot.output*.4f)<1.0e-5f,"LFO 1 decay behavior changed");

        DrumParameters crossMod;for(auto&parameters:crossMod.lfos)parameters.rate=10;
        crossMod.lfos[0].shape=2;crossMod.lfos[1].shape=0;crossMod.lfos[1].modSource=0;crossMod.lfos[1].warp=1;
        DrumEngine crossEngine;crossEngine.prepare(100,1);singleSample.clear();crossEngine.render(singleSample,0,1,crossMod);
        LfoDisplaySnapshot firstTarget;crossEngine.getLfoSnapshot(1,firstTarget);
        require(std::abs(firstTarget.output-evaluateLfoWaveform(0,.1f))<1.0e-5f,"cross-mod did not use the source's previous sample");
        singleSample.clear();crossEngine.render(singleSample,0,1,crossMod);LfoDisplaySnapshot secondTarget;crossEngine.getLfoSnapshot(1,secondTarget);
        require(std::abs(secondTarget.output-evaluateLfoWaveform(0,.45f))<1.0e-5f,"quarter-cycle positive phase warp mismatch");
        const auto targetBin=static_cast<size_t>(secondTarget.phase*lfoTraceSize);
        require(std::abs(secondTarget.trace[targetBin]-secondTarget.output)<1.0e-6f,"published trace did not contain the post-modulated output");

        DrumParameters feedback;feedback.lfos[0].rate=20;feedback.lfos[0].shape=1;feedback.lfos[0].modSource=1;feedback.lfos[0].warp=1;
        feedback.lfos[1].rate=.05f;feedback.lfos[1].shape=2;feedback.lfos[1].modSource=0;feedback.lfos[1].warp=-1;
        DrumEngine feedbackEngine;feedbackEngine.prepare(48000,512);juce::AudioBuffer<float>feedbackBuffer(1,48000);feedbackBuffer.clear();feedbackEngine.render(feedbackBuffer,0,feedbackBuffer.getNumSamples(),feedback);
        for(size_t i=0;i<lfoCount;++i){LfoDisplaySnapshot snapshot;feedbackEngine.getLfoSnapshot(i,snapshot);require(std::isfinite(snapshot.output)&&std::abs(snapshot.output)<=1,"feedback loop became unstable");}

        DrumParameters summed;for(auto&parameters:summed.lfos){parameters.rate=1;parameters.shape=2;parameters.pitchDepth=24;parameters.decayDepth=1;}
        DrumEngine summedEngine;summedEngine.prepare(100,1);singleSample.clear();summedEngine.render(singleSample,0,1,summed);
        require(summedEngine.getPitchModulationForTests()==24,"combined pitch modulation was not safely clamped");
        require(summedEngine.getDecayModulationForTests()==1,"combined decay modulation was not safely clamped");

        DrumParameters selfMod;selfMod.lfos[2].rate=10;selfMod.lfos[2].shape=0;selfMod.lfos[2].modSource=2;selfMod.lfos[2].warp=1;
        DrumEngine selfEngine;selfEngine.prepare(100,1);singleSample.clear();selfEngine.render(singleSample,0,1,selfMod);LfoDisplaySnapshot selfSnapshot;selfEngine.getLfoSnapshot(2,selfSnapshot);
        require(std::abs(selfSnapshot.output-evaluateLfoWaveform(0,.1f))<1.0e-5f,"self-modulation was not safely ignored");

        struct Boundary { int note; bool valid; DrumEngine::Instrument instrument; int semitones; };
        const Boundary boundaries[] {
            {23,false,DrumEngine::kick,0},{24,true,DrumEngine::kick,0},
            {35,true,DrumEngine::kick,11},{36,true,DrumEngine::kick,12},
            {47,true,DrumEngine::kick,23},{48,true,DrumEngine::snare,0},
            {59,true,DrumEngine::snare,11},{60,true,DrumEngine::hat,0},
            {71,true,DrumEngine::hat,11},{72,false,DrumEngine::hat,0}
        };
        for(const auto& expected:boundaries)
        {
            PattyPunchAudioProcessor::MidiZoneHit routed;
            const auto valid=PattyPunchAudioProcessor::routeMidiNote(expected.note,routed);
            require(valid==expected.valid,"zone validity mismatch");
            auto processed=processNote(expected.note);
            if(!valid){require(processed.energy==0,"out-of-zone note produced audio");continue;}
            require(routed.instrument==expected.instrument,"wrong instrument at zone boundary");
            require(routed.semitoneOffset==expected.semitones,"wrong zone semitone offset");
            require(std::abs(routed.pitchMultiplier-std::pow(2.0f,expected.semitones/12.0f))<1.0e-6f,"wrong pitch multiplier");
            int activitySum=0;for(auto count:processed.activity)activitySum+=(int)count;
            require(activitySum==1,"zone boundary triggered multiple instruments");
            require(processed.activity[(size_t)expected.instrument]==1,"processor triggered wrong instrument");
            require(processed.energy>0,"in-zone processor note was silent");
        }

        KickVoice rootKick,octaveKick;rootKick.prepare(48000);octaveKick.prepare(48000);
        p=DrumParameters{};p.kickSweep=30;p.kickClick=0;p.kickDrive=0;p.kickDecay=2;
        rootKick.trigger(1,1,p);octaveKick.trigger(1,2,p);
        const auto rootSettled=estimateFrequency(rootKick,48000,12000,24000);
        const auto octaveSettled=estimateFrequency(octaveKick,48000,12000,24000);
        require(std::abs(octaveSettled/rootSettled-2.0)<.04,"Kick C1 did not settle near twice Kick C0");

        DrumEngine overlapping;overlapping.prepare(48000,256);overlapping.trigger(DrumEngine::kick,1,1,p);
        overlapping.trigger(DrumEngine::kick,1,std::pow(2.0f,7.0f/12.0f),p);
        const auto kickFrequencies=overlapping.getKickFinalFrequenciesForTests();
        require(std::abs(kickFrequencies[0]-p.kickTune)<.01f,"first overlapping kick lost pitch");
        require(std::abs(kickFrequencies[1]-p.kickTune*std::pow(2.0f,7.0f/12.0f))<.01f,"second overlapping kick lost pitch");

        SnareVoice lowSnare,highSnare;lowSnare.prepare(48000);highSnare.prepare(48000);
        p=DrumParameters{};lowSnare.trigger(1,1,p);highSnare.trigger(1,std::pow(2.0f,11.0f/12.0f),p);
        const auto lowFrequencies=lowSnare.getTonalFrequenciesForTests(),highFrequencies=highSnare.getTonalFrequenciesForTests();
        const auto snareRatio=std::pow(2.0f,11.0f/12.0f);
        require(std::abs(highFrequencies.first/lowFrequencies.first-snareRatio)<1.0e-5f,"snare fundamental was not transposed");
        require(std::abs(highFrequencies.second/lowFrequencies.second-snareRatio)<1.0e-5f,"snare inharmonic oscillator was not transposed");
        require(renderedHatFrequencyCeiling(snareRatio)>renderedHatFrequencyCeiling(1)*1.5f,
                "higher hat note did not raise metallic oscillator spectrum");

        auto processorQuiet=processNote(24,.2f),processorLoud=processNote(24,1);
        require(processorLoud.energy>processorQuiet.energy*2,"processor velocity response failed");
        auto delayed=processNote(24,1,777);
        for(int i=0;i<777;++i)require(delayed.audio.getSample(0,i)==0,"MIDI triggered before sample offset");
        require(delayed.audio.getMagnitude(0,777,delayed.audio.getNumSamples()-777)>0,"sample-offset note did not trigger");
        auto zeroVelocity=processNote(24,0);
        require(zeroVelocity.energy==0,"Note On velocity zero triggered a hit");

        DrumParameters extreme;extreme.hatTune=24;extreme.lfos[0].pitchDepth=24;extreme.lfos[0].rate=20;
        DrumEngine extremeHat;extremeHat.prepare(44100,64);juce::AudioBuffer<float>extremeBuffer(1,44100);extremeBuffer.clear();
        extremeHat.trigger(DrumEngine::hat,1,std::pow(2.0f,11.0f/12.0f),extreme);
        extremeHat.render(extremeBuffer,0,extremeBuffer.getNumSamples(),extreme);
        for(int i=0;i<extremeBuffer.getNumSamples();++i)require(std::isfinite(extremeBuffer.getSample(0,i)),"extreme hat pitch became non-finite");
        require(extremeHat.getHatForTests().getMaximumOscillatorFrequencyForTests()<=44100*.45f+1,"hat oscillator exceeded safe Nyquist limit");

        struct Dummy final:juce::AudioProcessor{
            Dummy():AudioProcessor(BusesProperties()){}const juce::String getName()const override{return"test";}void prepareToPlay(double,int)override{}void releaseResources()override{}
            void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override{}bool isBusesLayoutSupported(const BusesLayout&)const override{return true;}
            juce::AudioProcessorEditor*createEditor()override{return nullptr;}bool hasEditor()const override{return false;}double getTailLengthSeconds()const override{return 0;}
            bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
            int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return{};}
            void changeProgramName(int,const juce::String&)override{}void getStateInformation(juce::MemoryBlock&)override{}void setStateInformation(const void*,int)override{}
        } dummy;
        juce::AudioProcessorValueTreeState state(dummy,nullptr,"state",Params::createLayout());Params::ensureMidiProperties(state.state);
        const char* newIds[]{Params::lfo1ModFrom,Params::lfo1Warp,Params::lfo2Rate,Params::lfo2Shape,Params::lfo2Pitch,Params::lfo2Decay,Params::lfo2ModFrom,Params::lfo2Warp,
                             Params::lfo3Rate,Params::lfo3Shape,Params::lfo3Pitch,Params::lfo3Decay,Params::lfo3ModFrom,Params::lfo3Warp,
                             Params::lfo4Rate,Params::lfo4Shape,Params::lfo4Pitch,Params::lfo4Decay,Params::lfo4ModFrom,Params::lfo4Warp};
        for(auto*id:newIds)require(state.getParameter(id)!=nullptr,"missing WARBLER parameter");
        for(auto*id:{Params::lfo2Rate,Params::lfo3Rate,Params::lfo4Rate})require(std::abs(state.getRawParameterValue(id)->load()-1)<1.0e-6f,"new LFO rate default changed");
        for(auto*id:{Params::lfo1ModFrom,Params::lfo1Warp,Params::lfo2Pitch,Params::lfo2Decay,Params::lfo2ModFrom,Params::lfo2Warp,
                     Params::lfo3Pitch,Params::lfo3Decay,Params::lfo3ModFrom,Params::lfo3Warp,Params::lfo4Pitch,Params::lfo4Decay,Params::lfo4ModFrom,Params::lfo4Warp})
            require(state.getRawParameterValue(id)->load()==0,"new LFO neutral default changed");
        require(Params::modSourceForChoice(0,1)==1&&Params::modSourceForChoice(1,1)==0&&Params::modSourceForChoice(2,3)==3&&Params::modSourceForChoice(3,3)==2,"mod-source choice mapping failed");
        for(size_t target=0;target<lfoCount;++target)for(int choice=1;choice<=3;++choice)require(Params::modSourceForChoice(target,choice)!=static_cast<int>(target),"UI offered self-modulation");
        const auto& ordered=dummy.getParameters();require(ordered.size()>=27,"legacy parameter list shrank");
        const char* legacyIds[]{Params::lfoRate,Params::lfoShape,Params::lfoPitch,Params::lfoDecay,Params::masterLevel};
        for(int i=0;i<5;++i){auto*withId=dynamic_cast<juce::AudioProcessorParameterWithID*>(ordered[static_cast<size_t>(22+i)]);require(withId!=nullptr&&withId->paramID==legacyIds[i],"legacy automation parameter ordering changed");}

        PattyPunchAudioProcessor migrationProcessor;auto oldState=migrationProcessor.apvts.copyState();
        for(auto*id:newIds){auto child=oldState.getChildWithProperty("id",id);if(child.isValid())oldState.removeChild(child,nullptr);}
        oldState.getChildWithProperty("id",Params::lfoRate).setProperty("value",3.25f,nullptr);
        juce::MemoryBlock oldBinary;if(auto oldXml=oldState.createXml())juce::AudioProcessor::copyXmlToBinary(*oldXml,oldBinary);
        migrationProcessor.apvts.getParameter(Params::lfo2Rate)->setValueNotifyingHost(1);
        migrationProcessor.apvts.getParameter(Params::lfo2Pitch)->setValueNotifyingHost(1);
        migrationProcessor.setStateInformation(oldBinary.getData(),static_cast<int>(oldBinary.getSize()));
        require(std::abs(migrationProcessor.apvts.getRawParameterValue(Params::lfoRate)->load()-3.25f)<1.0e-6f,"old LFO 1 state was not preserved");
        require(std::abs(migrationProcessor.apvts.getRawParameterValue(Params::lfo2Rate)->load()-1)<1.0e-6f,"missing LFO rate did not migrate to its default");
        require(migrationProcessor.apvts.getRawParameterValue(Params::lfo2Pitch)->load()==0,"missing LFO depth did not migrate to neutral");
        state.state.setProperty(Params::kickNote,36,nullptr);auto xml=state.copyState().createXml();auto restored=juce::ValueTree::fromXml(*xml);
        require((int)restored.getProperty(Params::kickNote)==36,"MIDI mapping state failed");
        for(auto*id:{Params::kickTune,Params::kickDecay,Params::snareTune,Params::hatDecay,Params::masterLevel})
        {auto*param=state.getParameter(id);require(param!=nullptr,"missing parameter");param->setValueNotifyingHost(-10);require(param->getValue()>=0,"parameter failed low clamp");param->setValueNotifyingHost(10);require(param->getValue()<=1,"parameter failed high clamp");}
        std::cout<<"Patty Punch DSP tests passed\n";return 0;
    }
    catch(const std::exception&e){std::cerr<<"FAILED: "<<e.what()<<'\n';return 1;}
}
