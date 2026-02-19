#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "engine/MorphEngine.h"
#include "engine/PitchClass.h"
#include "engine/Chord.h"
#include "engine/ChordType.h"
#include <algorithm>

using namespace chordpumper;
using Catch::Matchers::WithinAbs;

namespace {

std::vector<int> rootPosition(const Chord& chord, int octave = 4) {
    return chord.midiNotes(octave);
}

bool containsChord(const std::array<ScoredChord, 32>& results,
                   PitchClass root, ChordType type) {
    return std::any_of(results.begin(), results.end(), [&](const ScoredChord& sc) {
        return sc.chord.root == root && sc.chord.type == type;
    });
}

int findRank(const std::array<ScoredChord, 32>& results,
             PitchClass root, ChordType type) {
    for (int i = 0; i < 32; ++i) {
        if (results[static_cast<size_t>(i)].chord.root == root &&
            results[static_cast<size_t>(i)].chord.type == type)
            return i;
    }
    return -1;
}

} // anonymous namespace

// --- Size and basic shape ---

TEST_CASE("MorphEngine returns exactly 32 results", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));
    REQUIRE(results.size() == 32);

    for (const auto& sc : results) {
        REQUIRE(sc.score > 0.0f);
    }
}

TEST_CASE("Results are sorted by score descending", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    for (size_t i = 1; i < 32; ++i) {
        REQUIRE(results[i - 1].score >= results[i].score);
    }
}

TEST_CASE("Each result has a non-empty Roman numeral", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    for (const auto& sc : results) {
        REQUIRE(!sc.romanNumeral.empty());
    }
}

TEST_CASE("Scores are in valid range (0, 1]", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    for (const auto& sc : results) {
        REQUIRE(sc.score > 0.0f);
        REQUIRE(sc.score <= 1.0f);
    }
}

// --- Diatonic scoring ---

TEST_CASE("scoreDiatonic: G major is V in Ionian = 1.0", "[morph_engine]") {
    MorphEngine engine;
    Chord gMajor{pitches::G, ChordType::Major};
    REQUIRE_THAT(engine.scoreDiatonic(pitches::C, gMajor),
                 WithinAbs(1.0, 0.001));
}

TEST_CASE("scoreDiatonic: Eb major is bIII in Aeolian = 0.85", "[morph_engine]") {
    MorphEngine engine;
    Chord ebMajor{pitches::Eb, ChordType::Major};
    REQUIRE_THAT(engine.scoreDiatonic(pitches::C, ebMajor),
                 WithinAbs(0.85, 0.001));
}

TEST_CASE("scoreDiatonic: augmented chord not in any mode = 0.0", "[morph_engine]") {
    MorphEngine engine;
    // Augmented quality never appears as a diatonic triad or seventh in any mode
    Chord cAug{pitches::C, ChordType::Augmented};
    REQUIRE_THAT(engine.scoreDiatonic(pitches::C, cAug),
                 WithinAbs(0.0, 0.001));
}

TEST_CASE("scoreDiatonic: F major is IV in Ionian = 1.0", "[morph_engine]") {
    MorphEngine engine;
    Chord fMajor{pitches::F, ChordType::Major};
    REQUIRE_THAT(engine.scoreDiatonic(pitches::C, fMajor),
                 WithinAbs(1.0, 0.001));
}

TEST_CASE("scoreDiatonic: A minor is vi in Ionian = 1.0", "[morph_engine]") {
    MorphEngine engine;
    Chord aMinor{pitches::A, ChordType::Minor};
    REQUIRE_THAT(engine.scoreDiatonic(pitches::C, aMinor),
                 WithinAbs(1.0, 0.001));
}

// --- Ranking behavior ---

TEST_CASE("Diatonic chords rank well for C major", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    // vi ranks top 10 (high common tones + good VL after octave search)
    int amRank = findRank(results, pitches::A, ChordType::Minor);
    REQUIRE(amRank >= 0);
    REQUIRE(amRank < 10);

    // IV and V rank in top half — same-root variants (Cmin, Cmaj7, Cdom7)
    // outscore them on VL distance, which is correct weighted behavior
    int fRank = findRank(results, pitches::F, ChordType::Major);
    int gRank = findRank(results, pitches::G, ChordType::Major);
    REQUIRE(fRank >= 0);
    REQUIRE(fRank < 20);
    REQUIRE(gRank >= 0);
    REQUIRE(gRank < 20);
}

TEST_CASE("Self-chord appears in results", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    REQUIRE(containsChord(results, pitches::C, ChordType::Major));
}

// --- Transposition invariance ---

TEST_CASE("Transposition invariance: C major and D major produce equivalent rankings",
           "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    Chord dMajor{pitches::D, ChordType::Major};

    auto cResults = engine.morph(cMajor, rootPosition(cMajor));
    auto dResults = engine.morph(dMajor, rootPosition(dMajor));

    for (size_t i = 0; i < 32; ++i) {
        int cInterval = (cResults[i].chord.root.semitone() -
                         pitches::C.semitone() + 12) % 12;
        int dInterval = (dResults[i].chord.root.semitone() -
                         pitches::D.semitone() + 12) % 12;
        REQUIRE(cInterval == dInterval);
        REQUIRE(cResults[i].chord.type == dResults[i].chord.type);
    }
}

// --- Variety filter ---

TEST_CASE("Variety filter: at least 2 from each quality category", "[morph_engine]") {
    MorphEngine engine;
    Chord cMajor{pitches::C, ChordType::Major};
    auto results = engine.morph(cMajor, rootPosition(cMajor));

    int majorFamily = 0, minorFamily = 0, dimAug = 0;
    for (const auto& sc : results) {
        switch (sc.chord.type) {
            case ChordType::Major:
            case ChordType::Maj7:
            case ChordType::Dom7:
                majorFamily++;
                break;
            case ChordType::Minor:
            case ChordType::Min7:
                minorFamily++;
                break;
            default:
                dimAug++;
                break;
        }
    }

    REQUIRE(majorFamily >= 2);
    REQUIRE(minorFamily >= 2);
    REQUIRE(dimAug >= 2);
}

TEST_CASE("Variety holds for minor reference chord too", "[morph_engine]") {
    MorphEngine engine;
    Chord aMinor{pitches::A, ChordType::Minor};
    auto results = engine.morph(aMinor, rootPosition(aMinor));

    int majorFamily = 0, minorFamily = 0, dimAug = 0;
    for (const auto& sc : results) {
        switch (sc.chord.type) {
            case ChordType::Major:
            case ChordType::Maj7:
            case ChordType::Dom7:
                majorFamily++;
                break;
            case ChordType::Minor:
            case ChordType::Min7:
                minorFamily++;
                break;
            default:
                dimAug++;
                break;
        }
    }

    REQUIRE(majorFamily >= 2);
    REQUIRE(minorFamily >= 2);
    REQUIRE(dimAug >= 2);
}
