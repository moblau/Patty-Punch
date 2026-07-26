#include "PluginEditor.h"

static const auto cream=juce::Colour(0xffeee2c2), orange=juce::Colour(0xffe77732);
PattyLookAndFeel::PattyLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId,cream);setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff25262a));setColour(juce::ComboBox::textColourId,cream);
    setColour(juce::Label::textColourId,cream);setColour(juce::TextButton::textColourOffId,cream);
}
void PattyLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float pos,float a0,float a1,juce::Slider&)
{
    auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(7);auto c=r.getCentre();auto rad=juce::jmin(r.getWidth(),r.getHeight())*.5f;
    g.setColour(juce::Colour(0xff101114));g.fillEllipse(r);g.setColour(juce::Colour(0xff55575d));g.drawEllipse(r,2);
    juce::Path p;p.addRoundedRectangle(-2,-rad+5,4,rad*.42f,2);
    g.setColour(orange);g.fillPath(p,juce::AffineTransform::rotation(a0+pos*(a1-a0)).translated(c.x,c.y));
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
 std::initializer_list<std::tuple<const char*,const char*,const char*>> defs):pad("PUNCH"),processor(p),instrument(i)
{
    title.setText(t,juce::dontSendNotification);title.setFont(juce::FontOptions(22,juce::Font::bold));title.setJustificationType(juce::Justification::centred);
    note.setJustificationType(juce::Justification::centred);note.setFont(juce::FontOptions(12));
    for(auto&d:defs){auto k=std::make_unique<ParamKnob>(p.apvts,std::get<0>(d),std::get<1>(d),std::get<2>(d));addAndMakeVisible(*k);knobs.push_back(std::move(k));}
    addAndMakeVisible(title);addAndMakeVisible(note);addAndMakeVisible(learn);addAndMakeVisible(minus);addAndMakeVisible(plus);addAndMakeVisible(pad);
    pad.onClick=[this]{processor.enqueuePad(instrument);};learn.onClick=[this]{processor.armMidiLearn(instrument);learn.setButtonText("PLAY NOTE");};
    minus.onClick=[this]{processor.setMidiNote(instrument,processor.getMidiNote(instrument)-1);};plus.onClick=[this]{processor.setMidiNote(instrument,processor.getMidiNote(instrument)+1);};
    update();
}
void InstrumentPanel::resized()
{
    auto r=getLocalBounds().reduced(8);title.setBounds(r.removeFromTop(28));auto midi=r.removeFromTop(24);
    minus.setBounds(midi.removeFromLeft(26));plus.setBounds(midi.removeFromRight(26));learn.setBounds(midi.removeFromRight(70));note.setBounds(midi);
    pad.setBounds(r.removeFromBottom(62).reduced(6));const auto cols=juce::jmin(4,(int)knobs.size());const auto rows=((int)knobs.size()+cols-1)/cols;
    for(size_t i=0;i<knobs.size();++i){auto w=r.getWidth()/cols;auto h=r.getHeight()/juce::jmax(1,rows);knobs[i]->setBounds(r.getX()+(int)i%cols*w,r.getY()+(int)i/cols*h,w,h);}
}
void InstrumentPanel::update()
{
    const auto n=processor.getMidiNote(instrument);note.setText(juce::MidiMessage::getMidiNoteName(n,true,true,3)+" / "+juce::String(n),juce::dontSendNotification);
    auto a=processor.getActivityCounter(instrument);if(a!=lastActivity){lastActivity=a;pad.flash();learn.setButtonText("LEARN");}
    if(pad.flashFrames>0){--pad.flashFrames;pad.repaint();}
}

