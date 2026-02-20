#include "PersistentState.h"
#include "midi/ChromaticPalette.h"

namespace chordpumper {

namespace {
    const juce::Identifier kStateType {"ChordPumperState"};
    const juce::Identifier kGridType {"Grid"};
    const juce::Identifier kPadType {"Pad"};
    const juce::Identifier kMorphContextType {"MorphContext"};
    const juce::Identifier kProgressionType {"Progression"};
    const juce::Identifier kChordType {"Chord"};
    const juce::Identifier kWeightsType {"Weights"};

    constexpr int kCurrentStateVersion = 1;
}

PersistentState::PersistentState()
    : gridChords(chromaticPalette())
    , lastPlayedChord{pitches::C, ChordType::Major}
{
}

juce::ValueTree PersistentState::toValueTree() const
{
    juce::ValueTree root(kStateType);
    root.setProperty("version", kCurrentStateVersion, nullptr);

    juce::ValueTree grid(kGridType);
    for (int i = 0; i < 32; ++i)
    {
        auto idx = static_cast<size_t>(i);
        juce::ValueTree pad(kPadType);
        pad.setProperty("index", i, nullptr);
        pad.setProperty("root", static_cast<int>(gridChords[idx].root.letter), nullptr);
        pad.setProperty("accidental", static_cast<int>(gridChords[idx].root.accidental), nullptr);
        pad.setProperty("type", static_cast<int>(gridChords[idx].type), nullptr);
        pad.setProperty("roman", juce::String(romanNumerals[idx]), nullptr);
        grid.addChild(pad, -1, nullptr);
    }
    root.addChild(grid, -1, nullptr);

    if (hasMorphed)
    {
        juce::ValueTree morph(kMorphContextType);
        morph.setProperty("root", static_cast<int>(lastPlayedChord.root.letter), nullptr);
        morph.setProperty("accidental", static_cast<int>(lastPlayedChord.root.accidental), nullptr);
        morph.setProperty("type", static_cast<int>(lastPlayedChord.type), nullptr);

        juce::String voicingStr;
        for (size_t j = 0; j < lastVoicing.size(); ++j)
        {
            if (j > 0) voicingStr += ",";
            voicingStr += juce::String(lastVoicing[j]);
        }
        morph.setProperty("voicing", voicingStr, nullptr);
        root.addChild(morph, -1, nullptr);
    }

    juce::ValueTree prog(kProgressionType);
    for (const auto& chord : progression)
    {
        juce::ValueTree c(kChordType);
        c.setProperty("root", static_cast<int>(chord.root.letter), nullptr);
        c.setProperty("accidental", static_cast<int>(chord.root.accidental), nullptr);
        c.setProperty("type", static_cast<int>(chord.type), nullptr);
        prog.addChild(c, -1, nullptr);
    }
    root.addChild(prog, -1, nullptr);

    juce::ValueTree w(kWeightsType);
    w.setProperty("diatonic", static_cast<double>(weights.diatonic), nullptr);
    w.setProperty("commonTones", static_cast<double>(weights.commonTones), nullptr);
    w.setProperty("voiceLeading", static_cast<double>(weights.voiceLeading), nullptr);
    root.addChild(w, -1, nullptr);

    return root;
}

PersistentState PersistentState::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.isValid() || tree.getType() != kStateType)
        return {};

    int version = tree.getProperty("version", 0);
    if (version < 1)
        return {};

    PersistentState state;

    auto grid = tree.getChildWithName(kGridType);
    if (grid.isValid())
    {
        for (int i = 0; i < grid.getNumChildren(); ++i)
        {
            auto pad = grid.getChild(i);
            int index = pad.getProperty("index", -1);
            if (index < 0 || index >= 32) continue;

            auto idx = static_cast<size_t>(index);
            state.gridChords[idx].root.letter =
                static_cast<NoteLetter>(static_cast<int>(pad.getProperty("root", 0)));
            state.gridChords[idx].root.accidental =
                static_cast<int8_t>(static_cast<int>(pad.getProperty("accidental", 0)));
            state.gridChords[idx].type =
                static_cast<ChordType>(static_cast<int>(pad.getProperty("type", 0)));
            state.romanNumerals[idx] =
                pad.getProperty("roman", "").toString().toStdString();
        }
    }

    auto morph = tree.getChildWithName(kMorphContextType);
    if (morph.isValid())
    {
        state.hasMorphed = true;
        state.lastPlayedChord.root.letter =
            static_cast<NoteLetter>(static_cast<int>(morph.getProperty("root", 0)));
        state.lastPlayedChord.root.accidental =
            static_cast<int8_t>(static_cast<int>(morph.getProperty("accidental", 0)));
        state.lastPlayedChord.type =
            static_cast<ChordType>(static_cast<int>(morph.getProperty("type", 0)));

        juce::String voicingStr = morph.getProperty("voicing", "");
        if (voicingStr.isNotEmpty())
        {
            auto tokens = juce::StringArray::fromTokens(voicingStr, ",", "");
            for (const auto& tok : tokens)
                state.lastVoicing.push_back(tok.getIntValue());
        }
    }

    auto prog = tree.getChildWithName(kProgressionType);
    if (prog.isValid())
    {
        for (int i = 0; i < prog.getNumChildren(); ++i)
        {
            auto c = prog.getChild(i);
            Chord chord;
            chord.root.letter =
                static_cast<NoteLetter>(static_cast<int>(c.getProperty("root", 0)));
            chord.root.accidental =
                static_cast<int8_t>(static_cast<int>(c.getProperty("accidental", 0)));
            chord.type =
                static_cast<ChordType>(static_cast<int>(c.getProperty("type", 0)));
            state.progression.push_back(chord);
        }
    }

    auto w = tree.getChildWithName(kWeightsType);
    if (w.isValid())
    {
        state.weights.diatonic =
            static_cast<float>(static_cast<double>(w.getProperty("diatonic", 0.40)));
        state.weights.commonTones =
            static_cast<float>(static_cast<double>(w.getProperty("commonTones", 0.25)));
        state.weights.voiceLeading =
            static_cast<float>(static_cast<double>(w.getProperty("voiceLeading", 0.25)));
    }

    return state;
}

} // namespace chordpumper
