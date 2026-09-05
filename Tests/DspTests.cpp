#include <juce_core/juce_core.h>
#include "DrumEngine.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}

struct Stats
{
    double energy = 0, early = 0, late = 0, highFrequency = 0, mean = 0;
    float peak = 0;
};

Stats renderHit (DrumEngine::Instrument instrument, DrumParameters parameters, double sampleRate,
                 int blockSize, float velocity = 1, int offset = 0, double seconds = 2,
                 float pitchMultiplier = 1)
{
    DrumEngine engine;
    engine.prepare (sampleRate, blockSize);
    juce::AudioBuffer<float> buffer (2, static_cast<int> (sampleRate * seconds));
    buffer.clear();
    engine.render (buffer, 0, offset, parameters);
    engine.trigger (instrument, velocity, pitchMultiplier, parameters);
    engine.render (buffer, offset, buffer.getNumSamples() - offset, parameters);
    Stats stats;
    float previous = 0;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = buffer.getSample (0, sample);
        require (std::isfinite (value), "voice emitted a non-finite sample");
        stats.energy += value * value;
        if (sample < static_cast<int> (.02 * sampleRate)) stats.early += value * value;
        if (sample > static_cast<int> (.7 * sampleRate)) stats.late += value * value;
        stats.highFrequency += (value - previous) * (value - previous);
        stats.mean += value;
        stats.peak = juce::jmax (stats.peak, std::abs (value));
        previous = value;
    }
    stats.mean /= buffer.getNumSamples();
    return stats;
}

void renderInBlocks (DrumEngine& engine, const DrumParameters& parameters, int samples,
                     const std::array<int, 7>& sizes = { 1, 7, 31, 64, 113, 257, 509 })
{
    juce::AudioBuffer<float> buffer (1, 509);
    int rendered = 0, cursor = 0;
    while (rendered < samples)
    {
        const auto count = juce::jmin (sizes[static_cast<size_t> (cursor++ % static_cast<int> (sizes.size()))],
                                       samples - rendered);
        buffer.clear();
        engine.render (buffer, 0, count, parameters);
        for (int sample = 0; sample < count; ++sample)
            require (std::isfinite (buffer.getSample (0, sample)), "block render emitted non-finite audio");
        rendered += count;
    }
}

struct ProcessorResult
{
    double energy = 0;
    std::array<uint32_t, drumCount> activity {};
    juce::AudioBuffer<float> audio { 2, 4096 };
};

ProcessorResult processNote (int note, float velocity = 1.0f, int sampleOffset = 0)
{
    PattyPunchAudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 48000.0, 4096);
    processor.prepareToPlay (48000.0, 4096);
    ProcessorResult result;
    result.audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, note, velocity), sampleOffset);
    processor.processBlock (result.audio, midi);
    for (int sample = 0; sample < result.audio.getNumSamples(); ++sample)
    {
        const auto value = result.audio.getSample (0, sample);
        require (std::isfinite (value), "processor emitted non-finite audio");
        result.energy += value * value;
    }
    for (size_t i = 0; i < drumCount; ++i)
        result.activity[i] = processor.getActivityCounter (static_cast<DrumEngine::Instrument> (i));
    return result;
}

void verifyRateAndBlock (double sampleRate, int blockSize)
{
    DrumParameters parameters;
    for (auto instrument : { DrumEngine::kick, DrumEngine::snare, DrumEngine::hat, DrumEngine::tom })
    {
        const auto stats = renderHit (instrument, parameters, sampleRate, blockSize, 1, 0, .75);
        require (stats.energy > 1.0e-5, "voice was silent at a supported rate/block size");
        require (stats.peak < 1.21f, "voice output escaped the bounded master stage");
    }
}

