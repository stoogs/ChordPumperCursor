#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "PadComponent.h"
#include "../PersistentState.h"
#include "engine/MorphEngine.h"
#include "engine/VoiceLeader.h"
#include <functional>
#include <vector>

namespace chordpumper {

class GridPanel : public juce::Component
{
public:
    GridPanel(juce::MidiKeyboardState& keyboardState,
              PersistentState& state,
              juce::CriticalSection& stateLock);
    ~GridPanel() override;

    void resized() override;
    void refreshFromState();
    void morphTo(const Chord& chord);

private:
    void startPreview(const Chord& chord);
    void stopPreview();
    void releaseCurrentChord();

    juce::MidiKeyboardState& keyboardState;
    PersistentState& persistentState;
    juce::CriticalSection& stateLock;
    juce::OwnedArray<PadComponent> pads;
    std::vector<int> activeNotes;
    MorphEngine morphEngine;

    float velocity = 0.8f;
    static constexpr int midiChannel = 1;
    static constexpr int defaultOctave = 4;
};

} // namespace chordpumper
