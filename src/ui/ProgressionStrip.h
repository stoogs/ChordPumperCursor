#pragma once

#include "engine/Chord.h"
#include "../PersistentState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
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

    std::function<void(const Chord&)> onChordClicked;
    std::function<void(const Chord&)> onChordDropped;
    std::function<void(const Chord&)> onPressStart;
    std::function<void(const Chord&)> onPressEnd;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    static constexpr int kMaxChords = 8;

private:
    void updateClearButton();
    void updateExportButton();
    void exportProgression();
    int getChordIndexAtPosition(juce::Point<int> pos) const;
    int insertionIndexAtX(int xPos) const;

    int reorderDragFromIndex = -1;
    int insertionIndex = -1;
    int pressedIndex = -1;

    PersistentState& persistentState;
    juce::CriticalSection& stateLock;
    std::vector<Chord> chords;
    juce::TextButton clearButton{"Clear"};
    juce::TextButton exportButton{"Export"};
    std::unique_ptr<juce::FileChooser> fileChooser;
    bool isReceivingDrag = false;
};

} // namespace chordpumper