PattyPunchAudioProcessorEditor::PattyPunchAudioProcessorEditor(PattyPunchAudioProcessor&p)
:AudioProcessorEditor(&p),processor(p),master(p.apvts,Params::masterLevel,"MASTER","Overall output level"),
 kick(p,DrumEngine::kick,"KICK",{{Params::kickTune,"TUNE","Final body pitch"},{Params::kickDecay,"DECAY","Body decay time"},{Params::kickSweep,"SWEEP","Pitch sweep depth"},{Params::kickSweepTime,"SWEEP TIME","Pitch sweep duration"},{Params::kickClick,"CLICK","Transient amount"},{Params::kickClickTone,"CLICK TONE","Transient low-pass cutoff"},{Params::kickDrive,"DRIVE","Soft saturation"},{Params::kickLevel,"LEVEL","Kick output level"}}),
 snare(p,DrumEngine::snare,"SNARE",{{Params::snareTune,"TUNE","Fundamental pitch"},{Params::snareDecay,"DECAY","Snare duration"},{Params::snareSnappy,"SNAPPY","Noise balance"},{Params::snareTone,"TONE","Noise cutoff"},{Params::snareLow,"LOW EQ","140 Hz shelf"},{Params::snareCrack,"CRACK EQ","2.2 kHz peak"},{Params::snareAir,"AIR EQ","8 kHz shelf"},{Params::snareLevel,"LEVEL","Snare output level"}}),
 hat(p,DrumEngine::hat,"HI-HAT",{{Params::hatTune,"TUNE","Metal oscillator pitch"},{Params::hatDecay,"DECAY","Hat envelope time"},{Params::hatTone,"TONE","Filtered-band blend"},{Params::hatHighPass,"HIGH PASS","Hat high-pass cutoff"},{Params::hatLevel,"LEVEL","Hat output level"}}),
 lfoRate(p.apvts,Params::lfoRate,"RATE","LFO frequency"),lfoPitch(p.apvts,Params::lfoPitch,"PITCH DEPTH","Continuous pitch modulation"),lfoDecay(p.apvts,Params::lfoDecay,"DECAY DEPTH","Per-hit decay variation")
{
    setLookAndFeel(&look);logo.setText("PATTY PUNCH",juce::dontSendNotification);logo.setFont(juce::FontOptions(30,juce::Font::bold));subtitle.setText("THREE-VOICE ANALOG DRUM SYNTHESIZER",juce::dontSendNotification);
    lfoTitle.setText("HI-HAT LFO",juce::dontSendNotification);lfoTitle.setFont(juce::FontOptions(16,juce::Font::bold));lfoIndicator.setText("●",juce::dontSendNotification);lfoIndicator.setJustificationType(juce::Justification::centred);
    lfoShape.addItemList({"SINE","TRIANGLE","SQUARE"},1);lfoShape.setTooltip("LFO waveform");choke.setTooltip("Fade old hats when a new hat triggers");
    lfoShapeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,Params::lfoShape,lfoShape);
    chokeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,Params::hatChoke,choke);
    std::array<juce::Component*,13> components { &logo,&subtitle,&master,&kick,&snare,&hat,
        &lfoTitle,&lfoIndicator,&lfoRate,&lfoPitch,&lfoDecay,&lfoShape,&choke };
    for(auto* c:components)addAndMakeVisible(c);
    setWantsKeyboardFocus(true);setResizable(true,true);setResizeLimits(860,600,1500,950);setSize(1120,720);startTimerHz(36);
}
PattyPunchAudioProcessorEditor::~PattyPunchAudioProcessorEditor(){setLookAndFeel(nullptr);}
void PattyPunchAudioProcessorEditor::paint(juce::Graphics&g)
{
    g.fillAll(juce::Colour(0xff17181b));g.setColour(juce::Colour(0xff2a2b30));
    auto body=getLocalBounds().toFloat().reduced(10);g.fillRoundedRectangle(body,12);g.setColour(orange);g.drawRoundedRectangle(body,12,2);
    auto header=juce::Rectangle<float>(body.getX()+310,body.getY()+24,body.getWidth()-480,12);g.setColour(juce::Colour(0xff090a0b));g.fillRoundedRectangle(header,4);
    g.setColour(juce::Colour(0xfff04b35));g.fillRoundedRectangle(header.withWidth(header.getWidth()*juce::jlimit(0.0f,1.0f,meter)),4);
}
void PattyPunchAudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(22);auto header=r.removeFromTop(78);logo.setBounds(header.removeFromLeft(280).removeFromTop(40));subtitle.setBounds(25,55,290,20);master.setBounds(header.removeFromRight(100));
    auto lfoArea=r.removeFromBottom(125);auto panels=r;auto w=panels.getWidth()/3;kick.setBounds(panels.removeFromLeft(w).reduced(4));snare.setBounds(panels.removeFromLeft(w).reduced(4));hat.setBounds(panels.reduced(4));
    lfoTitle.setBounds(lfoArea.removeFromLeft(120));lfoIndicator.setBounds(lfoArea.removeFromLeft(45));lfoRate.setBounds(lfoArea.removeFromLeft(105));lfoPitch.setBounds(lfoArea.removeFromLeft(105));lfoDecay.setBounds(lfoArea.removeFromLeft(105));
    lfoShape.setBounds(lfoArea.removeFromLeft(120).reduced(8,42));choke.setBounds(lfoArea.removeFromLeft(100).reduced(8,42));
}
bool PattyPunchAudioProcessorEditor::keyPressed(const juce::KeyPress&k)
{
    const auto c=juce::CharacterFunctions::toLowerCase(k.getTextCharacter());
    if(c=='a')processor.enqueuePad(DrumEngine::kick);else if(c=='s')processor.enqueuePad(DrumEngine::snare);else if(c=='d')processor.enqueuePad(DrumEngine::hat);else return false;return true;
}
void PattyPunchAudioProcessorEditor::timerCallback()
{
    const auto m=processor.getMeter(),l=processor.getLfoValue();if(std::abs(m-meter)>.003f){meter=m;repaint(320,22,getWidth()-490,20);}
    if(std::abs(l-lfo)>.02f){lfo=l;lfoIndicator.setColour(juce::Label::textColourId,orange.withAlpha(.35f+.65f*(lfo*.5f+.5f)));}
    kick.update();snare.update();hat.update();
}
