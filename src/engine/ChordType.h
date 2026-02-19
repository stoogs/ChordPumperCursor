#pragma once

#include <array>
#include <cstdint>

namespace chordpumper {

enum class ChordType : uint8_t {
    Major, Minor, Diminished, Augmented,
    Maj7, Min7, Dom7, Dim7, HalfDim7
};

inline constexpr std::array<std::array<int, 4>, 9> kIntervals = {{
    {0, 4, 7, -1},   // Major
    {0, 3, 7, -1},   // Minor
    {0, 3, 6, -1},   // Diminished
    {0, 4, 8, -1},   // Augmented
    {0, 4, 7, 11},   // Maj7
    {0, 3, 7, 10},   // Min7
    {0, 4, 7, 10},   // Dom7
    {0, 3, 6,  9},   // Dim7
    {0, 3, 6, 10},   // HalfDim7
}};

inline constexpr std::array<const char*, 9> kChordSuffix = {
    "", "m", "dim", "aug", "maj7", "m7", "7", "dim7", "m7b5"
};

inline constexpr int noteCount(ChordType type) {
    return (static_cast<int>(type) < 4) ? 3 : 4;
}

} // namespace chordpumper
