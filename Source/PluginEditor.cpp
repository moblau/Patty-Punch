#include "PluginEditor.h"

static const auto cream=juce::Colour(0xffeee2c2), orange=juce::Colour(0xffe77732);
PattyLookAndFeel::PattyLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId,cream);setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff25262a));setColour(juce::ComboBox::textColourId,cream);
    setColour(juce::Label::textColourId,cream);setColour(juce::TextButton::textColourOffId,cream);
}
void PattyLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float pos,float a0,float a1,juce::Slider&s)
{
    auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(7);auto c=r.getCentre();auto rad=juce::jmin(r.getWidth(),r.getHeight())*.5f;
    g.setColour(juce::Colour(0xff101114));g.fillEllipse(r);g.setColour(juce::Colour(0xff55575d));g.drawEllipse(r,2);
    juce::Path p;p.addRoundedRectangle(-2,-rad+5,4,rad*.42f,2);
    g.setColour(s.findColour(juce::Slider::rotarySliderFillColourId));g.fillPath(p,juce::AffineTransform::rotation(a0+pos*(a1-a0)).translated(c.x,c.y));
}
void PattyLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&b,const juce::Colour&,bool over,bool down)
{
    auto r=b.getLocalBounds().toFloat().reduced(1);g.setColour(down?juce::Colour(0xfff3a33d):(over?juce::Colour(0xffb9512d):juce::Colour(0xff3a3b40)));
    g.fillRoundedRectangle(r,5);g.setColour(orange.withAlpha(.65f));g.drawRoundedRectangle(r,5,1);
}

ParamKnob::ParamKnob(juce::AudioProcessorValueTreeState&s,const juce::String&id,const juce::String&label,const juce::String&tip)
{
    name.setText(label,juce::dontSendNotification);name.setJustificationType(juce::Justification::centred);name.setFont(juce::FontOptions(11));
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,18);
    slider.setColour(juce::Slider::rotarySliderFillColourId,orange);
    slider.setDoubleClickReturnValue(true,s.getParameter(id)->getDefaultValue());slider.setTooltip(tip);slider.setWantsKeyboardFocus(true);
    addAndMakeVisible(name);addAndMakeVisible(slider);attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s,id,slider);
}
void ParamKnob::resized(){auto r=getLocalBounds();name.setBounds(r.removeFromTop(17));slider.setBounds(r);}

PerformancePad::PerformancePad(const juce::String&t):TextButton(t){setWantsKeyboardFocus(true);setTooltip("Trigger this instrument");}
void PerformancePad::flash(){flashFrames=5;repaint();}
void PerformancePad::paintButton(juce::Graphics&g,bool over,bool down)
{
    auto r=getLocalBounds().toFloat().reduced(3);auto lit=down||flashFrames>0;
    g.setColour(lit?juce::Colour(0xffffad34):over?juce::Colour(0xffd45e34):juce::Colour(0xff8f302b));g.fillRoundedRectangle(r,10);
    g.setColour(juce::Colour(0xff1b1513));g.drawRoundedRectangle(r,10,3);g.setColour(lit?juce::Colours::black:cream);
    g.setFont(juce::FontOptions(17,juce::Font::bold));g.drawText(getButtonText(),getLocalBounds(),juce::Justification::centred);
}

