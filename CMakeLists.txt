cmake_minimum_required(VERSION 3.22)

project(BassSplitter VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCEのパスを指定
add_subdirectory(/Volumes/AUDITIVE/development/JUCE JUCE)

juce_add_plugin(BassSplitter
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Bspl
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "BassSplitter"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "BassSplitter needs microphone access for audio input."
)

target_sources(BassSplitter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/SpectrumAnalyzer.cpp
        Source/GUI/SpectrumDisplay.cpp
        Source/GUI/LevelMeter.cpp
        Source/GUI/FaderMeter.cpp
)

target_compile_definitions(BassSplitter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BassSplitter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
