#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ChordPumperLookAndFeel.h"
#include "GridPanel.h"
#include "ProgressionStrip.h"

namespace chordpumper {

class ChordPumperProcessor;

class ChordPumperEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer
{
public:
    explicit ChordPumperEditor(ChordPumperProcessor& processor);
    ~ChordPumperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChordPumperProcessor& processor;
    ChordPumperLookAndFeel lookAndFeel;
    GridPanel gridPanel;
    ProgressionStrip progressionStrip;
};

} // namespace chordpumper
