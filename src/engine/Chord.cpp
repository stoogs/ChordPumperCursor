#include "engine/Chord.h"

namespace chordpumper {

int Chord::noteCount() const {
    return chordpumper::noteCount(type);
}

std::vector<int> Chord::midiNotes(int octave) const {
    int rootMidi = root.midiNote(octave);
    auto& intervals = kIntervals[static_cast<int>(type)];
    int count = noteCount();
    std::vector<int> notes;
    notes.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        notes.push_back(rootMidi + intervals[static_cast<size_t>(i)]);
    return notes;
}

std::string Chord::name() const {
    return root.name() + kChordSuffix[static_cast<int>(type)];
}

} // namespace chordpumper
