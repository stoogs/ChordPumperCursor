#pragma once

#include "engine/PitchClass.h"
#include "engine/ChordType.h"
#include <string>
#include <vector>

namespace chordpumper {

struct Chord {
    PitchClass root;
    ChordType type;

    int noteCount() const;
    std::vector<int> midiNotes(int octave) const;
    std::string name() const;
};

} // namespace chordpumper
