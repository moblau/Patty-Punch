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
    juce::TextButton learn { "LEARN" }, minus { "-" }, plus { "+" };
    std::vector<std::unique_ptr<ParamKnob>> knobs;
    uint32_t lastActivity=0;
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
    juce::Label logo, subtitle, masterLabel, lfoTitle, lfoIndicator;
    ParamKnob master;
    InstrumentPanel kick, snare, hat;
    ParamKnob lfoRate, lfoPitch, lfoDecay;
    juce::ComboBox lfoShape;
    juce::ToggleButton choke { "CHOKE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoShapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chokeAttachment;
    float meter=0, lfo=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PattyPunchAudioProcessorEditor)
};
