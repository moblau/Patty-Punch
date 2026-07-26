#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{
inline constexpr int version = 1;
inline constexpr auto kickTune = "kickTune";
inline constexpr auto kickDecay = "kickDecay";
inline constexpr auto kickSweep = "kickSweep";
inline constexpr auto kickSweepTime = "kickSweepTime";
inline constexpr auto kickClick = "kickClick";
inline constexpr auto kickClickTone = "kickClickTone";
inline constexpr auto kickDrive = "kickDrive";
inline constexpr auto kickLevel = "kickLevel";
inline constexpr auto snareTune = "snareTune";
inline constexpr auto snareDecay = "snareDecay";
inline constexpr auto snareSnappy = "snareSnappy";
inline constexpr auto snareTone = "snareTone";
inline constexpr auto snareLow = "snareLow";
inline constexpr auto snareCrack = "snareCrack";
inline constexpr auto snareAir = "snareAir";
inline constexpr auto snareLevel = "snareLevel";
inline constexpr auto hatTune = "hatTune";
inline constexpr auto hatDecay = "hatDecay";
inline constexpr auto hatTone = "hatTone";
inline constexpr auto hatHighPass = "hatHighPass";
inline constexpr auto hatChoke = "hatChoke";
inline constexpr auto hatLevel = "hatLevel";
inline constexpr auto lfoRate = "lfoRate";
inline constexpr auto lfoShape = "lfoShape";
inline constexpr auto lfoPitch = "lfoPitch";
inline constexpr auto lfoDecay = "lfoDecay";
inline constexpr auto masterLevel = "masterLevel";
inline constexpr auto kickNote = "kickNote";
inline constexpr auto snareNote = "snareNote";
inline constexpr auto hatNote = "hatNote";

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
void ensureMidiProperties (juce::ValueTree&);
juce::String frequencyText (float hz);
}