InstrumentPanel::InstrumentPanel(PattyPunchAudioProcessor&p,DrumEngine::Instrument i,const juce::String&t,
 std::initializer_list<std::tuple<const char*,const char*,const char*>> mainDefs,
 std::initializer_list<std::tuple<const char*,const char*,const char*>> repeatDefs):pad("PUNCH"),processor(p),instrument(i)
{
    title.setText(t,juce::dontSendNotification);title.setFont(juce::FontOptions(22,juce::Font::bold));title.setJustificationType(juce::Justification::centred);
    note.setJustificationType(juce::Justification::centred);note.setFont(juce::FontOptions(12));
    note.setText(PattyPunchAudioProcessor::zoneLabel(instrument),juce::dontSendNotification);
    repeatTitle.setText("REPEAT PERFORMANCE",juce::dontSendNotification);repeatTitle.setJustificationType(juce::Justification::centred);
    repeatTitle.setFont(juce::FontOptions(9,juce::Font::bold));repeatTitle.setColour(juce::Label::textColourId,cream.withAlpha(.6f));
    for(auto&d:mainDefs){auto k=std::make_unique<ParamKnob>(p.apvts,std::get<0>(d),std::get<1>(d),std::get<2>(d));addAndMakeVisible(*k);knobs.push_back(std::move(k));}
    mainControlCount=knobs.size();
    for(auto&d:repeatDefs){auto k=std::make_unique<ParamKnob>(p.apvts,std::get<0>(d),std::get<1>(d),std::get<2>(d));addAndMakeVisible(*k);knobs.push_back(std::move(k));}
    addAndMakeVisible(title);addAndMakeVisible(note);addAndMakeVisible(repeatTitle);addAndMakeVisible(pad);
    pad.onClick=[this]{processor.enqueuePad(instrument);};
    update();
}
void InstrumentPanel::resized()
{
    auto r=getLocalBounds().reduced(6);title.setBounds(r.removeFromTop(26));note.setBounds(r.removeFromTop(20));
    pad.setBounds(r.removeFromBottom(52).reduced(5));auto repeatArea=r.removeFromBottom(93);repeatTitle.setBounds(repeatArea.removeFromTop(14));
    const auto repeatCount=knobs.size()-mainControlCount;const auto repeatWidth=repeatArea.getWidth()/juce::jmax(1,(int)repeatCount);
    for(size_t i=0;i<repeatCount;++i)knobs[mainControlCount+i]->setBounds(repeatArea.getX()+(int)i*repeatWidth,repeatArea.getY(),repeatWidth,repeatArea.getHeight());
    const auto cols=3;const auto rows=((int)mainControlCount+cols-1)/cols;
    for(size_t i=0;i<mainControlCount;++i){auto w=r.getWidth()/cols;auto h=r.getHeight()/juce::jmax(1,rows);knobs[i]->setBounds(r.getX()+(int)i%cols*w,r.getY()+(int)i/cols*h,w,h);}
}
void InstrumentPanel::update()
{
    auto a=processor.getActivityCounter(instrument);if(a!=lastActivity){lastActivity=a;pad.flash();}
    if(pad.flashFrames>0){--pad.flashFrames;pad.repaint();}
}

