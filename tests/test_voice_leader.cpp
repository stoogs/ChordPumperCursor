#include <catch2/catch_test_macros.hpp>
#include "engine/VoiceLeader.h"

using namespace chordpumper;

TEST_CASE("voiceLeadingDistance with equal-size chords", "[voice_leader]") {
    SECTION("C major → F major close voicing = 3") {
        // C4=60, E4=64, G4=67 → C4=60, F4=65, A4=69
        std::vector<int> from = {60, 64, 67};
        std::vector<int> to = {60, 65, 69};
        REQUIRE(voiceLeadingDistance(from, to) == 3);
    }
    SECTION("Finds optimal permutation regardless of order") {
        std::vector<int> from = {60, 64, 67};
        std::vector<int> to = {65, 69, 60}; // scrambled F major
        REQUIRE(voiceLeadingDistance(from, to) == 3);
    }
    SECTION("Unison = 0 distance") {
        std::vector<int> notes = {60, 64, 67};
        REQUIRE(voiceLeadingDistance(notes, notes) == 0);
    }
}

TEST_CASE("voiceLeadingDistance with empty vectors", "[voice_leader]") {
    std::vector<int> empty;
    std::vector<int> notes = {60, 64, 67};
    REQUIRE(voiceLeadingDistance(empty, notes) == 0);
    REQUIRE(voiceLeadingDistance(notes, empty) == 0);
}

TEST_CASE("voiceLeadingDistance with size mismatch (triad to 7th)", "[voice_leader]") {
    // C major triad to C major 7th — extra note B4=71, nearest source is G4=67 → +4
    std::vector<int> triad = {60, 64, 67};
    std::vector<int> seventh = {60, 64, 67, 71};
    int dist = voiceLeadingDistance(triad, seventh);
    REQUIRE(dist >= 0);
    REQUIRE(dist == 4); // 0+0+0 for matching notes + 4 for extra B nearest to G
}

TEST_CASE("Transposition invariance", "[voice_leader]") {
    // C major → F major should equal D major → G major (both shifted by 2)
    std::vector<int> cMaj = {60, 64, 67};
    std::vector<int> fMaj = {60, 65, 69};
    std::vector<int> dMaj = {62, 66, 69};
    std::vector<int> gMaj = {62, 67, 71};
    REQUIRE(voiceLeadingDistance(cMaj, fMaj) == voiceLeadingDistance(dMaj, gMaj));
}

TEST_CASE("optimalVoicing with empty previousNotes returns root position", "[voice_leader]") {
    Chord cMaj{pitches::C, ChordType::Major};
    auto voiced = optimalVoicing(cMaj, {}, 4);
    REQUIRE(voiced.chord.root == pitches::C);
    REQUIRE(voiced.chord.type == ChordType::Major);
    REQUIRE(voiced.midiNotes == std::vector<int>{60, 64, 67});
}

TEST_CASE("optimalVoicing produces close voicing near previous", "[voice_leader]") {
    // From C major root position at octave 4, voice F major close
    std::vector<int> prev = {60, 64, 67};
    Chord fMaj{pitches::F, ChordType::Major};
    auto voiced = optimalVoicing(fMaj, prev, 4);

    int dist = voiceLeadingDistance(prev, voiced.midiNotes);
    // Optimal F major near [60,64,67] should be close (e.g., [60,65,69] = distance 3)
    REQUIRE(dist <= 5);
    REQUIRE(voiced.midiNotes.size() == 3);
}

TEST_CASE("optimalVoicing for 7th chord from triad doesn't crash", "[voice_leader]") {
    std::vector<int> prev = {60, 64, 67};
    Chord cMin7{pitches::C, ChordType::Min7};
    auto voiced = optimalVoicing(cMin7, prev, 4);
    REQUIRE(voiced.midiNotes.size() == 4);
}
