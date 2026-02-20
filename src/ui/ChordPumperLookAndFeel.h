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
        constexpr juce::uint32 accents[18] = {
            majorAccent, minorAccent, diminishedAccent, augmentedAccent,
            maj7Accent, min7Accent, dom7Accent, dim7Accent, halfDim7Accent,
            0xff4db8ff,  // Maj9  — brighter blue
            0xff80ccff,  // Maj11 — lighter blue
            0xffaadeff,  // Maj13 — palest blue
            0xffc4abff,  // Min9  — lighter purple
            0xffd5c2ff,  // Min11 — even lighter purple
            0xffe8d8ff,  // Min13 — palest purple
            0xff7dd6eb,  // Dom9  — lighter teal
            0xff9de3f2,  // Dom11 — even lighter teal
            0xffbdeef8,  // Dom13 — palest teal
        };
        return accents[static_cast<int>(type)];
    }

    inline juce::Colour similarityColour(float score)
    {
        struct Stop { float pos; juce::Colour colour; };
        static const Stop stops[] = {
            { 0.00f, juce::Colour(0xffF44336) },  // red — extremely dissimilar
            { 0.25f, juce::Colour(0xff9C27B0) },  // purple — exotic
            { 0.45f, juce::Colour(0xffFF9800) },  // orange — bold
            { 0.65f, juce::Colour(0xff4CAF50) },  // green — good
            { 0.85f, juce::Colour(0xff4A9EFF) },  // blue — very similar
            { 1.00f, juce::Colour(0xff4A9EFF) },  // blue (clamp)
        };

        score = juce::jlimit(0.0f, 1.0f, score);
        for (int i = 0; i < 5; ++i)
        {
            if (score <= stops[i + 1].pos)
            {
                float t = (score - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
                return stops[i].colour.interpolatedWith(stops[i + 1].colour, t);
            }
        }
        return stops[5].colour;
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
