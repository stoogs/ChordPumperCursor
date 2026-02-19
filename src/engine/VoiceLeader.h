#pragma once

#include "engine/Chord.h"
#include <vector>

namespace chordpumper {

struct VoicedChord {
    Chord chord;
    std::vector<int> midiNotes;
};

int voiceLeadingDistance(const std::vector<int>& from, const std::vector<int>& to);

VoicedChord optimalVoicing(const Chord& target, const std::vector<int>& previousNotes,
                           int octave);

} // namespace chordpumper
