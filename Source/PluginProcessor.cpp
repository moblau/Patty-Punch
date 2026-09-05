#include "PluginProcessor.h"
#include "PluginEditor.h"

PattyPunchAudioProcessor::PattyPunchAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PattyPunchState", Params::createLayout())
{
    Params::ensureMidiProperties (apvts.state);
    const auto get=[this](const char* id)
    {
        auto* value=apvts.getRawParameterValue(id);jassert(value!=nullptr);return value;
    };
    raw.kickTune=get(Params::kickTune);raw.kickDecay=get(Params::kickDecay);
    raw.kickSweep=get(Params::kickSweep);raw.kickSweepTime=get(Params::kickSweepTime);
    raw.kickClick=get(Params::kickClick);raw.kickClickTone=get(Params::kickClickTone);
    raw.kickDrive=get(Params::kickDrive);raw.kickLevel=get(Params::kickLevel);
    raw.snareTune=get(Params::snareTune);raw.snareDecay=get(Params::snareDecay);
    raw.snareSnappy=get(Params::snareSnappy);raw.snareTone=get(Params::snareTone);
    raw.snareLow=get(Params::snareLow);raw.snareCrack=get(Params::snareCrack);
    raw.snareAir=get(Params::snareAir);raw.snareLevel=get(Params::snareLevel);
    raw.snareDrive=get(Params::snareDrive);
    raw.hatTune=get(Params::hatTune);raw.hatDecay=get(Params::hatDecay);
    raw.hatTone=get(Params::hatTone);raw.hatHighPass=get(Params::hatHighPass);
    raw.hatChoke=get(Params::hatChoke);raw.hatLevel=get(Params::hatLevel);
    raw.tomTune=get(Params::tomTune);raw.tomSweep=get(Params::tomSweep);
    raw.tomDecay=get(Params::tomDecay);raw.tomTone=get(Params::tomTone);
    raw.tomAttack=get(Params::tomAttack);raw.tomLevel=get(Params::tomLevel);
    const char* repeatCounts[]{Params::kickRepeatCount,Params::snareRepeatCount,Params::hatRepeatCount,Params::tomRepeatCount};
    const char* repeatTimes[]{Params::kickRepeatTime,Params::snareRepeatTime,Params::hatRepeatTime,Params::tomRepeatTime};
    const char* repeatShapes[]{Params::kickRepeatShape,Params::snareRepeatShape,Params::hatRepeatShape,Params::tomRepeatShape};
    for(size_t i=0;i<drumCount;++i)
        raw.repeats[i]={get(repeatCounts[i]),get(repeatTimes[i]),get(repeatShapes[i])};
    const char* rates[]{Params::lfoRate,Params::lfo2Rate,Params::lfo3Rate,Params::lfo4Rate};
    const char* shapes[]{Params::lfoShape,Params::lfo2Shape,Params::lfo3Shape,Params::lfo4Shape};
    const char* pitches[]{Params::lfoPitch,Params::lfo2Pitch,Params::lfo3Pitch,Params::lfo4Pitch};
    const char* decays[]{Params::lfoDecay,Params::lfo2Decay,Params::lfo3Decay,Params::lfo4Decay};
    const char* sources[]{Params::lfo1ModFrom,Params::lfo2ModFrom,Params::lfo3ModFrom,Params::lfo4ModFrom};
    const char* warps[]{Params::lfo1Warp,Params::lfo2Warp,Params::lfo3Warp,Params::lfo4Warp};
    for(size_t i=0;i<lfoCount;++i)
    {
        auto& lfo=raw.lfos[i];
        lfo.rate=get(rates[i]);lfo.shape=get(shapes[i]);lfo.pitch=get(pitches[i]);
        lfo.decay=get(decays[i]);lfo.modFrom=get(sources[i]);lfo.warp=get(warps[i]);
        lfo.targets={get(Params::lfoTargetA[i]),get(Params::lfoTargetB[i])};
        lfo.depths={get(Params::lfoDepthA[i]),get(Params::lfoDepthB[i])};
    }
    raw.masterLevel=get(Params::masterLevel);
}

