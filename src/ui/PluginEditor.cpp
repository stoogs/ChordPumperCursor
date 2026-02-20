#include "PluginEditor.h"
#include "PadComponent.h"
#include "midi/MidiFileBuilder.h"
#include "../PluginProcessor.h"

namespace chordpumper {

ChordPumperEditor::ChordPumperEditor(ChordPumperProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      gridPanel(p.getKeyboardState(), p.getState(), p.getStateLock()),
      progressionStrip(p.getState(), p.getStateLock())
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(gridPanel);
    addAndMakeVisible(progressionStrip);
    progressionStrip.onPressStart = [this](const Chord& c) {
        auto& ks = processor.getKeyboardState();
        auto notes = c.midiNotes(4 + c.octaveOffset);
        for (auto n : notes) ks.noteOn(1, n, 0.8f);
        stripActiveNotes = std::vector<int>(notes.begin(), notes.end());
    };

    progressionStrip.onPressEnd = [this](const Chord&) {
        auto& ks = processor.getKeyboardState();
        for (auto n : stripActiveNotes) ks.noteOff(1, n, 0.0f);
        stripActiveNotes.clear();
    };

    progressionStrip.onChordClicked = [this](const Chord& c) {
        gridPanel.morphTo(c);
    };

    progressionStrip.onChordDropped = [this](const Chord& c) {
        gridPanel.morphTo(c);
    };

    processor.addChangeListener(this);
    setSize(1000, 600);
}

ChordPumperEditor::~ChordPumperEditor()
{
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void ChordPumperEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    gridPanel.refreshFromState();
    progressionStrip.refreshFromState();
}

void ChordPumperEditor::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(juce::Font(juce::FontOptions(20.0f)));
    g.drawText("ChordPumper", getLocalBounds().removeFromTop(40),
               juce::Justification::centred);
    g.setColour(juce::Colour(0xff4a4a5a).withAlpha(0.5f));
    g.drawHorizontalLine(40, 10.0f, static_cast<float>(getWidth() - 10));
}

void ChordPumperEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(40);
    auto stripArea = area.removeFromBottom(50);
    area.removeFromBottom(6);
    gridPanel.setBounds(area);
    progressionStrip.setBounds(stripArea);
}

bool ChordPumperEditor::shouldDropFilesWhenDraggedExternally(
    const juce::DragAndDropTarget::SourceDetails& details,
    juce::StringArray& files,
    bool& canMoveFiles)
{
    if (auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get()))
    {
        auto midiFile = MidiFileBuilder::createMidiFile(pad->getChord(), 4);
        if (midiFile.existsAsFile())
        {
            files.add(midiFile.getFullPathName());
            canMoveFiles = false;
            return true;
        }
    }
    return false;
}

} // namespace chordpumper