void verifyLfoRoute (size_t lfoIndex, Params::ModDestination destination)
{
    DrumParameters parameters;
    parameters.lfos[lfoIndex].rate = 1;
    parameters.lfos[lfoIndex].shape = 2;
    parameters.lfos[lfoIndex].routes[0] = { destination, 1 };
    DrumEngine engine;
    engine.prepare (100, 1);
    juce::AudioBuffer<float> sample (1, 1);
    sample.clear();
    engine.render (sample, 0, 1, parameters);
    require (std::abs (engine.getModulationForTests (destination) - 1.0f) < 1.0e-6f,
             "LFO route did not reach its destination");
}

double renderModulatedTail (DrumEngine::Instrument instrument, Params::ModDestination destination,
                            float depth)
{
    constexpr double sampleRate = 48000;
    DrumParameters parameters;
    parameters.lfos[0].rate = 1;
    parameters.lfos[0].shape = 2;
    parameters.lfos[0].routes[0] = { destination, depth };
    DrumEngine engine;
    engine.prepare (sampleRate, 127);
    juce::AudioBuffer<float> buffer (1, static_cast<int> (sampleRate * 2));
    buffer.clear();
    engine.render (buffer, 0, 1, parameters);
    engine.trigger (instrument, 1, 1, parameters);
    engine.render (buffer, 1, buffer.getNumSamples() - 1, parameters);
    double tail = 0;
    for (int sample = static_cast<int> (sampleRate * .7); sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = buffer.getSample (0, sample);
        require (std::isfinite (value), "decay modulation emitted non-finite audio");
        tail += value * value;
    }
    return tail;
}

