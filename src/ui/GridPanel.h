#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "PadComponent.h"
#include <vector>

namespace chordpumper {

class GridPanel : public juce::Component, private juce::Timer
{
public:
    explicit GridPanel(juce::MidiKeyboardState& keyboardState);
    ~GridPanel() override;

    void resized() override;

private:
    void padClicked(const Chord& chord);
    void releaseCurrentChord();
    void timerCallback() override;

    juce::MidiKeyboardState& keyboardState;
    juce::OwnedArray<PadComponent> pads;
    std::vector<int> activeNotes;

    float velocity = 0.8f;
    static constexpr int midiChannel = 1;
    static constexpr int defaultOctave = 4;
    static constexpr int noteDurationMs = 300;
};

} // namespace chordpumper
