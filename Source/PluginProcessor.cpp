#include "PluginProcessor.h"
#include "PluginEditor.h"

PattyPunchAudioProcessor::PattyPunchAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PattyPunchState", Params::createLayout())
{
    Params::ensureMidiProperties (apvts.state);
    apvts.state.addListener (this);
    valueTreePropertyChanged (apvts.state, {});
    const char* ids[] = { Params::kickTune,Params::kickDecay,Params::kickSweep,Params::kickSweepTime,
        Params::kickClick,Params::kickClickTone,Params::kickDrive,Params::kickLevel,
        Params::snareTune,Params::snareDecay,Params::snareSnappy,Params::snareTone,
        Params::snareLow,Params::snareCrack,Params::snareAir,Params::snareLevel,
        Params::hatTune,Params::hatDecay,Params::hatTone,Params::hatHighPass,Params::hatChoke,Params::hatLevel,
        Params::lfoRate,Params::lfoShape,Params::lfoPitch,Params::lfoDecay,Params::masterLevel };
    for (size_t i=0;i<raw.size();++i) raw[i]=apvts.getRawParameterValue(ids[i]);
}

PattyPunchAudioProcessor::~PattyPunchAudioProcessor()
{
    cancelPendingUpdate();
    apvts.state.removeListener (this);
}

void PattyPunchAudioProcessor::prepareToPlay (double sr, int block)
{
    engine.prepare (sr, block);
    outputMeter.store (0);
}
void PattyPunchAudioProcessor::releaseResources() { engine.reset(); }
bool PattyPunchAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o=l.getMainOutputChannelSet();
    return o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo();
}

DrumParameters PattyPunchAudioProcessor::snapshot() const
{
    DrumParameters p; size_t i=0;
    p.kickTune=raw[i++]->load();p.kickDecay=raw[i++]->load();p.kickSweep=raw[i++]->load();
    p.kickSweepTime=raw[i++]->load()*.001f;p.kickClick=raw[i++]->load()*.01f;
    p.kickClickTone=raw[i++]->load();p.kickDrive=raw[i++]->load();
    p.kickLevel=juce::Decibels::decibelsToGain(raw[i++]->load());
    p.snareTune=raw[i++]->load();p.snareDecay=raw[i++]->load()*.001f;p.snareSnappy=raw[i++]->load()*.01f;
    p.snareTone=raw[i++]->load();p.snareLow=raw[i++]->load();p.snareCrack=raw[i++]->load();p.snareAir=raw[i++]->load();
    p.snareLevel=juce::Decibels::decibelsToGain(raw[i++]->load());p.hatTune=raw[i++]->load();
    p.hatDecay=raw[i++]->load()*.001f;p.hatTone=raw[i++]->load()*.01f;p.hatHP=raw[i++]->load();
    p.hatChoke=raw[i++]->load()>.5f;p.hatLevel=juce::Decibels::decibelsToGain(raw[i++]->load());
    p.lfoRate=raw[i++]->load();p.lfoShape=(int)raw[i++]->load();p.lfoPitch=raw[i++]->load();
    p.lfoDecay=raw[i++]->load()*.01f;p.master=juce::Decibels::decibelsToGain(raw[i++]->load());
    return p;
}

void PattyPunchAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals guard;
    b.clear();
    const auto p=snapshot();
    int s1,n1,s2,n2;
    uiFifo.prepareToRead(32,s1,n1,s2,n2);
    for(int k=0;k<n1;++k){auto h=uiHits[(size_t)(s1+k)];engine.trigger((DrumEngine::Instrument)h.instrument,h.velocity,p);activity[(size_t)h.instrument]++;}
    for(int k=0;k<n2;++k){auto h=uiHits[(size_t)(s2+k)];engine.trigger((DrumEngine::Instrument)h.instrument,h.velocity,p);activity[(size_t)h.instrument]++;}
    uiFifo.finishedRead(n1+n2);

    int rendered=0;
    for(const auto meta:midi)
    {
        const auto pos=juce::jlimit(0,b.getNumSamples(),meta.samplePosition);
        if(pos>rendered){engine.render(b,rendered,pos-rendered,p);rendered=pos;}
        const auto m=meta.getMessage();
        if(m.isNoteOn()&&m.getVelocity()>0)
        {
            const auto note=m.getNoteNumber();const auto learning=learnTarget.exchange(-1);
            if(learning>=0){learnedNote.store(note);learnTarget.store(-2-learning);triggerAsyncUpdate();}
            for(int i=0;i<3;++i)if(note==midiNotes[(size_t)i].load()){engine.trigger((DrumEngine::Instrument)i,m.getFloatVelocity(),p);activity[(size_t)i]++;}
        }
    }
    if(rendered<b.getNumSamples())engine.render(b,rendered,b.getNumSamples()-rendered,p);
    float peak=0;for(int ch=0;ch<b.getNumChannels();++ch)peak=juce::jmax(peak,b.getMagnitude(ch,0,b.getNumSamples()));
    outputMeter.store(juce::jmax(peak,outputMeter.load()*.92f),std::memory_order_relaxed);
}

void PattyPunchAudioProcessor::enqueuePad (DrumEngine::Instrument i,float v)
{
    int s1,n1,s2,n2;uiFifo.prepareToWrite(1,s1,n1,s2,n2);
    if(n1){uiHits[(size_t)s1]={i,v};uiFifo.finishedWrite(1);}
}
void PattyPunchAudioProcessor::armMidiLearn(DrumEngine::Instrument i){learnTarget.store((int)i);}
int PattyPunchAudioProcessor::getMidiNote(DrumEngine::Instrument i)const{return midiNotes[(size_t)i].load();}
void PattyPunchAudioProcessor::setMidiNote(DrumEngine::Instrument i,int n)
{
    n=juce::jlimit(0,127,n);const char* ids[]={Params::kickNote,Params::snareNote,Params::hatNote};
    apvts.state.setProperty(ids[(size_t)i],n,nullptr);
}
void PattyPunchAudioProcessor::valueTreePropertyChanged(juce::ValueTree&,const juce::Identifier&)
{
    midiNotes[0].store((int)apvts.state.getProperty(Params::kickNote,60));
    midiNotes[1].store((int)apvts.state.getProperty(Params::snareNote,61));
    midiNotes[2].store((int)apvts.state.getProperty(Params::hatNote,62));
}
void PattyPunchAudioProcessor::handleAsyncUpdate()
{
    const auto encoded=learnTarget.load();const auto note=learnedNote.exchange(-1);
    if(encoded<=-2&&note>=0){setMidiNote((DrumEngine::Instrument)(-2-encoded),note);learnTarget.store(-1);}
}
void PattyPunchAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{
    if(auto xml=apvts.copyState().createXml())copyXmlToBinary(*xml,d);
}
void PattyPunchAudioProcessor::setStateInformation(const void*d,int n)
{
    if(auto xml=getXmlFromBinary(d,n)){auto tree=juce::ValueTree::fromXml(*xml);if(tree.isValid()){apvts.replaceState(tree);Params::ensureMidiProperties(apvts.state);valueTreePropertyChanged(apvts.state,{});}}
}
juce::AudioProcessorEditor* PattyPunchAudioProcessor::createEditor(){return new PattyPunchAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PattyPunchAudioProcessor();}
