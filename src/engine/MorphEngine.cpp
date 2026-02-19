#include "engine/MorphEngine.h"

namespace chordpumper {

float MorphEngine::scoreDiatonic(const PitchClass& /*referenceRoot*/,
                                  const Chord& /*candidate*/) const {
    return -1.0f;
}

std::array<ScoredChord, 32> MorphEngine::morph(const Chord& /*reference*/,
                                                const std::vector<int>& /*currentVoicing*/) const {
    return {};
}

} // namespace chordpumper
