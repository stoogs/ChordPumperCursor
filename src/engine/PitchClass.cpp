#include "engine/PitchClass.h"

namespace chordpumper {

std::string PitchClass::name() const {
    std::string result = kLetterNames[static_cast<int>(letter)];
    if (accidental > 0) {
        for (int i = 0; i < accidental; ++i) result += '#';
    } else if (accidental < 0) {
        for (int i = 0; i < -accidental; ++i) result += 'b';
    }
    return result;
}

int PitchClass::midiNote(int octave) const {
    return semitone() + (octave + 1) * 12;
}

} // namespace chordpumper
