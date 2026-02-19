cmake_minimum_required(VERSION 3.22)

project(ChordPumper VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

add_subdirectory(libs/JUCE)

add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(ChordPumper
    COMPANY_NAME           "ChordPumperDev"
    PLUGIN_MANUFACTURER_CODE Cpdv
    PLUGIN_CODE            Cpmp
    IS_SYNTH               TRUE
    NEEDS_MIDI_INPUT       TRUE
    NEEDS_MIDI_OUTPUT      TRUE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS                VST3 Standalone
    PRODUCT_NAME           "ChordPumper"
    PLUGIN_NAME            "ChordPumper"
    DESCRIPTION            "Chord progression discovery instrument"
)

clap_juce_extensions_plugin(
    TARGET ChordPumper
    CLAP_ID "org.chordpumper.chordpumper"
    CLAP_FEATURES instrument utility
)

target_sources(ChordPumper PRIVATE
    src/PluginProcessor.cpp
    src/ui/PluginEditor.cpp
)

target_include_directories(ChordPumper PRIVATE src)

target_compile_definitions(ChordPumper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordPumper
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

add_custom_command(TARGET ChordPumper_CLAP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.clap"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ChordPumper_CLAP>" "$ENV{HOME}/.clap/"
)
