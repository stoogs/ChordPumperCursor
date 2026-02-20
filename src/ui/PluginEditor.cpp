#include "PluginEditor.h"
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
    gridPanel.onChordPlayed = [this](const Chord& c) { progressionStrip.addChord(c); };
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

} // namespace chordpumper
