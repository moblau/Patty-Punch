#pragma once
#include "PluginProcessor.h"

class PattyLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PattyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
};

class ParamKnob final : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState&, const juce::String& id,
               const juce::String& label, const juce::String& tooltip);
    void resized() override;
    double getValue() const { return slider.getValue(); }
    void setAccentColour (juce::Colour colour) { slider.setColour (juce::Slider::rotarySliderFillColourId, colour); }
private:
    juce::Label name;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class PerformancePad final : public juce::TextButton
{
public:
    explicit PerformancePad (const juce::String& text);
    void flash();
    void paintButton (juce::Graphics&, bool, bool) override;
private:
    int flashFrames=0;
    friend class PattyPunchAudioProcessorEditor;
    friend class InstrumentPanel;
};

class InstrumentPanel final : public juce::Component
{
public:
    InstrumentPanel (PattyPunchAudioProcessor&, DrumEngine::Instrument,
                     const juce::String&, std::initializer_list<std::tuple<const char*,const char*,const char*>>);
    void resized() override;
    void update();
    PerformancePad pad;
private:
    PattyPunchAudioProcessor& processor;
    DrumEngine::Instrument instrument;
    juce::Label title, note;
    std::vector<std::unique_ptr<ParamKnob>> knobs;
    uint32_t lastActivity=0;
};

class LfoWaveform final : public juce::Component
{
public:
    explicit LfoWaveform (juce::Colour);
    void setSnapshot (const LfoDisplaySnapshot&, int shape, bool modulationActive);
    void paint (juce::Graphics&) override;
private:
    juce::Colour accent;
    LfoDisplaySnapshot snapshot;
    int baseShape=0;
    bool showBaseShape=false, hasSnapshot=false;
};

class LfoCard final : public juce::Component
{
public:
    LfoCard (PattyPunchAudioProcessor&, size_t index, juce::Colour accent,
             const char* rateId, const char* shapeId, const char* pitchId,
             const char* decayId, const char* modFromId, const char* warpId);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update();
private:
    PattyPunchAudioProcessor& processor;
    size_t index;
    juce::Colour accent;
    juce::Label title, shapeLabel, modFromLabel;
    LfoWaveform waveform;
    ParamKnob rate, pitch, decay, warp;
    juce::ComboBox shape, modFrom;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment, modFromAttachment;
};

class PattyPunchAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit PattyPunchAudioProcessorEditor (PattyPunchAudioProcessor&);
    ~PattyPunchAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
private:
    void timerCallback() override;
    PattyPunchAudioProcessor& processor;
    PattyLookAndFeel look;
    juce::Label logo, subtitle, masterLabel, warblerTitle, warblerSubtitle;
    ParamKnob master;
    InstrumentPanel kick, snare, hat;
    std::array<std::unique_ptr<LfoCard>,lfoCount> lfoCards;
    juce::ToggleButton choke { "CHOKE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chokeAttachment;
    juce::Rectangle<int> meterBounds;
    float meter=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PattyPunchAudioProcessorEditor)
};
