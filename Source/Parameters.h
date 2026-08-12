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
inline constexpr auto lfo1ModFrom = "lfo1ModFrom";
inline constexpr auto lfo1Warp = "lfo1Warp";
inline constexpr auto lfo2Rate = "lfo2Rate";
inline constexpr auto lfo2Shape = "lfo2Shape";
inline constexpr auto lfo2Pitch = "lfo2Pitch";
inline constexpr auto lfo2Decay = "lfo2Decay";
inline constexpr auto lfo2ModFrom = "lfo2ModFrom";
inline constexpr auto lfo2Warp = "lfo2Warp";
inline constexpr auto lfo3Rate = "lfo3Rate";
inline constexpr auto lfo3Shape = "lfo3Shape";
inline constexpr auto lfo3Pitch = "lfo3Pitch";
inline constexpr auto lfo3Decay = "lfo3Decay";
inline constexpr auto lfo3ModFrom = "lfo3ModFrom";
inline constexpr auto lfo3Warp = "lfo3Warp";
inline constexpr auto lfo4Rate = "lfo4Rate";
inline constexpr auto lfo4Shape = "lfo4Shape";
inline constexpr auto lfo4Pitch = "lfo4Pitch";
inline constexpr auto lfo4Decay = "lfo4Decay";
inline constexpr auto lfo4ModFrom = "lfo4ModFrom";
inline constexpr auto lfo4Warp = "lfo4Warp";
inline constexpr auto masterLevel = "masterLevel";
inline constexpr auto kickNote = "kickNote";
inline constexpr auto snareNote = "snareNote";
inline constexpr auto hatNote = "hatNote";

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
void ensureMidiProperties (juce::ValueTree&);
void ensureWarblerState (juce::ValueTree&);
juce::StringArray modSourceNames (size_t targetLfo);
int modSourceForChoice (size_t targetLfo, int choice) noexcept;
juce::String frequencyText (float hz);
}
