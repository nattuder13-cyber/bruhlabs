cmake_minimum_required(VERSION 3.22)
project(MorphKick VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COPY_AFTER_BUILD "Copy plugin to system VST3 folder after build" ON)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MorphKick
    COMPANY_NAME "DraftAudio"
    PLUGIN_MANUFACTURER_CODE Drft
    PLUGIN_CODE Mkck
    FORMATS VST3 Standalone
    PRODUCT_NAME "MorphKick"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD}
    VST3_CATEGORIES Instrument Synth)

target_sources(MorphKick PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MorphKick PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MorphKick
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
