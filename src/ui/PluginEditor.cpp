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
    progressionStrip.onChordClicked = [this](const Chord& c) {
        auto& ks = processor.getKeyboardState();
        auto notes = c.midiNotes(4);
        for (auto n : notes) ks.noteOn(1, n, 0.8f);
        juce::Timer::callAfterDelay(300,
            [safeThis = juce::Component::SafePointer<ChordPumperEditor>(this), notes]() {
                if (safeThis == nullptr) return;
                auto& ks2 = safeThis->processor.getKeyboardState();
                for (auto n : notes) ks2.noteOff(1, n, 0.0f);
            });
    };

    processor.addChangeListener(this);
    setSize(1000, 650);
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
    g.setFont(juce::Font(juce::FontOptions(18.0f)));
    g.drawText("ChordPumper", getLocalBounds().removeFromTop(40),
               juce::Justification::centred);
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
