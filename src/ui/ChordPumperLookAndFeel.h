#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace chordpumper {

namespace PadColours {
    inline constexpr juce::uint32 background = 0xff2a2a3a;
    inline constexpr juce::uint32 pressed    = 0xff6c8ebf;
    inline constexpr juce::uint32 hovered    = 0xff353545;
    inline constexpr juce::uint32 text       = 0xffe0e0e0;
    inline constexpr juce::uint32 border     = 0xff4a4a5a;
} // namespace PadColours

inline juce::LookAndFeel_V4::ColourScheme getChordPumperColourScheme()
{
    return {
        0xff1e1e2e,  // windowBackground
        0xff181825,  // widgetBackground
        0xff2a2a3a,  // menuBackground
        0xff4a4a5a,  // outline
        0xffe0e0e0,  // defaultText
        0xff6c8ebf,  // defaultFill
        0xffffffff,  // highlightedText
        0xff3a3a4a,  // highlightedFill
        0xffe0e0e0   // menuText
    };
}

class ChordPumperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordPumperLookAndFeel()
        : juce::LookAndFeel_V4(getChordPumperColourScheme())
    {
    }
};

} // namespace chordpumper
