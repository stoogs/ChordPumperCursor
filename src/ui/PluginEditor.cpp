#include "PluginEditor.h"
#include "../PluginProcessor.h"

namespace chordpumper {

ChordPumperEditor::ChordPumperEditor(ChordPumperProcessor& p)
    : AudioProcessorEditor(&p), processor(p), gridPanel(p.getKeyboardState())
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(gridPanel);
    setSize(1000, 600);
}

ChordPumperEditor::~ChordPumperEditor()
{
    setLookAndFeel(nullptr);
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
    gridPanel.setBounds(area);
}

} // namespace chordpumper
