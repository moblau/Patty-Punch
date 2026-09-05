#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace Params
{
inline constexpr int version = 1;
enum class ModDestination
{
    off,
    kickPitch, kickSweep, kickDecay, kickClick, kickDrive, kickLevel,
    snarePitch, snareDecay, snareSnappy, snareTone, snareDrive, snareLevel,
    hatPitch, hatDecay, hatTone, hatCharacter, hatLevel,
    tomPitch, tomSweep, tomDecay, tomTone, tomAttack, tomLevel,
    count
};
inline constexpr size_t modDestinationCount = static_cast<size_t> (ModDestination::count);
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
inline constexpr auto snareDrive = "snareDrive";
inline constexpr auto hatTune = "hatTune";
inline constexpr auto hatDecay = "hatDecay";
inline constexpr auto hatTone = "hatTone";
inline constexpr auto hatHighPass = "hatHighPass";
inline constexpr auto hatChoke = "hatChoke";
inline constexpr auto hatLevel = "hatLevel";
inline constexpr auto tomTune = "tomTune";
inline constexpr auto tomSweep = "tomSweep";
inline constexpr auto tomDecay = "tomDecay";
inline constexpr auto tomTone = "tomTone";
inline constexpr auto tomAttack = "tomAttack";
inline constexpr auto tomLevel = "tomLevel";
inline constexpr auto kickRepeatCount = "kickRepeatCount";
inline constexpr auto kickRepeatTime = "kickRepeatTime";
inline constexpr auto kickRepeatShape = "kickRepeatShape";
inline constexpr auto snareRepeatCount = "snareRepeatCount";
inline constexpr auto snareRepeatTime = "snareRepeatTime";
inline constexpr auto snareRepeatShape = "snareRepeatShape";
inline constexpr auto hatRepeatCount = "hatRepeatCount";
inline constexpr auto hatRepeatTime = "hatRepeatTime";
inline constexpr auto hatRepeatShape = "hatRepeatShape";
inline constexpr auto tomRepeatCount = "tomRepeatCount";
inline constexpr auto tomRepeatTime = "tomRepeatTime";
inline constexpr auto tomRepeatShape = "tomRepeatShape";
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
inline constexpr std::array<const char*, 4> lfoTargetA {
    "lfo1TargetA", "lfo2TargetA", "lfo3TargetA", "lfo4TargetA" };
inline constexpr std::array<const char*, 4> lfoDepthA {
    "lfo1DepthA", "lfo2DepthA", "lfo3DepthA", "lfo4DepthA" };
inline constexpr std::array<const char*, 4> lfoTargetB {
    "lfo1TargetB", "lfo2TargetB", "lfo3TargetB", "lfo4TargetB" };
inline constexpr std::array<const char*, 4> lfoDepthB {
    "lfo1DepthB", "lfo2DepthB", "lfo3DepthB", "lfo4DepthB" };
inline constexpr auto masterLevel = "masterLevel";
inline constexpr auto kickNote = "kickNote";
inline constexpr auto snareNote = "snareNote";
inline constexpr auto hatNote = "hatNote";
inline constexpr auto tomNote = "tomNote";

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
void ensureMidiProperties (juce::ValueTree&);
void ensureWarblerState (juce::ValueTree&);
void ensureCurrentState (juce::ValueTree&);
juce::StringArray modSourceNames (size_t targetLfo);
juce::StringArray modDestinationNames();
int modSourceForChoice (size_t targetLfo, int choice) noexcept;
juce::String frequencyText (float hz);
}
