#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/Chord.h"

using namespace chordpumper;

TEST_CASE("Triads produce 3 notes", "[chord]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto type = GENERATE(
        ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Augmented
    );

    Chord chord{root, type};
    REQUIRE(chord.noteCount() == 3);
    REQUIRE(chord.midiNotes(4).size() == 3);
}

TEST_CASE("Seventh chords produce 4 notes", "[chord]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto type = GENERATE(
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7,
        ChordType::Dim7, ChordType::HalfDim7
    );

    Chord chord{root, type};
    REQUIRE(chord.noteCount() == 4);
    REQUIRE(chord.midiNotes(4).size() == 4);
}

TEST_CASE("C major MIDI notes at octave 4", "[chord]") {
    Chord chord{pitches::C, ChordType::Major};
    REQUIRE(chord.midiNotes(4) == std::vector<int>{60, 64, 67});
}

TEST_CASE("All chord MIDI notes start with root", "[chord]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto type = GENERATE(
        ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Augmented,
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7,
        ChordType::Dim7, ChordType::HalfDim7
    );

    Chord chord{root, type};
    auto notes = chord.midiNotes(4);
    REQUIRE(notes[0] == root.midiNote(4));
}

TEST_CASE("Chord intervals match lookup table", "[chord]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto type = GENERATE(
        ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Augmented,
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7,
        ChordType::Dim7, ChordType::HalfDim7
    );

    Chord chord{root, type};
    auto notes = chord.midiNotes(4);
    int rootMidi = root.midiNote(4);
    auto& intervals = kIntervals[static_cast<int>(type)];
    int count = chord.noteCount();

    for (int i = 0; i < count; ++i) {
        REQUIRE(notes[static_cast<size_t>(i)] == rootMidi + intervals[static_cast<size_t>(i)]);
    }
}

TEST_CASE("Specific chord MIDI note spot checks", "[chord]") {
    SECTION("C minor at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Minor}.midiNotes(4) == std::vector<int>{60, 63, 67});
    }
    SECTION("C diminished at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Diminished}.midiNotes(4) == std::vector<int>{60, 63, 66});
    }
    SECTION("C augmented at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Augmented}.midiNotes(4) == std::vector<int>{60, 64, 68});
    }
    SECTION("C maj7 at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Maj7}.midiNotes(4) == std::vector<int>{60, 64, 67, 71});
    }
    SECTION("C min7 at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Min7}.midiNotes(4) == std::vector<int>{60, 63, 67, 70});
    }
    SECTION("C dom7 at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Dom7}.midiNotes(4) == std::vector<int>{60, 64, 67, 70});
    }
    SECTION("C dim7 at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::Dim7}.midiNotes(4) == std::vector<int>{60, 63, 66, 69});
    }
    SECTION("C half-dim7 at octave 4") {
        REQUIRE(Chord{pitches::C, ChordType::HalfDim7}.midiNotes(4) == std::vector<int>{60, 63, 66, 70});
    }
    SECTION("F# major at octave 4") {
        REQUIRE(Chord{pitches::Fs, ChordType::Major}.midiNotes(4) == std::vector<int>{66, 70, 73});
    }
    SECTION("Bb min7 at octave 4") {
        REQUIRE(Chord{pitches::Bb, ChordType::Min7}.midiNotes(4) == std::vector<int>{70, 73, 77, 80});
    }
}
