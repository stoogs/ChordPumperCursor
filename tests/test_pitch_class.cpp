#include <catch2/catch_test_macros.hpp>
#include "engine/PitchClass.h"

using namespace chordpumper;

TEST_CASE("PitchClass smoke test", "[pitch]") {
    PitchClass c{NoteLetter::C, 0};
    REQUIRE(c.letter == NoteLetter::C);
    REQUIRE(c.accidental == 0);
}
