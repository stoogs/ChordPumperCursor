#include "PluginEditor.h"
#include "../PluginProcessor.h"

namespace chordpumper {

ChordPumperEditor::ChordPumperEditor(ChordPumperProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
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
    g.setFont(juce::Font(28.0f));
    g.drawText("ChordPumper v0.1.0", getLocalBounds(), juce::Justification::centred);
}

void ChordPumperEditor::resized()
{
}

} // namespace chordpumper
