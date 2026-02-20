#include "GridPanel.h"
#include "midi/ChromaticPalette.h"

namespace chordpumper {

namespace {

std::array<Chord, 4> makeSubChords(PitchClass root,
                                    ChordType tl, ChordType tr,
                                    ChordType bl, ChordType br)
{
    return { Chord{root, tl}, Chord{root, tr}, Chord{root, bl}, Chord{root, br} };
}

void applySubVariations(PadComponent& pad, const Chord& chord)
{
    switch (chord.type)
    {
        case ChordType::Major:
            pad.setSubVariations(true, makeSubChords(chord.root,
                ChordType::Major, ChordType::Maj7, ChordType::Maj9, ChordType::Maj13));
            break;
        case ChordType::Minor:
            pad.setSubVariations(true, makeSubChords(chord.root,
                ChordType::Minor, ChordType::Min7, ChordType::Min9, ChordType::Min11));
            break;
        case ChordType::Maj7:
            pad.setSubVariations(true, makeSubChords(chord.root,
                ChordType::Maj7, ChordType::Maj9, ChordType::Maj11, ChordType::Maj13));
            break;
        case ChordType::Min7:
            pad.setSubVariations(true, makeSubChords(chord.root,
                ChordType::Min7, ChordType::Min9, ChordType::Min11, ChordType::Min13));
            break;
        case ChordType::Dom7:
            pad.setSubVariations(true, makeSubChords(chord.root,
                ChordType::Dom7, ChordType::Dom9, ChordType::Dom11, ChordType::Dom13));
            break;
        default:
            pad.setSubVariations(false, {});
            break;
    }
}

} // anonymous namespace

GridPanel::GridPanel(juce::MidiKeyboardState& ks,
                     PersistentState& state,
                     juce::CriticalSection& lock)
    : keyboardState(ks), persistentState(state), stateLock(lock)
{
    {
        const juce::ScopedLock sl(stateLock);
        morphEngine.weights = persistentState.weights;
    }

    for (int i = 0; i < 64; ++i)
    {
        auto* pad = pads.add(new PadComponent());
        pad->onPressStart = [this](const Chord& c) { startPreview(c); };
        pad->onPressEnd   = [this](const Chord&)   { stopPreview(); };
        addAndMakeVisible(pad);
    }

    refreshFromState();
}

GridPanel::~GridPanel()
{
    releaseCurrentChord();
}

void GridPanel::startPreview(const Chord& chord)
{
    releaseCurrentChord();
    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);
    for (auto note : voiced.midiNotes)
        keyboardState.noteOn(midiChannel, note, velocity);
    activeNotes.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
}

void GridPanel::stopPreview()
{
    releaseCurrentChord();
}

void GridPanel::morphTo(const Chord& chord)
{
    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);
    auto suggestions = morphEngine.morph(chord, voiced.midiNotes);

    for (int i = 0; i < 64; ++i)
    {
        const auto& suggestion = suggestions[static_cast<size_t>(i)];
        pads[i]->setChord(suggestion.chord);
        pads[i]->setRomanNumeral(suggestion.romanNumeral);
        pads[i]->setScore(suggestion.score);
        applySubVariations(*pads[i], suggestion.chord);
    }

    {
        const juce::ScopedLock sl(stateLock);
        persistentState.lastPlayedChord = chord;
        persistentState.lastVoicing.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
        persistentState.hasMorphed = true;
        for (int i = 0; i < 64; ++i)
        {
            persistentState.gridChords[static_cast<size_t>(i)] = suggestions[static_cast<size_t>(i)].chord;
            persistentState.romanNumerals[static_cast<size_t>(i)] = suggestions[static_cast<size_t>(i)].romanNumeral;
        }
    }
    repaint();
}

void GridPanel::releaseCurrentChord()
{
    for (auto note : activeNotes)
        keyboardState.noteOff(midiChannel, note, 0.0f);

    activeNotes.clear();
}

void GridPanel::refreshFromState()
{
    const juce::ScopedLock sl(stateLock);

    if (persistentState.hasMorphed)
    {
        for (int i = 0; i < 64; ++i)
        {
            const auto& c = persistentState.gridChords[static_cast<size_t>(i)];
            pads[i]->setChord(c);
            pads[i]->setRomanNumeral(persistentState.romanNumerals[static_cast<size_t>(i)]);
            pads[i]->setScore(-1.0f);
            applySubVariations(*pads[i], c);
        }
        activeNotes = persistentState.lastVoicing;
    }
    else
    {
        auto palette = chromaticPalette();
        for (int i = 0; i < 64; ++i)
        {
            const auto& c = palette[static_cast<size_t>(i)];
            pads[i]->setChord(c);
            pads[i]->setRomanNumeral({});
            pads[i]->setScore(-1.0f);
            applySubVariations(*pads[i], c);
        }
        activeNotes.clear();
    }

    morphEngine.weights = persistentState.weights;
    repaint();
}

void GridPanel::resized()
{
    juce::Grid grid;
    grid.setGap(juce::Grid::Px(4));

    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    for (int i = 0; i < 8; ++i)
        grid.templateColumns.add(Track(Fr(1)));

    for (int i = 0; i < 8; ++i)
        grid.templateRows.add(Track(Fr(1)));

    for (auto* pad : pads)
        grid.items.add(juce::GridItem(*pad));

    grid.performLayout(getLocalBounds());
}

} // namespace chordpumper
