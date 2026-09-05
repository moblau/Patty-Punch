#include "Parameters.h"

namespace
{
using PID = juce::ParameterID;
std::unique_ptr<juce::AudioParameterFloat> fp (const char* id, const juce::String& name,
    float lo, float hi, float step, float skew, float def, const juce::String& unit)
{
    juce::NormalisableRange<float> r (lo, hi, step);
    r.setSkewForCentre (skew);
    return std::make_unique<juce::AudioParameterFloat> (
        PID { id, Params::version }, name, r, def,
        juce::AudioParameterFloatAttributes().withLabel (unit));
}

std::unique_ptr<juce::AudioParameterInt> repeatCountParameter (const char* id,
                                                               const juce::String& name)
{
    return std::make_unique<juce::AudioParameterInt> (
        PID { id, Params::version }, name, 0, 10, 0,
        juce::AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int value, int) { return value == 0 ? juce::String { "Off" }
                                                    : juce::String { value }; }));
}

void addRepeatParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
                          const juce::String& drum, const char* countId,
                          const char* timeId, const char* shapeId)
{
    parameters.push_back (repeatCountParameter (countId, drum + " Repeat Count"));
    parameters.push_back (fp (timeId, drum + " Repeat Time", 10, 500, .1f, 90, 90, "ms"));
    parameters.push_back (fp (shapeId, drum + " Repeat Shape", 0, 100, .1f, 50, 0, "%"));
}

void addLfoParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
                       size_t index, const char* rateId, const char* shapeId,
                       const char* pitchId, const char* decayId,
                       const char* modFromId, const char* warpId)
{
    const auto number = juce::String (static_cast<int> (index + 1));
    parameters.push_back (fp (rateId, "LFO " + number + " Rate", .05f, 20, .001f, 2, 1, "Hz"));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { shapeId, Params::version }, "LFO " + number + " Shape",
        juce::StringArray { "Sine", "Triangle", "Square" }, 0));
    parameters.push_back (fp (pitchId, "LFO " + number + " Hat Pitch", 0, 24, .01f, 12, 0, "st"));
    parameters.push_back (fp (decayId, "LFO " + number + " Hat Decay", 0, 100, .1f, 50, 0, "%"));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { modFromId, Params::version }, "LFO " + number + " Mod From",
        Params::modSourceNames (index), 0));
    parameters.push_back (fp (warpId, "LFO " + number + " Warp", -100, 100, .1f, 0, 0, "%"));
}

void addMissingParameter (juce::ValueTree& state, const char* id, float defaultValue)
{
    if (state.getChildWithProperty ("id", id).isValid())
        return;

    juce::ValueTree parameter { "PARAM" };
    parameter.setProperty ("id", id, nullptr);
    parameter.setProperty ("value", defaultValue, nullptr);
    state.addChild (parameter, -1, nullptr);
}

void addLfoRoutes (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
                   size_t index)
{
    const auto number = juce::String (static_cast<int> (index + 1));
    const auto destinations = Params::modDestinationNames();
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { Params::lfoTargetA[index], Params::version }, "LFO " + number + " Target A",
        destinations, 0));
    parameters.push_back (fp (Params::lfoDepthA[index], "LFO " + number + " Depth A",
                              -100, 100, .1f, 0, 0, "%"));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { Params::lfoTargetB[index], Params::version }, "LFO " + number + " Target B",
        destinations, 0));
    parameters.push_back (fp (Params::lfoDepthB[index], "LFO " + number + " Depth B",
                              -100, 100, .1f, 0, 0, "%"));
}
}

