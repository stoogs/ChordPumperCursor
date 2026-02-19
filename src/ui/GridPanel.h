#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PadComponent.h"
#include <functional>

namespace chordpumper {

class GridPanel : public juce::Component
{
public:
    GridPanel();
    void resized() override;

    std::function<void(const Chord&)> onPadClicked;

private:
    juce::OwnedArray<PadComponent> pads;
};

} // namespace chordpumper
