#pragma once

#include "engine/Chord.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>

namespace chordpumper {

class PadComponent : public juce::Component
{
public:
    void setChord(const Chord& c);
    void setRomanNumeral(const std::string& rn);
    const Chord& getChord() const;

    std::function<void(const Chord&)> onClick;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    Chord chord{};
    std::string romanNumeral_;
    bool isPressed = false;
    bool isHovered = false;
    bool isDragInProgress = false;
};

} // namespace chordpumper
