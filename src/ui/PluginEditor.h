#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ChordPumperLookAndFeel.h"

namespace chordpumper {

class ChordPumperProcessor;

class ChordPumperEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChordPumperEditor(ChordPumperProcessor& processor);
    ~ChordPumperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChordPumperProcessor& processor;
    ChordPumperLookAndFeel lookAndFeel;
};

} // namespace chordpumper
