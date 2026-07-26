#include "Parameters.h"

namespace
{
using PID = juce::ParameterID;
std::unique_ptr<juce::AudioParameterFloat> fp (const char* id, const char* name,
    float lo, float hi, float step, float skew, float def, const juce::String& unit)
{
    juce::NormalisableRange<float> r (lo, hi, step);
    r.setSkewForCentre (skew);
    return std::make_unique<juce::AudioParameterFloat> (
        PID { id, Params::version }, name, r, def,
        juce::AudioParameterFloatAttributes().withLabel (unit));
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
    p.push_back (fp (lfoRate, "Hat LFO Rate", .05f, 20, .001f, 2, 1, "Hz"));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        PID { lfoShape, version }, "Hat LFO Shape", juce::StringArray { "Sine", "Triangle", "Square" }, 0));
    p.push_back (fp (lfoPitch, "Hat LFO Pitch Depth", 0, 24, .01f, 12, 0, "st"));
    p.push_back (fp (lfoDecay, "Hat LFO Decay Depth", 0, 100, .1f, 50, 0, "%"));
    p.push_back (fp (masterLevel, "Master Level", -60, 6, .01f, -12, -3, "dB"));
    return { p.begin(), p.end() };
}

void Params::ensureMidiProperties (juce::ValueTree& state)
{
    if (! state.hasProperty (kickNote)) state.setProperty (kickNote, 60, nullptr);
    if (! state.hasProperty (snareNote)) state.setProperty (snareNote, 61, nullptr);
    if (! state.hasProperty (hatNote)) state.setProperty (hatNote, 62, nullptr);
}

juce::String Params::frequencyText (float hz)
{
    const auto midi = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));
    return juce::String (hz, 1) + " Hz / " + juce::MidiMessage::getMidiNoteName (midi, true, true, 3);
}
