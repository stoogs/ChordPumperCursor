#include <catch2/catch_test_macros.hpp>
#include "engine/Chord.h"

using namespace chordpumper;

TEST_CASE("Chord smoke test", "[chord]") {
    Chord chord{pitches::C, ChordType::Major};
    REQUIRE(chord.root == pitches::C);
    REQUIRE(chord.type == ChordType::Major);
}