PattyPunchAudioProcessor::~PattyPunchAudioProcessor()
{
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
    DrumParameters p;
    p.kickTune=raw.kickTune->load();p.kickDecay=raw.kickDecay->load();p.kickSweep=raw.kickSweep->load();
    p.kickSweepTime=raw.kickSweepTime->load()*.001f;p.kickClick=raw.kickClick->load()*.01f;
    p.kickClickTone=raw.kickClickTone->load();p.kickDrive=raw.kickDrive->load();
    p.kickLevelDb=raw.kickLevel->load();
    p.snareTune=raw.snareTune->load();p.snareDecay=raw.snareDecay->load()*.001f;p.snareSnappy=raw.snareSnappy->load()*.01f;
    p.snareTone=raw.snareTone->load();p.snareDrive=raw.snareDrive->load();
    p.snareLevelDb=raw.snareLevel->load();p.hatTune=raw.hatTune->load();
    p.hatDecay=raw.hatDecay->load()*.001f;p.hatTone=raw.hatTone->load()*.01f;p.hatHP=raw.hatHighPass->load();
    p.hatChoke=raw.hatChoke->load()>.5f;p.hatLevelDb=raw.hatLevel->load();
    p.tomTune=raw.tomTune->load();p.tomSweep=raw.tomSweep->load();p.tomDecay=raw.tomDecay->load()*.001f;
    p.tomTone=raw.tomTone->load()*.01f;p.tomAttack=raw.tomAttack->load()*.01f;p.tomLevelDb=raw.tomLevel->load();
    for(size_t i=0;i<drumCount;++i)
    {
        p.repeats[i].count=static_cast<int>(raw.repeats[i].count->load());
        p.repeats[i].timeSeconds=raw.repeats[i].time->load()*.001f;
        p.repeats[i].shape=raw.repeats[i].shape->load()*.01f;
    }
    for(size_t i=0;i<lfoCount;++i)
    {
        const auto& source=raw.lfos[i];auto& destination=p.lfos[i];
        destination.rate=source.rate->load();destination.shape=static_cast<int>(source.shape->load());
        destination.legacyHatPitchDepth=source.pitch->load();destination.legacyHatDecayDepth=source.decay->load()*.01f;
        destination.modSource=Params::modSourceForChoice(i,static_cast<int>(source.modFrom->load()));
        destination.warp=source.warp->load()*.01f;
        for(size_t route=0;route<2;++route)
        {
            const auto target=juce::jlimit(0,static_cast<int>(Params::ModDestination::count)-1,
                                           static_cast<int>(source.targets[route]->load()));
            destination.routes[route].destination=static_cast<Params::ModDestination>(target);
            destination.routes[route].depth=source.depths[route]->load()*.01f;
        }
    }
    p.master=juce::Decibels::decibelsToGain(raw.masterLevel->load());
    return p;
}

void PattyPunchAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals guard;
    b.clear();
    const auto p=snapshot();
    int s1,n1,s2,n2;
    uiFifo.prepareToRead(32,s1,n1,s2,n2);
    for(int k=0;k<n1;++k){auto h=uiHits[(size_t)(s1+k)];engine.trigger((DrumEngine::Instrument)h.instrument,h.velocity,h.pitchMultiplier,p);activity[(size_t)h.instrument]++;}
    for(int k=0;k<n2;++k){auto h=uiHits[(size_t)(s2+k)];engine.trigger((DrumEngine::Instrument)h.instrument,h.velocity,h.pitchMultiplier,p);activity[(size_t)h.instrument]++;}
    uiFifo.finishedRead(n1+n2);

    int rendered=0;
    for(const auto meta:midi)
    {
        const auto pos=juce::jlimit(0,b.getNumSamples(),meta.samplePosition);
        if(pos>rendered){engine.render(b,rendered,pos-rendered,p);rendered=pos;}
        const auto m=meta.getMessage();
        if(m.isNoteOn(false)&&m.getVelocity()>0)
        {
            MidiZoneHit hit;
            if(routeMidiNote(m.getNoteNumber(),hit))
            {
                engine.trigger(hit.instrument,m.getFloatVelocity(),hit.pitchMultiplier,p);
                activity[(size_t)hit.instrument]++;
            }
        }
    }
    if(rendered<b.getNumSamples())engine.render(b,rendered,b.getNumSamples()-rendered,p);
    float peak=0;for(int ch=0;ch<b.getNumChannels();++ch)peak=juce::jmax(peak,b.getMagnitude(ch,0,b.getNumSamples()));
    outputMeter.store(juce::jmax(peak,outputMeter.load()*.92f),std::memory_order_relaxed);
}

void PattyPunchAudioProcessor::enqueuePad (DrumEngine::Instrument i,float v)
{
    int s1,n1,s2,n2;uiFifo.prepareToWrite(1,s1,n1,s2,n2);
    if(n1){uiHits[(size_t)s1]={i,v,1.0f};uiFifo.finishedWrite(1);}
}

bool PattyPunchAudioProcessor::routeMidiNote(int midiNote,MidiZoneHit& hit) noexcept
{
    int root=0;
    if(midiNote>=24&&midiNote<=47){hit.instrument=DrumEngine::kick;root=24;}
    else if(midiNote>=48&&midiNote<=59){hit.instrument=DrumEngine::snare;root=48;}
    else if(midiNote>=60&&midiNote<=71){hit.instrument=DrumEngine::hat;root=60;}
    else if(midiNote>=72&&midiNote<=83){hit.instrument=DrumEngine::tom;root=72;}
    else return false;
    hit.semitoneOffset=midiNote-root;
    hit.pitchMultiplier=std::pow(2.0f,hit.semitoneOffset/12.0f);
    return true;
}
const char* PattyPunchAudioProcessor::zoneLabel(DrumEngine::Instrument i) noexcept
{
    static constexpr const char* labels[]{"C0-B1 | MIDI 24-47","C2-B2 | MIDI 48-59","C3-B3 | MIDI 60-71","C4-B4 | MIDI 72-83"};
    return labels[(size_t)i];
}
void PattyPunchAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{
    if(auto xml=apvts.copyState().createXml())copyXmlToBinary(*xml,d);
}
void PattyPunchAudioProcessor::setStateInformation(const void*d,int n)
{
    if(auto xml=getXmlFromBinary(d,n)){auto tree=juce::ValueTree::fromXml(*xml);if(tree.isValid()){Params::ensureCurrentState(tree);apvts.replaceState(tree);Params::ensureMidiProperties(apvts.state);}}
}
juce::AudioProcessorEditor* PattyPunchAudioProcessor::createEditor(){return new PattyPunchAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PattyPunchAudioProcessor();}
