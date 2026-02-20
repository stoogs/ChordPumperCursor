#include "PadComponent.h"
#include "ChordPumperLookAndFeel.h"
#include "midi/MidiFileBuilder.h"

namespace chordpumper {

void PadComponent::setChord(const Chord& c)
{
    chord = c;
    repaint();
}

void PadComponent::setRomanNumeral(const std::string& rn)
{
    romanNumeral_ = rn;
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

    auto baseColour = isPressed ? juce::Colour(PadColours::pressed)
                    : isHovered ? juce::Colour(PadColours::hovered)
                                : juce::Colour(PadColours::background);
    auto gradient = juce::ColourGradient::vertical(
        baseColour.brighter(0.05f), baseColour.darker(0.05f), bounds);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);

    float accentAlpha = isPressed ? 0.8f : isHovered ? 0.6f : 0.4f;
    g.setColour(juce::Colour(PadColours::accentForType(chord.type)).withAlpha(accentAlpha));
    g.drawRoundedRectangle(bounds, cornerSize, 1.5f);

    auto textArea = getLocalBounds();

    if (romanNumeral_.empty())
    {
        g.setColour(juce::Colour(0xffe0e0e0));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawText(juce::String(chord.name()), textArea,
                   juce::Justification::centred);
    }
    else
    {
        auto topHalf = textArea.removeFromTop(textArea.getHeight() / 2);
        auto bottomHalf = textArea;

        g.setColour(juce::Colour(0xffe0e0e0));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawText(juce::String(chord.name()), topHalf,
                   juce::Justification::centredBottom);

        g.setColour(juce::Colour(0xffaaaaaa));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(juce::String(romanNumeral_), bottomHalf,
                   juce::Justification::centredTop);
    }
}

void PadComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        auto exportDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                             .getChildFile("ChordPumper-Export");
        MidiFileBuilder::exportToDirectory(chord, 4, exportDir);

        // Brief visual flash to confirm export
        isPressed = true;
        repaint();
        juce::Timer::callAfterDelay(200, [safeThis = juce::Component::SafePointer<PadComponent>(this)]() {
            if (safeThis != nullptr)
            {
                safeThis->isPressed = false;
                safeThis->repaint();
            }
        });
        return;
    }

    isPressed = true;
    isDragInProgress = false;
    repaint();

    if (onPressStart)
        onPressStart(chord);
}

void PadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 6)
        return;

    if (isDragInProgress)
        return;

    isDragInProgress = true;

    if (onPressEnd)
        onPressEnd(chord);

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        container->startDragging(juce::var(juce::String(chord.name())), this, juce::ScaledImage{}, false);
}

void PadComponent::mouseUp(const juce::MouseEvent& event)
{
    isPressed = false;
    repaint();

    if (!isDragInProgress)
    {
        if (onPressEnd)
            onPressEnd(chord);
        if (event.getDistanceFromDragStart() < 6 && onClick)
            onClick(chord);
    }
    isDragInProgress = false;
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
