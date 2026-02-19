#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/RomanNumeral.h"

using namespace chordpumper;

TEST_CASE("All 12 major chord intervals from C major reference", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};

    SECTION("Unison: C major = I")   { REQUIRE(romanNumeral(ref, {pitches::C,  ChordType::Major}) == "I"); }
    SECTION("m2: Db major = ♭II")    { REQUIRE(romanNumeral(ref, {pitches::Cs, ChordType::Major}) == "\u266dII"); }
    SECTION("M2: D major = II")      { REQUIRE(romanNumeral(ref, {pitches::D,  ChordType::Major}) == "II"); }
    SECTION("m3: Eb major = ♭III")   { REQUIRE(romanNumeral(ref, {pitches::Eb, ChordType::Major}) == "\u266dIII"); }
    SECTION("M3: E major = III")     { REQUIRE(romanNumeral(ref, {pitches::E,  ChordType::Major}) == "III"); }
    SECTION("P4: F major = IV")      { REQUIRE(romanNumeral(ref, {pitches::F,  ChordType::Major}) == "IV"); }
    SECTION("tritone: F# major = ♯IV") { REQUIRE(romanNumeral(ref, {pitches::Fs, ChordType::Major}) == "\u266fIV"); }
    SECTION("P5: G major = V")       { REQUIRE(romanNumeral(ref, {pitches::G,  ChordType::Major}) == "V"); }
    SECTION("m6: Ab major = ♭VI")    { REQUIRE(romanNumeral(ref, {pitches::Ab, ChordType::Major}) == "\u266dVI"); }
    SECTION("M6: A major = VI")      { REQUIRE(romanNumeral(ref, {pitches::A,  ChordType::Major}) == "VI"); }
    SECTION("m7: Bb major = ♭VII")   { REQUIRE(romanNumeral(ref, {pitches::Bb, ChordType::Major}) == "\u266dVII"); }
    SECTION("M7: B major = VII")     { REQUIRE(romanNumeral(ref, {pitches::B,  ChordType::Major}) == "VII"); }
}

TEST_CASE("Case sensitivity: major = uppercase, minor = lowercase", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};
    REQUIRE(romanNumeral(ref, {pitches::F, ChordType::Major}) == "IV");
    REQUIRE(romanNumeral(ref, {pitches::D, ChordType::Minor}) == "ii");
}

TEST_CASE("7th chord suffixes", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};

    SECTION("Maj7") { REQUIRE(romanNumeral(ref, {pitches::C, ChordType::Maj7}) == "I\u0394"); }
    SECTION("Min7") { REQUIRE(romanNumeral(ref, {pitches::D, ChordType::Min7}) == "ii7"); }
    SECTION("Dom7") { REQUIRE(romanNumeral(ref, {pitches::G, ChordType::Dom7}) == "V7"); }
    SECTION("Dim7") { REQUIRE(romanNumeral(ref, {pitches::B, ChordType::Dim7}) == "vii\u00b07"); }
    SECTION("HalfDim7") { REQUIRE(romanNumeral(ref, {pitches::B, ChordType::HalfDim7}) == "vii\u00f87"); }
}

TEST_CASE("Non-C reference: A major → D major = IV", "[roman_numeral]") {
    Chord ref{pitches::A, ChordType::Major};
    REQUIRE(romanNumeral(ref, {pitches::D, ChordType::Major}) == "IV");
}

TEST_CASE("Diminished uses lowercase", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};
    auto rn = romanNumeral(ref, {pitches::B, ChordType::Diminished});
    REQUIRE(rn == "vii\u00b0");
}

TEST_CASE("Augmented uses uppercase with + suffix", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};
    auto rn = romanNumeral(ref, {pitches::E, ChordType::Augmented});
    REQUIRE(rn == "III+");
}

TEST_CASE("Tritone ambiguity: ♯IV for major, ♭v for minor/dim", "[roman_numeral]") {
    Chord ref{pitches::C, ChordType::Major};

    SECTION("Major quality at tritone = ♯IV") {
        REQUIRE(romanNumeral(ref, {pitches::Fs, ChordType::Major}) == "\u266fIV");
    }
    SECTION("Minor quality at tritone = ♭v") {
        REQUIRE(romanNumeral(ref, {pitches::Fs, ChordType::Minor}) == "\u266dv");
    }
    SECTION("Diminished quality at tritone = ♭v°") {
        REQUIRE(romanNumeral(ref, {pitches::Fs, ChordType::Diminished}) == "\u266dv\u00b0");
    }
    SECTION("Augmented quality at tritone = ♯IV+") {
        REQUIRE(romanNumeral(ref, {pitches::Fs, ChordType::Augmented}) == "\u266fIV+");
    }
}

TEST_CASE("isUpperCase for all chord types", "[roman_numeral]") {
    REQUIRE(isUpperCase(ChordType::Major) == true);
    REQUIRE(isUpperCase(ChordType::Augmented) == true);
    REQUIRE(isUpperCase(ChordType::Maj7) == true);
    REQUIRE(isUpperCase(ChordType::Dom7) == true);
    REQUIRE(isUpperCase(ChordType::Minor) == false);
    REQUIRE(isUpperCase(ChordType::Diminished) == false);
    REQUIRE(isUpperCase(ChordType::Min7) == false);
    REQUIRE(isUpperCase(ChordType::Dim7) == false);
    REQUIRE(isUpperCase(ChordType::HalfDim7) == false);
}
