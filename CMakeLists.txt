cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime: the plugin has no VC++ redistributable dependency on the user's PC.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(Lumivore VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Lumivore
    COMPANY_NAME               "Lumivore Audio"
    PRODUCT_NAME               "Lumivore"
    PLUGIN_MANUFACTURER_CODE   Lmva
    PLUGIN_CODE                Lmv1
    FORMATS                    VST3 Standalone
    VST3_CATEGORIES            Fx Analyzer
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE)

target_sources(Lumivore PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VisualizerComponent.cpp
    Source/Shaders.cpp)

target_compile_definitions(Lumivore PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(Lumivore
    PRIVATE juce::juce_audio_utils juce::juce_opengl
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# JUCE-free core tests (run with: ctest, or build target LumivoreCoreTests)
enable_testing()
add_executable(LumivoreCoreTests tests/core_test.cpp)
target_compile_features(LumivoreCoreTests PRIVATE cxx_std_17)
add_test(NAME core COMMAND LumivoreCoreTests)
