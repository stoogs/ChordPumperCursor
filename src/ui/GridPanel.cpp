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

    auto notes = chord.midiNotes(defaultOctave);
    for (auto note : notes)
        keyboardState.noteOn(midiChannel, note, velocity);

    activeNotes.assign(notes.begin(), notes.end());
    startTimer(noteDurationMs);
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
