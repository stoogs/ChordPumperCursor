cmake_minimum_required(VERSION 3.22)

project(ChordPumper VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
endif()

# Pin glibc symbol versions for DAW host compatibility.
# CachyOS glibc 2.43 redirects math/string functions to newer symbol
# versions via IEC 60559 / C23 feature macros.  Bitwig's plugin sandbox
# bundles an older glibc that lacks them.
#
# Two-layer fix:
#  1. Compile-time: cmake/glibc-compat/bits/libc-header-start.h intercepts
#     every system header and zeros the feature macros so object files emit
#     unversioned or classic symbol references.
#  2. Link-time: cmake/glibc_compat_math.c + --wrap flags pin unversioned
#     math symbols to GLIBC_2.2.5 instead of the linker's default (newest).
add_compile_options(-isystem ${CMAKE_SOURCE_DIR}/cmake/glibc-compat)

add_library(ChordPumperEngine STATIC
    src/engine/PitchClass.cpp
    src/engine/Chord.cpp
    src/engine/VoiceLeader.cpp
    src/engine/RomanNumeral.cpp
    src/engine/MorphEngine.cpp
)
set_target_properties(ChordPumperEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(ChordPumperEngine PUBLIC src)
target_compile_features(ChordPumperEngine PUBLIC cxx_std_20)

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
    src/ui/PadComponent.cpp
    src/ui/GridPanel.cpp
    src/ui/ProgressionStrip.cpp
    src/midi/MidiFileBuilder.cpp
    cmake/glibc_compat_math.c
)

target_include_directories(ChordPumper PRIVATE src)

target_compile_definitions(ChordPumper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ChordPumper
    PRIVATE
        ChordPumperEngine
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Link-time: --wrap redirects unversioned math calls through our pinned wrappers
set(GLIBC_WRAP_FLAGS
    -Wl,--wrap=sqrtf,--wrap=atan2f,--wrap=fmod,--wrap=hypot,--wrap=hypotf)
foreach(_target ChordPumper_VST3 ChordPumper_Standalone ChordPumper_CLAP)
    if(TARGET ${_target})
        target_link_options(${_target} PRIVATE ${GLIBC_WRAP_FLAGS})
    endif()
endforeach()

add_custom_command(TARGET ChordPumper_CLAP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.clap"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ChordPumper_CLAP>" "$ENV{HOME}/.clap/"
)

option(CHORDPUMPER_BUILD_TESTS "Build unit tests" ON)

if(CHORDPUMPER_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)

    add_executable(ChordPumperTests
        tests/test_pitch_class.cpp
        tests/test_chord.cpp
        tests/test_chord_naming.cpp
        tests/test_pitch_class_set.cpp
        tests/test_voice_leader.cpp
        tests/test_roman_numeral.cpp
        tests/test_morph_engine.cpp
        tests/test_midi_file_builder.cpp
        src/midi/MidiFileBuilder.cpp
    )
    target_include_directories(ChordPumperTests PRIVATE src)
    target_compile_definitions(ChordPumperTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    target_link_libraries(ChordPumperTests PRIVATE
        ChordPumperEngine
        Catch2::Catch2WithMain
        juce::juce_audio_basics
    )
    catch_discover_tests(ChordPumperTests)
endif()
