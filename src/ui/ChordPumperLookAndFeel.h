#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/ChordType.h"

namespace chordpumper {

namespace PadColours {
    inline constexpr juce::uint32 background = 0xff2a2a3a;
    inline constexpr juce::uint32 pressed    = 0xff6c8ebf;
    inline constexpr juce::uint32 hovered    = 0xff353545;
    inline constexpr juce::uint32 text       = 0xffe0e0e0;
    inline constexpr juce::uint32 border     = 0xff4a4a5a;

    inline constexpr juce::uint32 majorAccent      = 0xff4a9eff; // blue
    inline constexpr juce::uint32 minorAccent      = 0xff9b6dff; // purple
    inline constexpr juce::uint32 diminishedAccent = 0xffff6b6b; // red-ish
    inline constexpr juce::uint32 augmentedAccent  = 0xffffb347; // orange
    inline constexpr juce::uint32 maj7Accent       = 0xff6baed6; // light blue
    inline constexpr juce::uint32 min7Accent       = 0xffb39ddb; // light purple
    inline constexpr juce::uint32 dom7Accent       = 0xff5bc0de; // teal
    inline constexpr juce::uint32 dim7Accent       = 0xffef5350; // bright red
    inline constexpr juce::uint32 halfDim7Accent   = 0xffff8a65; // salmon

    inline juce::uint32 accentForType(ChordType type) {
        constexpr juce::uint32 accents[] = {
            majorAccent, minorAccent, diminishedAccent, augmentedAccent,
            maj7Accent, min7Accent, dom7Accent, dim7Accent, halfDim7Accent
        };
        return accents[static_cast<int>(type)];
    }
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
