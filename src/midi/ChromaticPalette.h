#pragma once

#include "engine/Chord.h"
#include <array>

namespace chordpumper {

inline std::array<Chord, 32> chromaticPalette()
{
    using namespace pitches;
    using CT = ChordType;
    return {{
        // Row 0: Major chords C through G
        {C, CT::Major}, {Cs, CT::Major}, {D, CT::Major}, {Eb, CT::Major},
        {E, CT::Major}, {F, CT::Major},  {Fs, CT::Major}, {G, CT::Major},
        // Row 1: Major Ab–B, then Minor C–Eb
        {Ab, CT::Major}, {A, CT::Major}, {Bb, CT::Major}, {B, CT::Major},
        {C, CT::Minor},  {Cs, CT::Minor}, {D, CT::Minor}, {Eb, CT::Minor},
        // Row 2: Minor E through B
        {E, CT::Minor},  {F, CT::Minor},  {Fs, CT::Minor}, {G, CT::Minor},
        {Ab, CT::Minor}, {A, CT::Minor},  {Bb, CT::Minor}, {B, CT::Minor},
        // Row 3: 4 diminished + 4 augmented
        {C, CT::Diminished}, {D, CT::Diminished}, {E, CT::Diminished}, {Fs, CT::Diminished},
        {C, CT::Augmented},  {Eb, CT::Augmented}, {G, CT::Augmented},  {Bb, CT::Augmented},
    }};
}

} // namespace chordpumper
