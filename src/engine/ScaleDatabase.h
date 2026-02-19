#pragma once

#include "engine/ChordType.h"
#include <array>

namespace chordpumper {

struct ScalePattern {
    std::array<int, 7> intervals;
    std::array<ChordType, 7> triadQualities;
    std::array<ChordType, 7> seventhQualities;
};

inline constexpr std::array<ScalePattern, 7> kModePatterns = {{
    // Ionian (major)
    {{0, 2, 4, 5, 7, 9, 11},
     {ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major,
      ChordType::Major, ChordType::Minor, ChordType::Diminished},
     {ChordType::Maj7, ChordType::Min7, ChordType::Min7, ChordType::Maj7,
      ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7}},
    // Dorian
    {{0, 2, 3, 5, 7, 9, 10},
     {ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major,
      ChordType::Minor, ChordType::Diminished, ChordType::Major},
     {ChordType::Min7, ChordType::Min7, ChordType::Maj7, ChordType::Dom7,
      ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7}},
    // Phrygian
    {{0, 1, 3, 5, 7, 8, 10},
     {ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor,
      ChordType::Diminished, ChordType::Major, ChordType::Minor},
     {ChordType::Min7, ChordType::Maj7, ChordType::Dom7, ChordType::Min7,
      ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7}},
    // Lydian
    {{0, 2, 4, 6, 7, 9, 11},
     {ChordType::Major, ChordType::Major, ChordType::Minor, ChordType::Diminished,
      ChordType::Major, ChordType::Minor, ChordType::Minor},
     {ChordType::Maj7, ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7,
      ChordType::Maj7, ChordType::Min7, ChordType::Min7}},
    // Mixolydian
    {{0, 2, 4, 5, 7, 9, 10},
     {ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Major,
      ChordType::Minor, ChordType::Minor, ChordType::Major},
     {ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7,
      ChordType::Min7, ChordType::Min7, ChordType::Maj7}},
    // Aeolian (natural minor)
    {{0, 2, 3, 5, 7, 8, 10},
     {ChordType::Minor, ChordType::Diminished, ChordType::Major, ChordType::Minor,
      ChordType::Minor, ChordType::Major, ChordType::Major},
     {ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7,
      ChordType::Min7, ChordType::Maj7, ChordType::Dom7}},
    // Locrian
    {{0, 1, 3, 5, 6, 8, 10},
     {ChordType::Diminished, ChordType::Major, ChordType::Minor, ChordType::Minor,
      ChordType::Major, ChordType::Major, ChordType::Minor},
     {ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7, ChordType::Min7,
      ChordType::Maj7, ChordType::Dom7, ChordType::Min7}},
}};

} // namespace chordpumper
