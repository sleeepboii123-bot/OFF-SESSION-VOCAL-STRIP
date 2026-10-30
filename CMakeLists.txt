cmake_minimum_required(VERSION 3.24)
project(OFF_SESSION_VOCAL_STRIP VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OFF_SESSION_VOCAL_STRIP
    COMPANY_NAME "NYXV x OFF SESSION"
    BUNDLE_ID "com.nyxv.offsession.vocalstrip"
    PLUGIN_MANUFACTURER_CODE Nyxv
    PLUGIN_CODE Vstr
    FORMATS VST3
    PRODUCT_NAME "OFF SESSION VOCAL STRIP"
    DESCRIPTION "Modular low-latency vocal channel strip by NYXV x OFF SESSION"
    VST3_CATEGORIES Fx Dynamics EQ Reverb Delay
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(OFF_SESSION_VOCAL_STRIP)

target_sources(OFF_SESSION_VOCAL_STRIP PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/VocalStripDSP.cpp
    Source/VocalStripDSP.h
)

target_compile_definitions(OFF_SESSION_VOCAL_STRIP PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OFF_SESSION_VOCAL_STRIP PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(OFF_SESSION_VOCAL_STRIP PRIVATE /W4 /permissive-)
else()
    target_compile_options(OFF_SESSION_VOCAL_STRIP PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(OFF_SESSION_VOCAL_STRIP_DSP_TESTS
        Source/VocalStripDSP.cpp
        Tests/VocalStripDSPTests.cpp
    )
    target_include_directories(OFF_SESSION_VOCAL_STRIP_DSP_TESTS PRIVATE Source)
    target_compile_features(OFF_SESSION_VOCAL_STRIP_DSP_TESTS PRIVATE cxx_std_17)
    target_link_libraries(OFF_SESSION_VOCAL_STRIP_DSP_TESTS PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
    add_test(NAME VocalStripDSPValidation COMMAND OFF_SESSION_VOCAL_STRIP_DSP_TESTS)
endif()