LfoWaveform::LfoWaveform(juce::Colour colour):accent(colour){setInterceptsMouseClicks(false,false);}
void LfoWaveform::setSnapshot(const LfoDisplaySnapshot& next,int shape,bool modulationActive)
{
    auto changed=!hasSnapshot||shape!=baseShape||modulationActive!=showBaseShape
                 ||std::abs(next.phase-snapshot.phase)>1.0e-5f||std::abs(next.output-snapshot.output)>1.0e-4f;
    if(!changed)for(size_t i=0;i<lfoTraceSize;++i)if(std::abs(next.trace[i]-snapshot.trace[i])>1.0e-4f){changed=true;break;}
    if(!changed)return;
    snapshot=next;baseShape=shape;showBaseShape=modulationActive;hasSnapshot=true;repaint();
}
void LfoWaveform::paint(juce::Graphics&g)
{
    auto bounds=getLocalBounds().toFloat();g.setColour(juce::Colour(0xff0e1013));g.fillRoundedRectangle(bounds,6);
    auto plot=bounds.reduced(8,7);g.setColour(cream.withAlpha(.09f));g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()),plot.getX(),plot.getRight());
    for(int i=1;i<4;++i){const auto x=plot.getX()+plot.getWidth()*i*.25f;g.drawVerticalLine(juce::roundToInt(x),plot.getY(),plot.getBottom());}
    const auto yFor=[plot](float value){return plot.getCentreY()-juce::jlimit(-1.0f,1.0f,value)*plot.getHeight()*.43f;};
    if(showBaseShape)
    {
        juce::Path base;for(int x=0;x<=juce::roundToInt(plot.getWidth());++x){const auto phase=x/plot.getWidth();const auto point=juce::Point<float>(plot.getX()+x,yFor(evaluateLfoWaveform(baseShape,phase)));if(x==0)base.startNewSubPath(point);else base.lineTo(point);}
        g.setColour(cream.withAlpha(.14f));g.strokePath(base,juce::PathStrokeType(1.0f));
    }
    juce::Path processed;
    for(size_t i=0;i<lfoTraceSize;++i){const auto x=plot.getX()+plot.getWidth()*static_cast<float>(i)/static_cast<float>(lfoTraceSize-1);const auto point=juce::Point<float>(x,yFor(snapshot.trace[i]));if(i==0)processed.startNewSubPath(point);else processed.lineTo(point);}
    g.setColour(accent.withAlpha(.16f));g.strokePath(processed,juce::PathStrokeType(5.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(accent.withAlpha(.9f));g.strokePath(processed,juce::PathStrokeType(1.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    const auto dot=juce::Point<float>(plot.getX()+plot.getWidth()*juce::jlimit(0.0f,1.0f,snapshot.phase),yFor(snapshot.output));
    g.setColour(accent.withAlpha(.18f));g.fillEllipse(juce::Rectangle<float>(12,12).withCentre(dot));g.setColour(accent);g.fillEllipse(juce::Rectangle<float>(6,6).withCentre(dot));
}

LfoCard::LfoCard(PattyPunchAudioProcessor&p,size_t lfoIndex,juce::Colour colour,const char*rateId,const char*shapeId,
                 const char*modFromId,const char*warpId,const char*targetAId,const char*depthAId,
                 const char*targetBId,const char*depthBId)
:processor(p),index(lfoIndex),accent(colour),waveform(colour),rate(p.apvts,rateId,"RATE","Free-running LFO rate"),
 depthA(p.apvts,depthAId,"DEPTH A","Bipolar amount applied to Target A"),
 depthB(p.apvts,depthBId,"DEPTH B","Bipolar amount applied to Target B"),
 warp(p.apvts,warpId,"WARP","Bipolar phase-warp amount")
{
    title.setText("LFO "+juce::String(static_cast<int>(index+1)),juce::dontSendNotification);title.setFont(juce::FontOptions(15,juce::Font::bold));title.setColour(juce::Label::textColourId,accent);
    shapeLabel.setText("SHAPE",juce::dontSendNotification);modFromLabel.setText("MOD FROM",juce::dontSendNotification);
    targetALabel.setText("TARGET A",juce::dontSendNotification);targetBLabel.setText("TARGET B",juce::dontSendNotification);
    for(auto*label:{&shapeLabel,&modFromLabel,&targetALabel,&targetBLabel}){label->setFont(juce::FontOptions(9,juce::Font::bold));label->setJustificationType(juce::Justification::centredLeft);label->setColour(juce::Label::textColourId,cream.withAlpha(.65f));}
    shape.addItemList({"SINE","TRIANGLE","SQUARE"},1);shape.setTooltip("Base waveform before phase warp");
    auto sourceNames=Params::modSourceNames(index);for(auto&name:sourceNames)name=name.toUpperCase();modFrom.addItemList(sourceNames,1);modFrom.setTooltip("Choose the LFO that bends this waveform");
    auto destinations=Params::modDestinationNames();for(auto&name:destinations)name=name.toUpperCase();
    targetA.addItemList(destinations,1);targetB.addItemList(destinations,1);
    targetA.setTooltip("First modulation destination");targetB.setTooltip("Second modulation destination");
    shapeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,shapeId,shape);
    modFromAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,modFromId,modFrom);
    targetAAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,targetAId,targetA);
    targetBAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,targetBId,targetB);
    for(auto*knob:{&rate,&depthA,&depthB,&warp})knob->setAccentColour(accent);
    std::array<juce::Component*,14> components{&title,&waveform,&rate,&depthA,&depthB,&warp,&shapeLabel,&modFromLabel,&targetALabel,&targetBLabel,&shape,&modFrom,&targetA,&targetB};
    for(auto*component:components)addAndMakeVisible(component);
}
void LfoCard::paint(juce::Graphics&g)
{
    auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(0xff202227));g.fillRoundedRectangle(r,8);g.setColour(accent.withAlpha(.42f));g.drawRoundedRectangle(r,8,1);
    g.setColour(accent.withAlpha(.75f));g.fillRoundedRectangle(r.getX()+8,r.getY()+7,24,3,1.5f);
}
void LfoCard::resized()
{
    auto r=getLocalBounds().reduced(8);title.setBounds(r.removeFromTop(20));waveform.setBounds(r.removeFromTop(58));r.removeFromTop(2);
    auto knobs=r.removeFromTop(68);const auto knobWidth=knobs.getWidth()/4;rate.setBounds(knobs.removeFromLeft(knobWidth));depthA.setBounds(knobs.removeFromLeft(knobWidth));depthB.setBounds(knobs.removeFromLeft(knobWidth));warp.setBounds(knobs);
    r.removeFromTop(2);auto labels=r.removeFromTop(13);const auto half=labels.getWidth()/2;shapeLabel.setBounds(labels.removeFromLeft(half).reduced(3,0));modFromLabel.setBounds(labels.reduced(3,0));
    auto combos=r.removeFromTop(27);shape.setBounds(combos.removeFromLeft(combos.getWidth()/2).reduced(2,1));modFrom.setBounds(combos.reduced(2,1));
    auto targetLabels=r.removeFromTop(13);targetALabel.setBounds(targetLabels.removeFromLeft(targetLabels.getWidth()/2).reduced(3,0));targetBLabel.setBounds(targetLabels.reduced(3,0));
    auto targets=r.removeFromTop(27);targetA.setBounds(targets.removeFromLeft(targets.getWidth()/2).reduced(2,1));targetB.setBounds(targets.reduced(2,1));
}
void LfoCard::update()
{
    LfoDisplaySnapshot next;processor.getLfoSnapshot(index,next);
    waveform.setSnapshot(next,juce::jmax(0,shape.getSelectedItemIndex()),modFrom.getSelectedItemIndex()>0&&std::abs(warp.getValue())>.01);
}

