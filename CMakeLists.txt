cmake_minimum_required(VERSION 3.22)
project(PattyPunch VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

# Prefer a JUCE checkout supplied at configure time. If this repository gains a
# local JUCE checkout, use that automatically; otherwise use an installed JUCE
# package.
set(JUCE_PATH "" CACHE PATH "Path to a JUCE source checkout")

if(JUCE_PATH)
    get_filename_component(JUCE_PATH "${JUCE_PATH}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
        message(FATAL_ERROR
            "JUCE_PATH does not point to a JUCE source checkout: ${JUCE_PATH}")
    endif()

    add_subdirectory("${JUCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

set(PATTY_PUNCH_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PATTY_PUNCH_FORMATS AU)
endif()

juce_add_plugin(PattyPunch
    COMPANY_NAME "Zuluby"
    PLUGIN_MANUFACTURER_CODE Zlby
    PLUGIN_CODE Pp81
    FORMATS ${PATTY_PUNCH_FORMATS}
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
target_sources(PattyPunchTests PRIVATE Tests/DspTests.cpp)
target_include_directories(PattyPunchTests PRIVATE Source)
target_compile_definitions(PattyPunchTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(PattyPunchTests PRIVATE
    PattyPunch juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME PattyPunchDSP COMMAND PattyPunchTests)

if(WIN32)
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
endif()

if(APPLE)
    set(PATTY_PUNCH_VST3_INSTALL_DIR
        "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")

    add_custom_command(TARGET PattyPunch_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${PATTY_PUNCH_VST3_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PattyPunch_VST3>"
            "${PATTY_PUNCH_VST3_INSTALL_DIR}/Patty Punch.vst3"
        COMMENT "Copying Patty Punch VST3 to ${PATTY_PUNCH_VST3_INSTALL_DIR}..."
        VERBATIM)
endif()
