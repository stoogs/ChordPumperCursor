#pragma once

#include "engine/Chord.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>
#include <array>

namespace chordpumper {

class PadComponent : public juce::Component
{
public:
    void setChord(const Chord& c);
    void setRomanNumeral(const std::string& rn);
    void setScore(float s);
    const Chord& getChord() const;
    const Chord& getDragChord() const;
    void setSubVariations(bool enabled, const std::array<Chord, 4>& chords);

    std::function<void(const Chord&)> onClick;
    std::function<void(const Chord&)> onPressStart;
    std::function<void(const Chord&)> onPressEnd;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    Chord chord{};
    std::string romanNumeral_;
    float score_ = -1.0f;
    bool isPressed = false;
    bool isHovered = false;
    bool isDragInProgress = false;
    Chord dragChord_{};

    bool hasSubVariations = false;
    std::array<Chord, 4> subChords{};  // TL=0, TR=1, BL=2, BR=3
    int pressedQuadrant = -1;          // -1 = whole pad, 0-3 = quadrant index

    int quadrantAt(juce::Point<int> pos) const;
};

} // namespace chordpumper
