#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/PitchClass.h"

using namespace chordpumper;

TEST_CASE("PitchClass semitone values for all 12 canonical pitches", "[pitch]") {
    struct Case { PitchClass pc; int expected; };
    auto c = GENERATE(
        Case{pitches::C,   0},
        Case{pitches::Cs,  1},
        Case{pitches::D,   2},
        Case{pitches::Eb,  3},
        Case{pitches::E,   4},
        Case{pitches::F,   5},
        Case{pitches::Fs,  6},
        Case{pitches::G,   7},
        Case{pitches::Ab,  8},
        Case{pitches::A,   9},
        Case{pitches::Bb, 10},
        Case{pitches::B,  11}
    );
    CAPTURE(c.expected);
    REQUIRE(c.pc.semitone() == c.expected);
}

TEST_CASE("PitchClass semitone edge cases", "[pitch]") {
    SECTION("Cb wraps to 11") {
        PitchClass cb{NoteLetter::C, -1};
        REQUIRE(cb.semitone() == 11);
    }
    SECTION("Fb equals 4") {
        PitchClass fb{NoteLetter::F, -1};
        REQUIRE(fb.semitone() == 4);
    }
    SECTION("B# wraps to 0") {
        PitchClass bs{NoteLetter::B, 1};
        REQUIRE(bs.semitone() == 0);
    }
    SECTION("E# equals 5") {
        PitchClass es{NoteLetter::E, 1};
        REQUIRE(es.semitone() == 5);
    }
    SECTION("Dbb equals 0") {
        PitchClass dbb{NoteLetter::D, -2};
        REQUIRE(dbb.semitone() == 0);
    }
    SECTION("C## equals 2") {
        PitchClass css{NoteLetter::C, 2};
        REQUIRE(css.semitone() == 2);
    }
}

TEST_CASE("PitchClass name generation", "[pitch]") {
    SECTION("12 canonical pitches") {
        CHECK(pitches::C.name()  == "C");
        CHECK(pitches::Cs.name() == "C#");
        CHECK(pitches::D.name()  == "D");
        CHECK(pitches::Eb.name() == "Eb");
        CHECK(pitches::E.name()  == "E");
        CHECK(pitches::F.name()  == "F");
        CHECK(pitches::Fs.name() == "F#");
        CHECK(pitches::G.name()  == "G");
        CHECK(pitches::Ab.name() == "Ab");
        CHECK(pitches::A.name()  == "A");
        CHECK(pitches::Bb.name() == "Bb");
        CHECK(pitches::B.name()  == "B");
    }
    SECTION("double accidentals") {
        PitchClass css{NoteLetter::C, 2};
        CHECK(css.name() == "C##");

        PitchClass dbb{NoteLetter::D, -2};
        CHECK(dbb.name() == "Dbb");
    }
    SECTION("natural has no suffix") {
        PitchClass natural{NoteLetter::G, 0};
        CHECK(natural.name() == "G");
    }
}

TEST_CASE("PitchClass MIDI note numbers", "[pitch]") {
    CHECK(pitches::C.midiNote(4)  == 60);
    CHECK(pitches::A.midiNote(4)  == 69);
    CHECK(pitches::C.midiNote(5)  == 72);
    CHECK(pitches::C.midiNote(-1) == 0);
    CHECK(pitches::Fs.midiNote(4) == 66);
    CHECK(pitches::B.midiNote(3)  == 59);
}

TEST_CASE("PitchClass equality", "[pitch]") {
    SECTION("same note is equal") {
        REQUIRE(pitches::C == pitches::C);
        REQUIRE(pitches::Fs == pitches::Fs);
    }
    SECTION("enharmonic equivalents are not equal") {
        PitchClass cs{NoteLetter::C, 1};
        PitchClass db{NoteLetter::D, -1};
        REQUIRE_FALSE(cs == db);
    }
    SECTION("different accidentals on same letter are not equal") {
        PitchClass c_nat{NoteLetter::C, 0};
        PitchClass c_sharp{NoteLetter::C, 1};
        REQUIRE_FALSE(c_nat == c_sharp);
    }
}
