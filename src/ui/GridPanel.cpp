#include "GridPanel.h"
#include "midi/ChromaticPalette.h"

namespace chordpumper {

GridPanel::GridPanel(juce::MidiKeyboardState& state)
    : keyboardState(state)
{
    auto palette = chromaticPalette();

    for (int i = 0; i < 32; ++i)
    {
        auto* pad = pads.add(new PadComponent());
        pad->setChord(palette[static_cast<size_t>(i)]);
        pad->onClick = [this](const Chord& chord) { padClicked(chord); };
        addAndMakeVisible(pad);
    }
}

GridPanel::~GridPanel()
{
    releaseCurrentChord();
}

void GridPanel::padClicked(const Chord& chord)
{
    releaseCurrentChord();

    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);

    for (auto note : voiced.midiNotes)
        keyboardState.noteOn(midiChannel, note, velocity);

    activeNotes.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
    startTimer(noteDurationMs);

    auto suggestions = morphEngine.morph(chord, voiced.midiNotes);

    for (int i = 0; i < 32; ++i)
    {
        pads[i]->setChord(suggestions[static_cast<size_t>(i)].chord);
        pads[i]->setRomanNumeral(suggestions[static_cast<size_t>(i)].romanNumeral);
    }

    repaint();
}

void GridPanel::releaseCurrentChord()
{
    for (auto note : activeNotes)
        keyboardState.noteOff(midiChannel, note, 0.0f);

    activeNotes.clear();
}

void GridPanel::timerCallback()
{
    releaseCurrentChord();
    stopTimer();
}

void GridPanel::resized()
{
    juce::Grid grid;
    grid.setGap(juce::Grid::Px(4));

    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    for (int i = 0; i < 8; ++i)
        grid.templateColumns.add(Track(Fr(1)));

    for (int i = 0; i < 4; ++i)
        grid.templateRows.add(Track(Fr(1)));

    for (auto* pad : pads)
        grid.items.add(juce::GridItem(*pad));

    grid.performLayout(getLocalBounds());
}

} // namespace chordpumper