struct DummyProcessor final : juce::AudioProcessor
{
    DummyProcessor() : AudioProcessor (BusesProperties()) {}
    const juce::String getName() const override { return "test"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI juceInitialiser;

        for (const auto sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
            for (const auto blockSize : { 1, 64, 257, 1024 })
                verifyRateAndBlock (sampleRate, blockSize);

        DrumParameters parameters;
        const auto quietKick = renderHit (DrumEngine::kick, parameters, 48000, 128, .2f);
        const auto loudKick = renderHit (DrumEngine::kick, parameters, 48000, 128, 1);
        require (loudKick.energy > quietKick.energy * 2, "kick velocity response is ineffective");
        parameters.kickDecay = .12f;
        const auto shortKick = renderHit (DrumEngine::kick, parameters, 48000, 128);
        parameters.kickDecay = 3;
        const auto longKick = renderHit (DrumEngine::kick, parameters, 48000, 128);
        require (longKick.late > shortKick.late * 10, "kick decay range is not musically effective");
        parameters = {};
        parameters.kickClick = 0;
        const auto roundKick = renderHit (DrumEngine::kick, parameters, 48000, 128);
        parameters.kickClick = 1;
        const auto clickKick = renderHit (DrumEngine::kick, parameters, 48000, 128);
        require (clickKick.highFrequency > roundKick.highFrequency, "kick Click lacks audible range");

        parameters = {};
        parameters.snareSnappy = .03f;
        const auto bodySnare = renderHit (DrumEngine::snare, parameters, 48000, 128);
        parameters.snareSnappy = 1;
        const auto wireSnare = renderHit (DrumEngine::snare, parameters, 48000, 128);
        require (wireSnare.highFrequency > bodySnare.highFrequency, "snare Snappy lacks audible range");
        parameters = {};
        parameters.tomAttack = 0;
        const auto softTom = renderHit (DrumEngine::tom, parameters, 48000, 128);
        parameters.tomAttack = 1;
        const auto stickTom = renderHit (DrumEngine::tom, parameters, 48000, 128);
        require (stickTom.highFrequency > softTom.highFrequency, "tom Attack lacks audible range");
        parameters = {};
        const auto hat = renderHit (DrumEngine::hat, parameters, 48000, 128);
        require (hat.highFrequency / hat.energy > .05, "hi-hat lacks metallic high-frequency content");

        for (int shape = 0; shape < 3; ++shape)
            for (int step = -32; step <= 160; ++step)
            {
                const auto value = evaluateLfoWaveform (shape, step / 128.0f);
                require (std::isfinite (value) && value >= -1 && value <= 1,
                         "LFO waveform escaped its bounds");
            }
        require (std::abs (evaluateLfoWaveform (0, .25f) - 1) < 1.0e-6f, "sine waveform mismatch");
        require (std::abs (evaluateLfoWaveform (1, .5f) - 1) < 1.0e-6f, "triangle waveform mismatch");
        require (evaluateLfoWaveform (2, .49f) > .99f && evaluateLfoWaveform (2, .5f) < -.99f,
                 "square waveform mismatch");

        for (size_t lfo = 0; lfo < lfoCount; ++lfo)
            for (const auto destination : { Params::ModDestination::kickPitch,
                                            Params::ModDestination::kickDecay,
                                            Params::ModDestination::snarePitch,
                                            Params::ModDestination::snareDecay })
                verifyLfoRoute (lfo, destination);
        for (size_t destination = 1; destination < Params::modDestinationCount; ++destination)
            verifyLfoRoute (0, static_cast<Params::ModDestination> (destination));
        require (renderModulatedTail (DrumEngine::kick, Params::ModDestination::kickDecay, 1)
                    > renderModulatedTail (DrumEngine::kick, Params::ModDestination::kickDecay, -1) * 20,
                 "kick decay modulation did not change the synthesized tail");
        require (renderModulatedTail (DrumEngine::snare, Params::ModDestination::snareDecay, 1)
                    > renderModulatedTail (DrumEngine::snare, Params::ModDestination::snareDecay, -1) * 20,
                 "snare decay modulation did not change the synthesized tail");

        DrumParameters legacy;
        legacy.lfos[0].rate = 1;
        legacy.lfos[0].shape = 2;
        legacy.lfos[0].legacyHatPitchDepth = 12;
        legacy.lfos[0].legacyHatDecayDepth = .4f;
        DrumEngine legacyEngine;
        legacyEngine.prepare (100, 1);
        juce::AudioBuffer<float> singleSample (1, 1);
        singleSample.clear();
        legacyEngine.render (singleSample, 0, 1, legacy);
        require (std::abs (legacyEngine.getModulationForTests (Params::ModDestination::hatPitch) - .5f) < 1.0e-6f,
                 "legacy hat pitch depth changed");
        require (std::abs (legacyEngine.getModulationForTests (Params::ModDestination::hatDecay) - .4f) < 1.0e-6f,
                 "legacy hat decay depth changed");

        DrumParameters modulated;
        modulated.snareTune = 240;
        modulated.lfos[0].rate = 1;
        modulated.lfos[0].shape = 2;
        modulated.lfos[0].routes[0] = { Params::ModDestination::kickPitch, 1 };
        modulated.lfos[0].routes[1] = { Params::ModDestination::snarePitch, -1 };
        DrumEngine modulatedEngine;
        modulatedEngine.prepare (48000, 1);
        singleSample.clear();
        modulatedEngine.render (singleSample, 0, 1, modulated);
        modulatedEngine.trigger (DrumEngine::kick, 1, 1, modulated);
        modulatedEngine.trigger (DrumEngine::snare, 1, 1, modulated);
        require (std::abs (modulatedEngine.getKickFinalFrequenciesForTests()[0] / modulated.kickTune - 4) < .01,
                 "kick pitch did not use semitone-domain modulation");
        require (std::abs (modulatedEngine.getSnareFinalFrequenciesForTests()[0] / modulated.snareTune - .25f) < .01,
                 "snare pitch did not support negative semitone modulation");

        for (int repeats = 1; repeats <= 10; ++repeats)
        {
            DrumParameters repeatParameters;
            repeatParameters.repeats[DrumEngine::kick] = { repeats, .01f, .63f };
            DrumEngine repeatEngine;
            repeatEngine.prepare (48000, 257);
            repeatEngine.trigger (DrumEngine::kick, 1, 1, repeatParameters);
            renderInBlocks (repeatEngine, repeatParameters,
                            static_cast<int> (repeatEngine.getRepeatOffsetForTests (DrumEngine::kick, repeats - 1) + 2));
            require (repeatEngine.getRepeatTriggerCountForTests (DrumEngine::kick) == static_cast<uint32_t> (repeats),
                     "Repeat Count did not produce the exact number of repeats");
        }

        DrumParameters exactRepeat;
        exactRepeat.repeats[DrumEngine::snare] = { 3, .01f, 0 };
        DrumEngine exactEngine;
        exactEngine.prepare (48000, 17);
        exactEngine.trigger (DrumEngine::snare, 1, 1, exactRepeat);
        require (exactEngine.getRepeatOffsetForTests (DrumEngine::snare, 0) == 480
                 && exactEngine.getRepeatOffsetForTests (DrumEngine::snare, 1) == 960
                 && exactEngine.getRepeatOffsetForTests (DrumEngine::snare, 2) == 1440,
                 "neutral repeat timing is not evenly spaced");
        renderInBlocks (exactEngine, exactRepeat, 480);
        require (exactEngine.getRepeatTriggerCountForTests (DrumEngine::snare) == 0,
                 "repeat triggered one sample early across a block boundary");
        renderInBlocks (exactEngine, exactRepeat, 1);
        require (exactEngine.getRepeatTriggerCountForTests (DrumEngine::snare) == 1,
                 "repeat did not trigger at its exact sample");

        float previousPosition = 0, previousGain = 1;
        for (int repeat = 1; repeat <= 10; ++repeat)
        {
            const auto neutral = repeatCurvePosition (repeat, 10, 0);
            const auto shaped = repeatCurvePosition (repeat, 10, 1);
            const auto gain = repeatCurveGain (repeat, 10, 1);
            require (std::abs (neutral - repeat / 10.0f) < 1.0e-6f,
                     "neutral repeat curve is not linear");
            require (shaped > previousPosition && shaped <= 1, "repeat Shape is not monotonic and bounded");
            require (gain > 0 && gain < previousGain, "repeat amplitude progression is not bounded and monotonic");
            if (repeat < 10) require (shaped < neutral, "positive Shape did not pull early repeats forward");
            previousPosition = shaped;
            previousGain = gain;
        }

        DrumParameters bounded;
        bounded.repeats[DrumEngine::tom] = { 10, .01f, 1 };
        DrumEngine boundedEngine;
        boundedEngine.prepare (48000, 64);
        for (int hit = 0; hit < 1000; ++hit) boundedEngine.trigger (DrumEngine::tom, 1, 1, bounded);
        renderInBlocks (boundedEngine, bounded,
                        static_cast<int> (boundedEngine.getRepeatOffsetForTests (DrumEngine::tom, 9) + 2));
        require (boundedEngine.getRepeatTriggerCountForTests (DrumEngine::tom) == 10,
                 "rapid incoming triggers grew or overlapped the repeat queue");

        for (const auto sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            DrumParameters extreme;
            extreme.master = 1;
            extreme.tomTune = 440;
            extreme.tomSweep = 36;
            extreme.tomDecay = 2.5f;
            extreme.tomTone = extreme.tomAttack = 1;
            extreme.tomLevelDb = 6;
            for (size_t lfo = 0; lfo < lfoCount; ++lfo)
            {
                extreme.lfos[lfo].rate = 20;
                extreme.lfos[lfo].shape = static_cast<int> (lfo % 3);
                extreme.lfos[lfo].routes[0] = { Params::ModDestination::tomPitch, 1 };
                extreme.lfos[lfo].routes[1] = { Params::ModDestination::tomDecay, lfo % 2 == 0 ? 1.0f : -1.0f };
            }
            extreme.repeats[DrumEngine::tom] = { 10, .01f, 1 };
            const auto stats = renderHit (DrumEngine::tom, extreme, sampleRate, 1, 1, 0, 1.5,
                                          std::exp2 (11.0f / 12.0f));
            require (stats.peak < 1.21f, "extreme tom/repeat/modulation output was not bounded");
            require (std::abs (stats.mean) < .005, "tom produced persistent DC offset");
        }

        struct Boundary { int note; bool valid; DrumEngine::Instrument instrument; int semitones; };
        const Boundary boundaries[] {
            { 23, false, DrumEngine::kick, 0 }, { 24, true, DrumEngine::kick, 0 },
            { 47, true, DrumEngine::kick, 23 }, { 48, true, DrumEngine::snare, 0 },
            { 59, true, DrumEngine::snare, 11 }, { 60, true, DrumEngine::hat, 0 },
            { 71, true, DrumEngine::hat, 11 }, { 72, true, DrumEngine::tom, 0 },
            { 83, true, DrumEngine::tom, 11 }, { 84, false, DrumEngine::tom, 0 }
        };
        for (const auto& expected : boundaries)
        {
            PattyPunchAudioProcessor::MidiZoneHit routed;
            const auto valid = PattyPunchAudioProcessor::routeMidiNote (expected.note, routed);
            require (valid == expected.valid, "MIDI zone validity mismatch");
            const auto processed = processNote (expected.note);
            if (! valid) { require (processed.energy < 1.0e-20, "out-of-zone note produced audio"); continue; }
            require (routed.instrument == expected.instrument && routed.semitoneOffset == expected.semitones,
                     "MIDI zone routed to the wrong voice or pitch");
            int activitySum = 0;
            for (const auto count : processed.activity) activitySum += static_cast<int> (count);
            require (activitySum == 1 && processed.activity[static_cast<size_t> (expected.instrument)] == 1,
                     "MIDI note triggered multiple or incorrect instruments");
            require (processed.energy > 0, "in-zone note was silent");
        }
        const auto delayed = processNote (72, 1, 777);
        for (int sample = 0; sample < 777; ++sample)
            require (std::abs (delayed.audio.getSample (0, sample)) < 1.0e-20f,
                     "MIDI note triggered before its sample offset");
        require (delayed.audio.getMagnitude (0, 777, delayed.audio.getNumSamples() - 777) > 0,
                 "sample-offset MIDI note did not trigger");
        require (processNote (24, 0).energy < 1.0e-20, "zero-velocity Note On triggered a hit");

        DummyProcessor dummy;
        juce::AudioProcessorValueTreeState state (dummy, nullptr, "state", Params::createLayout());
        Params::ensureMidiProperties (state.state);
        require (Params::modDestinationNames().size() == static_cast<int> (Params::modDestinationCount),
                 "modulation destination labels and enum diverged");
        for (const auto* id : { Params::snareDrive, Params::tomTune, Params::tomSweep, Params::tomDecay,
                                Params::tomTone, Params::tomAttack, Params::tomLevel,
                                Params::kickRepeatCount, Params::snareRepeatCount,
                                Params::hatRepeatCount, Params::tomRepeatCount })
            require (state.getParameter (id) != nullptr, "new drum/repeat parameter is missing");
        for (size_t lfo = 0; lfo < lfoCount; ++lfo)
            for (const auto* id : { Params::lfoTargetA[lfo], Params::lfoDepthA[lfo],
                                    Params::lfoTargetB[lfo], Params::lfoDepthB[lfo] })
                require (state.getParameter (id) != nullptr, "new LFO route parameter is missing");

        const auto& ordered = dummy.getParameters();
        const char* legacyIds[] { Params::lfoRate, Params::lfoShape, Params::lfoPitch,
                                  Params::lfoDecay, Params::masterLevel };
        for (int index = 0; index < 5; ++index)
        {
            auto* parameter = dynamic_cast<juce::AudioProcessorParameterWithID*> (
                ordered[22 + index]);
            require (parameter != nullptr && parameter->paramID == legacyIds[index],
                     "legacy automation parameter ordering changed");
        }
        for (const auto* id : { Params::snareLow, Params::snareCrack, Params::snareAir,
                                Params::kickSweepTime, Params::kickClickTone })
            require (state.getParameter (id) != nullptr, "deprecated compatibility parameter was removed");

        PattyPunchAudioProcessor sourceProcessor;
        auto* tomTune = sourceProcessor.apvts.getParameter (Params::tomTune);
        auto* repeatCount = sourceProcessor.apvts.getParameter (Params::tomRepeatCount);
        auto* target = sourceProcessor.apvts.getParameter (Params::lfoTargetA[0]);
        tomTune->setValueNotifyingHost (tomTune->convertTo0to1 (233));
        repeatCount->setValueNotifyingHost (repeatCount->convertTo0to1 (7));
        target->setValueNotifyingHost (target->convertTo0to1 (
            static_cast<float> (Params::ModDestination::tomPitch)));
        juce::MemoryBlock savedState;
        sourceProcessor.getStateInformation (savedState);
        PattyPunchAudioProcessor restoredProcessor;
        restoredProcessor.setStateInformation (savedState.getData(), static_cast<int> (savedState.getSize()));
        require (std::abs (restoredProcessor.apvts.getRawParameterValue (Params::tomTune)->load() - 233) < .11f,
                 "tom parameter state recall failed");
        require (std::abs (restoredProcessor.apvts.getRawParameterValue (Params::tomRepeatCount)->load() - 7) < .01f,
                 "repeat state recall failed");

        auto oldState = sourceProcessor.apvts.copyState();
        for (const auto* id : { Params::snareDrive, Params::tomTune, Params::tomSweep, Params::tomDecay,
                                Params::tomTone, Params::tomAttack, Params::tomLevel,
                                Params::kickRepeatCount, Params::kickRepeatTime, Params::kickRepeatShape,
                                Params::snareRepeatCount, Params::snareRepeatTime, Params::snareRepeatShape,
                                Params::hatRepeatCount, Params::hatRepeatTime, Params::hatRepeatShape,
                                Params::tomRepeatCount, Params::tomRepeatTime, Params::tomRepeatShape })
        {
            const auto child = oldState.getChildWithProperty ("id", id);
            if (child.isValid()) oldState.removeChild (child, nullptr);
        }
        for (size_t lfo = 0; lfo < lfoCount; ++lfo)
            for (const auto* id : { Params::lfoTargetA[lfo], Params::lfoDepthA[lfo],
                                    Params::lfoTargetB[lfo], Params::lfoDepthB[lfo] })
            {
                const auto child = oldState.getChildWithProperty ("id", id);
                if (child.isValid()) oldState.removeChild (child, nullptr);
            }
        juce::MemoryBlock oldBinary;
        if (const auto xml = oldState.createXml()) juce::AudioProcessor::copyXmlToBinary (*xml, oldBinary);
        restoredProcessor.setStateInformation (oldBinary.getData(), static_cast<int> (oldBinary.getSize()));
        require (std::abs (restoredProcessor.apvts.getRawParameterValue (Params::tomTune)->load() - 140) < .11f,
                 "old state did not migrate the tom to its default");
        require (std::abs (restoredProcessor.apvts.getRawParameterValue (Params::tomRepeatCount)->load()) < .01f,
                 "old state did not migrate repeats to Off");
        require (std::abs (restoredProcessor.apvts.getRawParameterValue (Params::lfoDepthA[0])->load()) < .01f,
                 "old state did not migrate LFO routing to neutral");

        juce::AudioBuffer<float> silence (2, 4096);
        silence.clear();
        juce::MidiBuffer noMidi;
        restoredProcessor.prepareToPlay (192000, 1);
        restoredProcessor.processBlock (silence, noMidi);
        require (silence.getMagnitude (0, 0, silence.getNumSamples()) < 1.0e-20f,
                 "silence produced DC, denormal residue, or non-zero output");

        std::unique_ptr<juce::AudioProcessorEditor> editor (restoredProcessor.createEditor());
        for (const auto size : { juce::Point<int> { 1180, 760 }, juce::Point<int> { 1500, 900 },
                                 juce::Point<int> { 1900, 1200 } })
        {
            editor->setSize (size.x, size.y);
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true);
            require (image.isValid() && image.getWidth() == size.x && image.getHeight() == size.y,
                     "editor failed to lay out and render at a supported size");
        }

        std::cout << "Patty Punch DSP tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
