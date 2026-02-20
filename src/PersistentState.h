#pragma once

#include "engine/Chord.h"
#include "engine/MorphEngine.h"
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <string>
#include <vector>

namespace chordpumper {

struct PersistentState {
    std::array<Chord, 32> gridChords;
    std::array<std::string, 32> romanNumerals;
    Chord lastPlayedChord;
    std::vector<int> lastVoicing;
    std::vector<Chord> progression;
    MorphWeights weights;
    bool hasMorphed = false;

    PersistentState();

    juce::ValueTree toValueTree() const;
    static PersistentState fromValueTree(const juce::ValueTree& tree);
};

} // namespace chordpumper
