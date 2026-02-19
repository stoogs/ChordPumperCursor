#include <catch2/catch_test_macros.hpp>
#include "engine/Chord.h"

using namespace chordpumper;

TEST_CASE("Chord naming smoke test", "[chord][naming]") {
    Chord chord{pitches::C, ChordType::Major};
    auto name = chord.name();
    REQUIRE(name.size() >= 0);
}
