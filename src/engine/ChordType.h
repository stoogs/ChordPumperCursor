#pragma once

#include <array>
#include <cstdint>

namespace chordpumper {

enum class ChordType : uint8_t {
    Major, Minor, Diminished, Augmented,
    Maj7, Min7, Dom7, Dim7, HalfDim7,
    Maj9 = 9, Maj11 = 10, Maj13 = 11,
    Min9 = 12, Min11 = 13, Min13 = 14,
    Dom9 = 15, Dom11 = 16, Dom13 = 17
};

inline constexpr std::array<std::array<int, 6>, 18> kIntervals = {{
    {0, 4, 7, -1, -1, -1},  // Major
    {0, 3, 7, -1, -1, -1},  // Minor
    {0, 3, 6, -1, -1, -1},  // Diminished
    {0, 4, 8, -1, -1, -1},  // Augmented
    {0, 4, 7, 11, -1, -1},  // Maj7
    {0, 3, 7, 10, -1, -1},  // Min7
    {0, 4, 7, 10, -1, -1},  // Dom7
    {0, 3, 6,  9, -1, -1},  // Dim7
    {0, 3, 6, 10, -1, -1},  // HalfDim7
    {0, 4, 7, 11, 14, -1},  // Maj9
    {0, 4, 7, 11, 14, 17},  // Maj11
    {0, 4, 7, 11, 14, 21},  // Maj13
    {0, 3, 7, 10, 14, -1},  // Min9
    {0, 3, 7, 10, 14, 17},  // Min11
    {0, 3, 7, 10, 14, 21},  // Min13
    {0, 4, 7, 10, 14, -1},  // Dom9
    {0, 4, 7, 10, 14, 17},  // Dom11
    {0, 4, 7, 10, 14, 21},  // Dom13
}};

inline constexpr std::array<const char*, 18> kChordSuffix = {
    "", "m", "dim", "aug", "maj7", "m7", "7", "dim7", "m7b5",
    "maj9", "maj11", "maj13", "m9", "m11", "m13", "9", "11", "13"
};

inline constexpr int noteCount(ChordType type) {
    int t = static_cast<int>(type);
    if (t < 4)  return 3;   // triads
    if (t < 9)  return 4;   // 7ths
    if (t == 9 || t == 12 || t == 15) return 5;  // 9ths
    return 6;  // 11ths and 13ths
}

} // namespace chordpumper
