cmake_minimum_required(VERSION 3.22)
project(PattyPunch VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_subdirectory("C:/Users/blaus/Juce/JUCE" JUCE)

juce_add_plugin(PattyPunch
    COMPANY_NAME "Zuluby"
    PLUGIN_MANUFACTURER_CODE Zlby
    PLUGIN_CODE Pp81
    FORMATS VST3 Standalone
    PRODUCT_NAME "Patty Punch"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CAN_REPLACE_VST2 FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

set(PATTY_SOURCES
    Source/Parameters.cpp
    Source/DrumEngine.cpp
    Source/PluginProcessor.cpp
)

target_sources(PattyPunch PRIVATE ${PATTY_SOURCES} Source/PluginEditor.cpp)
target_include_directories(PattyPunch PRIVATE Source)
target_compile_definitions(PattyPunch PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(PattyPunch
    PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_console_app(PattyPunchTests PRODUCT_NAME "Patty Punch DSP Tests")
target_sources(PattyPunchTests PRIVATE Tests/DspTests.cpp Source/Parameters.cpp Source/DrumEngine.cpp)
target_include_directories(PattyPunchTests PRIVATE Source)
target_compile_definitions(PattyPunchTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(PattyPunchTests PRIVATE
    juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME PattyPunchDSP COMMAND PattyPunchTests)

add_custom_target(run_reaper
    COMMAND powershell -NoProfile -Command
        "Start-Process -FilePath 'C:/Program Files/REAPER (x64)/reaper.exe'"
    DEPENDS PattyPunch_VST3
    COMMENT "Launching REAPER..." VERBATIM)

add_custom_command(TARGET PattyPunch_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "C:/Users/blaus/Juce/DevVST3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/PattyPunch_artefacts/$<CONFIG>/VST3/Patty Punch.vst3"
        "C:/Users/blaus/Juce/DevVST3/Patty Punch.vst3"
    COMMENT "Copying Patty Punch VST3 to DevVST3..." VERBATIM)
