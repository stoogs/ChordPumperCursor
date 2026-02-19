#pragma once

#include "engine/Chord.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace chordpumper {

class ProgressionStrip : public juce::Component
{
public:
    ProgressionStrip();

    void addChord(const Chord& chord);
    void clear();
    const std::vector<Chord>& getChords() const;
    bool isEmpty() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int kMaxChords = 8;

private:
    void updateClearButton();

    std::vector<Chord> chords;
    juce::TextButton clearButton{"Clear"};
};

} // namespace chordpumper
