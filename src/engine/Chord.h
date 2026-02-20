#pragma once

#include "engine/PitchClass.h"
#include "engine/ChordType.h"
#include <string>
#include <vector>

namespace chordpumper {

struct Chord {
    PitchClass root;
    ChordType type;
    int octaveOffset = 0;         // semitone octave shift applied at preview/playback (+1 = up, -1 = down)
    std::string romanNumeral;     // Roman numeral label captured at drag time (e.g. "IV", "vi")

    int noteCount() const;
    std::vector<int> midiNotes(int octave) const;
    std::string name() const;
};

} // namespace chordpumper
