#pragma once

#include "engine/Chord.h"
#include "engine/PitchClass.h"
#include <array>
#include <string>
#include <vector>

namespace chordpumper {

struct MorphWeights {
    float diatonic = 0.40f;
    float commonTones = 0.25f;
    float voiceLeading = 0.25f;
};

struct ScoredChord {
    Chord chord;
    float score;
    std::string romanNumeral;
};

class MorphEngine {
public:
    MorphWeights weights;

    std::array<ScoredChord, 64> morph(const Chord& reference,
                                       const std::vector<int>& currentVoicing) const;

    float scoreDiatonic(const PitchClass& referenceRoot,
                        const Chord& candidate) const;
};

} // namespace chordpumper
