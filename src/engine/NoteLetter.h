#pragma once

#include <array>
#include <cstdint>

namespace chordpumper {

enum class NoteLetter : uint8_t { C, D, E, F, G, A, B };

inline constexpr std::array<int, 7> kNaturalSemitones = {0, 2, 4, 5, 7, 9, 11};

inline constexpr std::array<const char*, 7> kLetterNames = {"C", "D", "E", "F", "G", "A", "B"};

} // namespace chordpumper
