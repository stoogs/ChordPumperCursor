#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/PitchClassSet.h"
#include <set>

using namespace chordpumper;

TEST_CASE("pitchClassSet builds correct bitmask for triads", "[pitch_class_set]") {
    SECTION("C major = bits 0, 4, 7") {
        auto set = pitchClassSet({pitches::C, ChordType::Major});
        REQUIRE(set == 0x091); // 0b10010001
        REQUIRE(__builtin_popcount(set) == 3);
    }
    SECTION("F major = bits 0, 5, 9") {
        auto set = pitchClassSet({pitches::F, ChordType::Major});
        REQUIRE(set == 0x221); // bits 0, 5, 9
        REQUIRE(__builtin_popcount(set) == 3);
    }
    SECTION("A minor = bits 0, 4, 9") {
        auto set = pitchClassSet({pitches::A, ChordType::Minor});
        REQUIRE(set == 0x211); // bits 0, 4, 9
        REQUIRE(__builtin_popcount(set) == 3);
    }
}

TEST_CASE("pitchClassSet for 7th chords has 4 bits set", "[pitch_class_set]") {
    auto type = GENERATE(
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7,
        ChordType::Dim7, ChordType::HalfDim7
    );
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto set = pitchClassSet({root, type});
    REQUIRE(__builtin_popcount(set) == 4);
}

TEST_CASE("pitchClassSet for triads has 3 bits set", "[pitch_class_set]") {
    auto type = GENERATE(
        ChordType::Major, ChordType::Minor, ChordType::Diminished
    );
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto set = pitchClassSet({root, type});
    REQUIRE(__builtin_popcount(set) == 3);
}

TEST_CASE("commonToneCount between chords", "[pitch_class_set]") {
    auto cMaj = pitchClassSet({pitches::C, ChordType::Major});
    auto fMaj = pitchClassSet({pitches::F, ChordType::Major});
    auto aMin = pitchClassSet({pitches::A, ChordType::Minor});

    SECTION("C major vs F major = 1 common tone (C)") {
        REQUIRE(commonToneCount(cMaj, fMaj) == 1);
    }
    SECTION("C major vs A minor = 2 common tones (C, E)") {
        REQUIRE(commonToneCount(cMaj, aMin) == 2);
    }
    SECTION("C major vs C major = 3 common tones") {
        REQUIRE(commonToneCount(cMaj, cMaj) == 3);
    }
    SECTION("No common tones between distant chords") {
        auto fsMaj = pitchClassSet({pitches::Fs, ChordType::Major});
        REQUIRE(commonToneCount(cMaj, fsMaj) == 0);
    }
}

TEST_CASE("kAllChords has 108 entries", "[pitch_class_set]") {
    REQUIRE(kAllChords.size() == 108);
}

TEST_CASE("kAllChords first entry is C Major", "[pitch_class_set]") {
    REQUIRE(kAllChords[0].root == pitches::C);
    REQUIRE(kAllChords[0].type == ChordType::Major);
}

TEST_CASE("kAllChords last entry is B HalfDim7", "[pitch_class_set]") {
    REQUIRE(kAllChords[107].root == pitches::B);
    REQUIRE(kAllChords[107].type == ChordType::HalfDim7);
}

TEST_CASE("kAllChords has no duplicate root+type pairs", "[pitch_class_set]") {
    std::set<std::pair<int, int>> seen;
    for (const auto& chord : kAllChords) {
        auto key = std::make_pair(chord.root.semitone(), static_cast<int>(chord.type));
        REQUIRE(seen.insert(key).second);
    }
}

TEST_CASE("Enharmonic equivalence: C# major and Db major same PitchClassSet", "[pitch_class_set]") {
    PitchClass db{NoteLetter::D, -1};
    auto csSet = pitchClassSet({pitches::Cs, ChordType::Major});
    auto dbSet = pitchClassSet({db, ChordType::Major});
    REQUIRE(csSet == dbSet);
}