juce::AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (fp (kickTune, "Kick Tune", 25, 100, 0.1f, 52, 52, "Hz"));
    p.push_back (fp (kickDecay, "Kick Decay", .1f, 5, .001f, .8f, 1.6f, "s"));
    p.push_back (fp (kickSweep, "Kick Pitch Sweep", 0, 48, .1f, 12, 30, "st"));
    p.push_back (fp (kickSweepTime, "Kick Sweep Time", 5, 150, .1f, 40, 40, "ms"));
    p.push_back (fp (kickClick, "Kick Click", 0, 100, .1f, 50, 35, "%"));
    p.push_back (fp (kickClickTone, "Kick Click Tone", 1000, 12000, 1, 4500, 4500, "Hz"));
    p.push_back (fp (kickDrive, "Kick Drive", 0, 12, .01f, 6, 2, "dB"));
    p.push_back (fp (kickLevel, "Kick Level", -60, 6, .01f, -12, -2, "dB"));
    p.push_back (fp (snareTune, "Snare Tune", 120, 320, .1f, 185, 185, "Hz"));
    p.push_back (fp (snareDecay, "Snare Decay", 80, 2000, 1, 420, 420, "ms"));
    p.push_back (fp (snareSnappy, "Snare Snappy", 0, 100, .1f, 50, 60, "%"));
    p.push_back (fp (snareTone, "Snare Tone", 500, 10000, 1, 3500, 3500, "Hz"));
    p.push_back (fp (snareLow, "Snare Low EQ", -12, 12, .1f, 0, 0, "dB"));
    p.push_back (fp (snareCrack, "Snare Crack EQ", -12, 12, .1f, 0, 0, "dB"));
    p.push_back (fp (snareAir, "Snare Air EQ", -12, 12, .1f, 0, 0, "dB"));
    p.push_back (fp (snareLevel, "Snare Level", -60, 6, .01f, -12, -2, "dB"));
    p.push_back (fp (hatTune, "Hat Tune", -24, 24, .1f, 0, 0, "st"));
    p.push_back (fp (hatDecay, "Hat Decay", 15, 2500, 1, 250, 180, "ms"));
    p.push_back (fp (hatTone, "Hat Tone", 0, 100, .1f, 50, 55, "%"));
    p.push_back (fp (hatHighPass, "Hat High Pass", 3000, 12000, 1, 6000, 6000, "Hz"));
    p.push_back (std::make_unique<juce::AudioParameterBool> (PID { hatChoke, version }, "Hat Choke", true));
    p.push_back (fp (hatLevel, "Hat Level", -60, 6, .01f, -12, -5, "dB"));
    // Keep the original parameter IDs and ordering intact for host automation.
    p.push_back (fp (lfoRate, "LFO 1 Rate", .05f, 20, .001f, 2, 1, "Hz"));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { lfoShape, version }, "LFO 1 Shape", juce::StringArray { "Sine", "Triangle", "Square" }, 0));
    p.push_back (fp (lfoPitch, "LFO 1 Hat Pitch", 0, 24, .01f, 12, 0, "st"));
    p.push_back (fp (lfoDecay, "LFO 1 Hat Decay", 0, 100, .1f, 50, 0, "%"));
    p.push_back (fp (masterLevel, "Master Level", -60, 6, .01f, -12, -3, "dB"));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { lfo1ModFrom, version }, "LFO 1 Mod From", modSourceNames (0), 0));
    p.push_back (fp (lfo1Warp, "LFO 1 Warp", -100, 100, .1f, 0, 0, "%"));
    addLfoParameters (p, 1, lfo2Rate, lfo2Shape, lfo2Pitch, lfo2Decay, lfo2ModFrom, lfo2Warp);
    addLfoParameters (p, 2, lfo3Rate, lfo3Shape, lfo3Pitch, lfo3Decay, lfo3ModFrom, lfo3Warp);
    addLfoParameters (p, 3, lfo4Rate, lfo4Shape, lfo4Pitch, lfo4Decay, lfo4ModFrom, lfo4Warp);

    // New parameters are appended so every pre-existing automation index remains stable.
    p.push_back (fp (snareDrive, "Snare Drive", 0, 18, .1f, 6, 3, "dB"));
    p.push_back (fp (tomTune, "Tom Pitch", 55, 440, .1f, 140, 140, "Hz"));
    p.push_back (fp (tomSweep, "Tom Pitch Bend", 0, 36, .1f, 10, 12, "st"));
    p.push_back (fp (tomDecay, "Tom Decay", 60, 2500, .1f, 420, 520, "ms"));
    p.push_back (fp (tomTone, "Tom Damping", 0, 100, .1f, 50, 58, "%"));
    p.push_back (fp (tomAttack, "Tom Attack", 0, 100, .1f, 50, 22, "%"));
    p.push_back (fp (tomLevel, "Tom Level", -60, 6, .1f, -12, -3, "dB"));
    addRepeatParameters (p, "Kick", kickRepeatCount, kickRepeatTime, kickRepeatShape);
    addRepeatParameters (p, "Snare", snareRepeatCount, snareRepeatTime, snareRepeatShape);
    addRepeatParameters (p, "Hi-Hat", hatRepeatCount, hatRepeatTime, hatRepeatShape);
    addRepeatParameters (p, "Tom", tomRepeatCount, tomRepeatTime, tomRepeatShape);
    for (size_t i = 0; i < 4; ++i)
        addLfoRoutes (p, i);
    return { p.begin(), p.end() };
}

