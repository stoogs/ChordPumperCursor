#pragma once

#include "engine/Chord.h"
#include "../PersistentState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace chordpumper {

class ProgressionStrip : public juce::Component,
                         public juce::DragAndDropTarget
{
public:
    ProgressionStrip(PersistentState& state, juce::CriticalSection& stateLock);

    void addChord(const Chord& chord);
    void setChords(const std::vector<Chord>& newChords);
    void clear();
    const std::vector<Chord>& getChords() const;
    bool isEmpty() const;
    void refreshFromState();

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;

    static constexpr int kMaxChords = 8;

private:
    void updateClearButton();

    PersistentState& persistentState;
    juce::CriticalSection& stateLock;
    std::vector<Chord> chords;
    juce::TextButton clearButton{"Clear"};
    bool isReceivingDrag = false;
};

} // namespace chordpumper
