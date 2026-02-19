#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "midi/MidiFileBuilder.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

using namespace chordpumper;

namespace {

juce::MidiMessageSequence readFirstTrack(const juce::File& file) {
    juce::FileInputStream stream(file);
    juce::MidiFile midi;
    if (stream.openedOk() && midi.readFrom(stream))
        if (midi.getNumTracks() > 0)
            return *midi.getTrack(0);
    return {};
}

short readTimeFormat(const juce::File& file) {
    juce::FileInputStream stream(file);
    juce::MidiFile midi;
    if (stream.openedOk() && midi.readFrom(stream))
        return midi.getTimeFormat();
    return 0;
}

} // anonymous namespace

TEST_CASE("createMidiFile produces a file on disk", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());
    REQUIRE(file.getSize() > 0);
    file.deleteFile();
}

TEST_CASE("C major MIDI file has 3 note-ons and 3 note-offs", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    int noteOns = 0, noteOffs = 0;
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto& msg = track.getEventPointer(i)->message;
        if (msg.isNoteOn()) ++noteOns;
        if (msg.isNoteOff()) ++noteOffs;
    }
    CHECK(noteOns == 3);
    CHECK(noteOffs == 3);
    file.deleteFile();
}

TEST_CASE("C major note numbers are 60, 64, 67", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    std::vector<int> notes;
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto& msg = track.getEventPointer(i)->message;
        if (msg.isNoteOn())
            notes.push_back(msg.getNoteNumber());
    }
    REQUIRE(notes == std::vector<int>{60, 64, 67});
    file.deleteFile();
}

TEST_CASE("Am7 produces 4 note-ons with correct MIDI notes", "[MidiFileBuilder]") {
    Chord chord{pitches::A, ChordType::Min7};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    std::vector<int> notes;
    int noteOns = 0, noteOffs = 0;
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto& msg = track.getEventPointer(i)->message;
        if (msg.isNoteOn()) { ++noteOns; notes.push_back(msg.getNoteNumber()); }
        if (msg.isNoteOff()) ++noteOffs;
    }
    CHECK(noteOns == 4);
    CHECK(noteOffs == 4);
    REQUIRE(notes == std::vector<int>{69, 72, 76, 79});
    file.deleteFile();
}

TEST_CASE("Note-on timestamps are all zero", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto* evt = track.getEventPointer(i);
        if (evt->message.isNoteOn())
            CHECK(evt->message.getTimeStamp() == 0.0);
    }
    file.deleteFile();
}

TEST_CASE("Note-off timestamps are all 1920", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto* evt = track.getEventPointer(i);
        if (evt->message.isNoteOff())
            CHECK(evt->message.getTimeStamp() == 1920.0);
    }
    file.deleteFile();
}

TEST_CASE("MIDI file TPQN is 480", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());
    CHECK(readTimeFormat(file) == 480);
    file.deleteFile();
}

TEST_CASE("MIDI file contains 120 BPM tempo event", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto file = MidiFileBuilder::createMidiFile(chord, 4);
    REQUIRE(file.existsAsFile());

    auto track = readFirstTrack(file);
    bool found = false;
    for (int i = 0; i < track.getNumEvents(); ++i) {
        auto& msg = track.getEventPointer(i)->message;
        if (msg.isTempoMetaEvent()) {
            found = true;
            CHECK(msg.getTempoSecondsPerQuarterNote() == Catch::Approx(0.5).margin(0.001));
        }
    }
    CHECK(found);
    file.deleteFile();
}

TEST_CASE("exportToDirectory writes chord-named file", "[MidiFileBuilder]") {
    Chord chord{pitches::C, ChordType::Major};
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("chordpumper_test_export");
    dir.deleteRecursively();

    auto file = MidiFileBuilder::exportToDirectory(chord, 4, dir);
    REQUIRE(file.existsAsFile());
    CHECK(file.getFileName() == juce::String("C.mid"));
    CHECK(file.getSize() > 0);
    dir.deleteRecursively();
}

TEST_CASE("exportToDirectory creates parent directory", "[MidiFileBuilder]") {
    Chord chord{pitches::D, ChordType::Min7};
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("chordpumper_test_nested/subdir");
    dir.getParentDirectory().deleteRecursively();

    auto file = MidiFileBuilder::exportToDirectory(chord, 4, dir);
    REQUIRE(file.existsAsFile());
    CHECK(file.getFileName() == juce::String("Dm7.mid"));
    dir.getParentDirectory().deleteRecursively();
}
