#include "engine/Chord.h"

namespace chordpumper {

int Chord::noteCount() const { return 0; }

std::vector<int> Chord::midiNotes(int octave) const { return {}; }

std::string Chord::name() const { return ""; }

} // namespace chordpumper