PattyPunchAudioProcessorEditor::PattyPunchAudioProcessorEditor(PattyPunchAudioProcessor&p)
:AudioProcessorEditor(&p),audioProcessor(p),master(p.apvts,Params::masterLevel,"MASTER","Overall output level"),
 kick(p,DrumEngine::kick,"KICK",
      {{Params::kickTune,"PITCH","Fundamental pitch at the zone root"},{Params::kickSweep,"PUNCH","Initial downward pitch sweep"},{Params::kickDecay,"DECAY","Body decay time"},{Params::kickClick,"CLICK","Short filtered-noise transient"},{Params::kickDrive,"DRIVE","Compensated soft saturation"},{Params::kickLevel,"LEVEL","Kick output level"}},
      {{Params::kickRepeatCount,"REPEATS","Off or 1–10 synthesized hits after the original"},{Params::kickRepeatTime,"TIME","Neutral spacing; 10–500 ms"},{Params::kickRepeatShape,"SHAPE","Higher values pull early repeats forward and fade the burst faster"}}),
 snare(p,DrumEngine::snare,"SNARE",
      {{Params::snareTune,"PITCH","Fundamental shell pitch at the zone root"},{Params::snareDecay,"DECAY","Body and wire decay time"},{Params::snareSnappy,"SNAPPY","Tonal-body to noise-wire balance"},{Params::snareTone,"TONE","Noise-wire low-pass cutoff"},{Params::snareDrive,"DRIVE","Compensated soft saturation"},{Params::snareLevel,"LEVEL","Snare output level"}},
      {{Params::snareRepeatCount,"REPEATS","Off or 1–10 synthesized hits after the original"},{Params::snareRepeatTime,"TIME","Neutral spacing; 10–500 ms"},{Params::snareRepeatShape,"SHAPE","Higher values pull early repeats forward and fade the burst faster"}}),
 hat(p,DrumEngine::hat,"HI-HAT",
      {{Params::hatTune,"PITCH","Metallic oscillator-bank tuning"},{Params::hatDecay,"DECAY","Hi-hat envelope time"},{Params::hatTone,"TONE","Dark-to-bright metallic band blend"},{Params::hatHighPass,"CHARACTER","High-pass frequency and metallic weight"},{Params::hatLevel,"LEVEL","Hi-hat output level"}},
      {{Params::hatRepeatCount,"REPEATS","Off or 1–10 synthesized hits after the original"},{Params::hatRepeatTime,"TIME","Neutral spacing; 10–500 ms"},{Params::hatRepeatShape,"SHAPE","Higher values pull early repeats forward and fade the burst faster"}}),
 tom(p,DrumEngine::tom,"TOM",
      {{Params::tomTune,"PITCH","Fundamental electronic-tom pitch"},{Params::tomSweep,"BEND","Fast initial downward pitch bend"},{Params::tomDecay,"DECAY","Resonant body decay time"},{Params::tomTone,"DAMPING","Brightness, bend speed, and secondary-mode colour"},{Params::tomAttack,"ATTACK","Short synthesized stick-noise transient"},{Params::tomLevel,"LEVEL","Tom output level"}},
      {{Params::tomRepeatCount,"REPEATS","Off or 1–10 synthesized hits after the original"},{Params::tomRepeatTime,"TIME","Neutral spacing; 10–500 ms"},{Params::tomRepeatShape,"SHAPE","Higher values pull early repeats forward and fade the burst faster"}})
{
    setLookAndFeel(&look);logo.setText("PATTY PUNCH",juce::dontSendNotification);logo.setFont(juce::FontOptions(30,juce::Font::bold));subtitle.setText("FOUR-VOICE ANALOG DRUM SYNTHESIZER",juce::dontSendNotification);
    warblerTitle.setText("WARBLER",juce::dontSendNotification);warblerTitle.setFont(juce::FontOptions(18,juce::Font::bold));warblerTitle.setColour(juce::Label::textColourId,juce::Colour(0xffffd69b));
    warblerSubtitle.setText("4  CROSS-MOD LFO",juce::dontSendNotification);warblerSubtitle.setFont(juce::FontOptions(11,juce::Font::bold));warblerSubtitle.setColour(juce::Label::textColourId,cream.withAlpha(.6f));
    choke.setTooltip("Fade old hats when a new hat triggers");
    chokeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,Params::hatChoke,choke);
    const juce::Colour accents[]{juce::Colour(0xffffb58f),juce::Colour(0xff8fe3c3),juce::Colour(0xffaeb8ff),juce::Colour(0xffffdf8a)};
    const char*rates[]{Params::lfoRate,Params::lfo2Rate,Params::lfo3Rate,Params::lfo4Rate};const char*shapes[]{Params::lfoShape,Params::lfo2Shape,Params::lfo3Shape,Params::lfo4Shape};
    const char*sources[]{Params::lfo1ModFrom,Params::lfo2ModFrom,Params::lfo3ModFrom,Params::lfo4ModFrom};const char*warps[]{Params::lfo1Warp,Params::lfo2Warp,Params::lfo3Warp,Params::lfo4Warp};
    for(size_t i=0;i<lfoCount;++i)lfoCards[i]=std::make_unique<LfoCard>(p,i,accents[i],rates[i],shapes[i],sources[i],warps[i],Params::lfoTargetA[i],Params::lfoDepthA[i],Params::lfoTargetB[i],Params::lfoDepthB[i]);
    std::array<juce::Component*,11> components { &logo,&subtitle,&master,&kick,&snare,&hat,&tom,&warblerTitle,&warblerSubtitle,&choke,lfoCards[0].get() };
    for(auto* c:components)addAndMakeVisible(c);
    for(size_t i=1;i<lfoCount;++i)addAndMakeVisible(*lfoCards[i]);
    setWantsKeyboardFocus(true);setResizable(true,true);setResizeLimits(1180,760,1900,1200);setSize(1500,900);startTimerHz(36);
}
PattyPunchAudioProcessorEditor::~PattyPunchAudioProcessorEditor(){setLookAndFeel(nullptr);}
void PattyPunchAudioProcessorEditor::paint(juce::Graphics&g)
{
    g.fillAll(juce::Colour(0xff17181b));g.setColour(juce::Colour(0xff2a2b30));
    auto body=getLocalBounds().toFloat().reduced(10);g.fillRoundedRectangle(body,12);g.setColour(orange);g.drawRoundedRectangle(body,12,2);
    auto header=meterBounds.toFloat();g.setColour(juce::Colour(0xff090a0b));g.fillRoundedRectangle(header,4);
    g.setColour(juce::Colour(0xfff04b35));g.fillRoundedRectangle(header.withWidth(header.getWidth()*juce::jlimit(0.0f,1.0f,meter)),4);
}
void PattyPunchAudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(22);auto header=r.removeFromTop(78);logo.setBounds(header.removeFromLeft(290).removeFromTop(40));subtitle.setBounds(25,55,300,20);master.setBounds(header.removeFromRight(100));choke.setBounds(header.removeFromRight(96).reduced(8,24));
    meterBounds=header.withSizeKeepingCentre(juce::jmax(40,header.getWidth()-28),12);
    auto warbler=r.removeFromBottom(282);auto heading=warbler.removeFromTop(28);warblerTitle.setBounds(heading.removeFromLeft(112));warblerSubtitle.setBounds(heading.removeFromLeft(160));
    auto panels=r;auto w=panels.getWidth()/4;kick.setBounds(panels.removeFromLeft(w).reduced(4));snare.setBounds(panels.removeFromLeft(w).reduced(4));hat.setBounds(panels.removeFromLeft(w).reduced(4));tom.setBounds(panels.reduced(4));
    const auto cardWidth=warbler.getWidth()/static_cast<int>(lfoCount);for(size_t i=0;i<lfoCount;++i)lfoCards[i]->setBounds((i+1==lfoCount?warbler:warbler.removeFromLeft(cardWidth)).reduced(3));
}
bool PattyPunchAudioProcessorEditor::keyPressed(const juce::KeyPress&k)
{
    const auto c=juce::CharacterFunctions::toLowerCase(k.getTextCharacter());
    if(c=='a')audioProcessor.enqueuePad(DrumEngine::kick);else if(c=='s')audioProcessor.enqueuePad(DrumEngine::snare);else if(c=='d')audioProcessor.enqueuePad(DrumEngine::hat);else if(c=='f')audioProcessor.enqueuePad(DrumEngine::tom);else return false;return true;
}
void PattyPunchAudioProcessorEditor::timerCallback()
{
    const auto m=audioProcessor.getMeter();if(std::abs(m-meter)>.003f){meter=m;repaint(meterBounds.expanded(2));}
    for(auto&card:lfoCards)card->update();
    kick.update();snare.update();hat.update();tom.update();
}
