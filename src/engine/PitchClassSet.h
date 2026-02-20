#pragma once

#include "engine/Chord.h"
#include "engine/ChordType.h"
#include "engine/PitchClass.h"
#include <array>
#include <cstdint>

namespace chordpumper {

using PitchClassSet = uint16_t;

inline constexpr PitchClassSet pitchClassSet(const Chord& chord) {
    PitchClassSet set = 0;
    const auto& intervals = kIntervals[static_cast<int>(chord.type)];
    int rootSemitone = chord.root.semitone();
    int count = noteCount(chord.type);
    for (int i = 0; i < count; ++i) {
        int pc = (rootSemitone + intervals[static_cast<size_t>(i)]) % 12;
        set |= static_cast<uint16_t>(1u << pc);
    }
    return set;
}

inline constexpr int commonToneCount(PitchClassSet a, PitchClassSet b) {
    return __builtin_popcount(a & b);
}

inline constexpr std::array<Chord, 216> allChords() {
    constexpr std::array<PitchClass, 12> roots = {
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    };
    constexpr std::array<ChordType, 18> types = {
        ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7, ChordType::Dim7, ChordType::HalfDim7,
        ChordType::Maj9, ChordType::Maj11, ChordType::Maj13,
        ChordType::Min9, ChordType::Min11, ChordType::Min13,
        ChordType::Dom9, ChordType::Dom11, ChordType::Dom13
    };
    std::array<Chord, 216> result{};
    int idx = 0;
    for (const auto& root : roots)
        for (auto type : types)
            result[static_cast<size_t>(idx++)] = Chord{root, type};
    return result;
}

inline constexpr auto kAllChords = allChords();

} // namespace chordpumper
