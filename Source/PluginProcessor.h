#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DrumEngine.h"

class PattyPunchAudioProcessor final : public juce::AudioProcessor
{
public:
    struct MidiZoneHit
    {
        DrumEngine::Instrument instrument=DrumEngine::kick;
        int semitoneOffset=0;
        float pitchMultiplier=1;
    };

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
    double getTailLengthSeconds() const override { return 13.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    void enqueuePad (DrumEngine::Instrument, float velocity=1.0f);
    static bool routeMidiNote (int midiNote, MidiZoneHit&) noexcept;
    static const char* zoneLabel (DrumEngine::Instrument) noexcept;
    float getMeter() const { return outputMeter.load(); }
    float getLfoValue() const { return engine.getLfoValue(); }
    void getLfoSnapshot (size_t index, LfoDisplaySnapshot& snapshot) const noexcept
    {
        engine.getLfoSnapshot (index, snapshot);
    }
    uint32_t getActivityCounter (DrumEngine::Instrument i) const { return activity[(size_t)i].load(); }

private:
    struct UiHit { int instrument=0; float velocity=1; float pitchMultiplier=1; };
    juce::AbstractFifo uiFifo { 32 };
    std::array<UiHit, 32> uiHits {};
    DrumEngine engine;
    std::array<std::atomic<uint32_t>, drumCount> activity {};
    std::atomic<float> outputMeter { 0 };

    struct RawLfoParameters
    {
        std::atomic<float>* rate=nullptr;
        std::atomic<float>* shape=nullptr;
        std::atomic<float>* pitch=nullptr;
        std::atomic<float>* decay=nullptr;
        std::atomic<float>* modFrom=nullptr;
        std::atomic<float>* warp=nullptr;
        std::array<std::atomic<float>*, 2> targets {};
        std::array<std::atomic<float>*, 2> depths {};
    };
    struct RawRepeatParameters
    {
        std::atomic<float>* count = nullptr;
        std::atomic<float>* time = nullptr;
        std::atomic<float>* shape = nullptr;
    };
    struct RawParameters
    {
        std::atomic<float>* kickTune=nullptr; std::atomic<float>* kickDecay=nullptr;
        std::atomic<float>* kickSweep=nullptr; std::atomic<float>* kickSweepTime=nullptr;
        std::atomic<float>* kickClick=nullptr; std::atomic<float>* kickClickTone=nullptr;
        std::atomic<float>* kickDrive=nullptr; std::atomic<float>* kickLevel=nullptr;
        std::atomic<float>* snareTune=nullptr; std::atomic<float>* snareDecay=nullptr;
        std::atomic<float>* snareSnappy=nullptr; std::atomic<float>* snareTone=nullptr;
        std::atomic<float>* snareLow=nullptr; std::atomic<float>* snareCrack=nullptr;
        std::atomic<float>* snareAir=nullptr; std::atomic<float>* snareLevel=nullptr;
        std::atomic<float>* snareDrive=nullptr;
        std::atomic<float>* hatTune=nullptr; std::atomic<float>* hatDecay=nullptr;
        std::atomic<float>* hatTone=nullptr; std::atomic<float>* hatHighPass=nullptr;
        std::atomic<float>* hatChoke=nullptr; std::atomic<float>* hatLevel=nullptr;
        std::atomic<float>* tomTune=nullptr; std::atomic<float>* tomSweep=nullptr;
        std::atomic<float>* tomDecay=nullptr; std::atomic<float>* tomTone=nullptr;
        std::atomic<float>* tomAttack=nullptr; std::atomic<float>* tomLevel=nullptr;
        std::array<RawRepeatParameters, drumCount> repeats {};
        std::array<RawLfoParameters,lfoCount> lfos {};
        std::atomic<float>* masterLevel=nullptr;
    } raw;

    DrumParameters snapshot() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PattyPunchAudioProcessor)
};
