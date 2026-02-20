#pragma once

#include "engine/Chord.h"
#include <array>

namespace chordpumper {

inline std::array<Chord, 64> chromaticPalette()
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
        // Row 4: Dominant 7th (8 common roots)
        {C, CT::Dom7}, {D, CT::Dom7}, {E, CT::Dom7}, {F, CT::Dom7},
        {G, CT::Dom7}, {A, CT::Dom7}, {Bb, CT::Dom7}, {B, CT::Dom7},
        // Row 5: Minor 7th
        {C, CT::Min7}, {D, CT::Min7}, {E, CT::Min7}, {F, CT::Min7},
        {G, CT::Min7}, {A, CT::Min7}, {Bb, CT::Min7}, {B, CT::Min7},
        // Row 6: Major 7th
        {C, CT::Maj7}, {D, CT::Maj7}, {E, CT::Maj7}, {F, CT::Maj7},
        {G, CT::Maj7}, {A, CT::Maj7}, {Bb, CT::Maj7}, {B, CT::Maj7},
        // Row 7: Half-diminished 7th (4) + Diminished 7th (4)
        {C, CT::HalfDim7}, {D, CT::HalfDim7}, {E, CT::HalfDim7}, {F, CT::HalfDim7},
        {C, CT::Dim7}, {Eb, CT::Dim7}, {Fs, CT::Dim7}, {A, CT::Dim7},
    }};
}

} // namespace chordpumper
