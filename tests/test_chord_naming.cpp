#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/Chord.h"

using namespace chordpumper;

TEST_CASE("Chord name spot checks with C root", "[chord][naming]") {
    CHECK(Chord{pitches::C, ChordType::Major}.name() == "C");
    CHECK(Chord{pitches::C, ChordType::Minor}.name() == "Cm");
    CHECK(Chord{pitches::C, ChordType::Diminished}.name() == "Cdim");
    CHECK(Chord{pitches::C, ChordType::Augmented}.name() == "Caug");
    CHECK(Chord{pitches::C, ChordType::Maj7}.name() == "Cmaj7");
    CHECK(Chord{pitches::C, ChordType::Min7}.name() == "Cm7");
    CHECK(Chord{pitches::C, ChordType::Dom7}.name() == "C7");
    CHECK(Chord{pitches::C, ChordType::Dim7}.name() == "Cdim7");
    CHECK(Chord{pitches::C, ChordType::HalfDim7}.name() == "Cm7b5");
}

TEST_CASE("CHRD-02 example names", "[chord][naming]") {
    CHECK(Chord{pitches::D, ChordType::Min7}.name() == "Dm7");
    CHECK(Chord{pitches::Fs, ChordType::Augmented}.name() == "F#aug");
}

TEST_CASE("Major chords have no suffix", "[chord][naming]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );

    Chord chord{root, ChordType::Major};
    CHECK(chord.name() == root.name());
}

TEST_CASE("All chord names start with root name", "[chord][naming]") {
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
    std::string chordName = chord.name();
    std::string rootName = root.name();
    CHECK(chordName.substr(0, rootName.size()) == rootName);
}

TEST_CASE("Additional naming spot checks", "[chord][naming]") {
    CHECK(Chord{pitches::Bb, ChordType::Maj7}.name() == "Bbmaj7");
    CHECK(Chord{pitches::B, ChordType::HalfDim7}.name() == "Bm7b5");
    CHECK(Chord{pitches::Ab, ChordType::Dim7}.name() == "Abdim7");
    CHECK(Chord{pitches::G, ChordType::Dom7}.name() == "G7");
    CHECK(Chord{pitches::A, ChordType::Minor}.name() == "Am");
    CHECK(Chord{pitches::Eb, ChordType::Maj7}.name() == "Ebmaj7");
}
