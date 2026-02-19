#include "PadComponent.h"

namespace chordpumper {

void PadComponent::setChord(const Chord& c)
{
    chord = c;
    repaint();
}

const Chord& PadComponent::getChord() const
{
    return chord;
}

void PadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    constexpr float cornerSize = 6.0f;

    if (isPressed)
        g.setColour(juce::Colour(0xff6c8ebf));
    else if (isHovered)
        g.setColour(juce::Colour(0xff353545));
    else
        g.setColour(juce::Colour(0xff2a2a3a));

    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText(juce::String(chord.name()), getLocalBounds(),
               juce::Justification::centred);
}

void PadComponent::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    repaint();
    if (onClick)
        onClick(chord);
}

void PadComponent::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    repaint();
}

void PadComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void PadComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    isPressed = false;
    repaint();
}

} // namespace chordpumper
