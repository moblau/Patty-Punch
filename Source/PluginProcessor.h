#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DrumEngine.h"

class PattyPunchAudioProcessor final : public juce::AudioProcessor,
                                        private juce::ValueTree::Listener,
                                        private juce::AsyncUpdater
{
public:
    PattyPunchAudioProcessor();
    ~PattyPunchAudioProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    void enqueuePad (DrumEngine::Instrument, float velocity=1.0f);
    void armMidiLearn (DrumEngine::Instrument);
    int getMidiNote (DrumEngine::Instrument) const;
    void setMidiNote (DrumEngine::Instrument, int);
    float getMeter() const { return outputMeter.load(); }
    float getLfoValue() const { return engine.getLfoValue(); }
    uint32_t getActivityCounter (DrumEngine::Instrument i) const { return activity[(size_t)i].load(); }

private:
    struct UiHit { int instrument=0; float velocity=1; };
    juce::AbstractFifo uiFifo { 32 };
    std::array<UiHit, 32> uiHits {};
    DrumEngine engine;
    std::array<std::atomic<int>,3> midiNotes { 60,61,62 };
    std::atomic<int> learnTarget { -1 }, learnedNote { -1 };
    std::array<std::atomic<uint32_t>,3> activity {};
    std::atomic<float> outputMeter { 0 };
    std::array<std::atomic<float>*,27> raw {};

    DrumParameters snapshot() const;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void handleAsyncUpdate() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PattyPunchAudioProcessor)
};