void Params::ensureWarblerState (juce::ValueTree& state)
{
    // Older sessions contain the four original LFO 1 parameters only. Add every
    // new child explicitly so loading an old state cannot inherit current values.
    addMissingParameter (state, lfo1ModFrom, 0);
    addMissingParameter (state, lfo1Warp, 0);
    for (const auto* id : { lfo2Shape, lfo2Pitch, lfo2Decay, lfo2ModFrom, lfo2Warp,
                            lfo3Shape, lfo3Pitch, lfo3Decay, lfo3ModFrom, lfo3Warp,
                            lfo4Shape, lfo4Pitch, lfo4Decay, lfo4ModFrom, lfo4Warp })
        addMissingParameter (state, id, 0);
    for (const auto* id : { lfo2Rate, lfo3Rate, lfo4Rate })
        addMissingParameter (state, id, 1);
}

void Params::ensureCurrentState (juce::ValueTree& state)
{
    ensureWarblerState (state);
    addMissingParameter (state, snareDrive, 3);
    addMissingParameter (state, tomTune, 140);
    addMissingParameter (state, tomSweep, 12);
    addMissingParameter (state, tomDecay, 520);
    addMissingParameter (state, tomTone, 58);
    addMissingParameter (state, tomAttack, 22);
    addMissingParameter (state, tomLevel, -3);
    for (const auto* id : { kickRepeatCount, snareRepeatCount, hatRepeatCount, tomRepeatCount,
                            kickRepeatShape, snareRepeatShape, hatRepeatShape, tomRepeatShape })
        addMissingParameter (state, id, 0);
    for (const auto* id : { kickRepeatTime, snareRepeatTime, hatRepeatTime, tomRepeatTime })
        addMissingParameter (state, id, 90);
    for (size_t i = 0; i < 4; ++i)
    {
        addMissingParameter (state, lfoTargetA[i], 0);
        addMissingParameter (state, lfoDepthA[i], 0);
        addMissingParameter (state, lfoTargetB[i], 0);
        addMissingParameter (state, lfoDepthB[i], 0);
    }
}

juce::StringArray Params::modSourceNames (size_t targetLfo)
{
    juce::StringArray names { "Off" };
    for (size_t source = 0; source < 4; ++source)
        if (source != targetLfo)
            names.add ("LFO " + juce::String (static_cast<int> (source + 1)));
    return names;
}

juce::StringArray Params::modDestinationNames()
{
    return { "Off",
             "Kick Pitch", "Kick Sweep", "Kick Decay", "Kick Click", "Kick Drive", "Kick Level",
             "Snare Pitch", "Snare Decay", "Snare Snappy", "Snare Tone", "Snare Drive", "Snare Level",
             "Hi-Hat Pitch", "Hi-Hat Decay", "Hi-Hat Tone", "Hi-Hat Character", "Hi-Hat Level",
             "Tom Pitch", "Tom Bend", "Tom Decay", "Tom Damping", "Tom Attack", "Tom Level" };
}

int Params::modSourceForChoice (size_t targetLfo, int choice) noexcept
{
    if (choice <= 0 || choice > 3 || targetLfo >= 4)
        return -1;

    int currentChoice = 0;
    for (int source = 0; source < 4; ++source)
        if (source != static_cast<int> (targetLfo) && ++currentChoice == choice)
            return source;
    return -1;
}

void Params::ensureMidiProperties (juce::ValueTree& state)
{
    if (! state.hasProperty (kickNote)) state.setProperty (kickNote, 60, nullptr);
    if (! state.hasProperty (snareNote)) state.setProperty (snareNote, 61, nullptr);
    if (! state.hasProperty (hatNote)) state.setProperty (hatNote, 62, nullptr);
    if (! state.hasProperty (tomNote)) state.setProperty (tomNote, 63, nullptr);
}

juce::String Params::frequencyText (float hz)
{
    const auto midi = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));
    return juce::String (hz, 1) + " Hz / " + juce::MidiMessage::getMidiNoteName (midi, true, true, 3);
}
