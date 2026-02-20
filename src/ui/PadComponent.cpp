#include "PadComponent.h"
#include "ChordPumperLookAndFeel.h"

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

void PadComponent::setScore(float s)
{
    score_ = s;
    repaint();
}

const Chord& PadComponent::getChord() const
{
    return chord;
}

const Chord& PadComponent::getDragChord() const
{
    return dragChord_;
}

void PadComponent::setSubVariations(bool enabled, const std::array<Chord, 4>& chords)
{
    hasSubVariations = enabled;
    subChords = chords;
    repaint();
}

int PadComponent::quadrantAt(juce::Point<int> pos) const
{
    if (!hasSubVariations) return -1;
    bool right  = pos.getX() >= getWidth()  / 2;
    bool bottom = pos.getY() >= getHeight() / 2;
    return (bottom ? 2 : 0) + (right ? 1 : 0);  // TL=0, TR=1, BL=2, BR=3
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

    // Quadrant separator lines
    if (hasSubVariations)
    {
        g.setColour(juce::Colour(0x22ffffff));
        // Vertical centre line
        g.drawLine(bounds.getCentreX(), bounds.getY() + 4.0f,
                   bounds.getCentreX(), bounds.getBottom() - 4.0f, 1.0f);
        // Horizontal centre line
        g.drawLine(bounds.getX() + 4.0f, bounds.getCentreY(),
                   bounds.getRight() - 4.0f, bounds.getCentreY(), 1.0f);
    }

    float accentAlpha = isPressed ? 0.8f : isHovered ? 0.6f : 0.4f;
    auto accentColour = (score_ >= 0.0f)
        ? PadColours::similarityColour(score_)
        : juce::Colour(PadColours::accentForType(chord.type));

    // Glow rings (hover only) — painted BEFORE the solid border
    if (isHovered)
    {
        g.setColour(accentColour.withAlpha(0.07f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 9.0f);
        g.setColour(accentColour.withAlpha(0.13f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 6.0f);
        g.setColour(accentColour.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 4.0f);
    }
    // Solid border — always drawn, 3px
    g.setColour(accentColour.withAlpha(accentAlpha));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 3.0f);

    auto textArea = getLocalBounds();

    if (hasSubVariations)
    {
        g.setFont(juce::Font(juce::FontOptions(7.5f)));
        constexpr juce::uint32 quadrantColour = 0xffcccccc;
        g.setColour(juce::Colour(quadrantColour));
        auto w = getWidth() / 2;
        auto h = getHeight() / 2;
        // TL quadrant (index 0)
        g.drawText(juce::String(subChords[0].name()),
                   juce::Rectangle<int>(0, 0, w, h), juce::Justification::centred);
        // TR quadrant (index 1)
        g.drawText(juce::String(subChords[1].name()),
                   juce::Rectangle<int>(w, 0, w, h), juce::Justification::centred);
        // BL quadrant (index 2)
        g.drawText(juce::String(subChords[2].name()),
                   juce::Rectangle<int>(0, h, w, h), juce::Justification::centred);
        // BR quadrant (index 3)
        g.drawText(juce::String(subChords[3].name()),
                   juce::Rectangle<int>(w, h, w, h), juce::Justification::centred);
    }
    else if (romanNumeral_.empty())
    {
        g.setColour(juce::Colour(0xffe0e0e0));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(juce::String(chord.name()), textArea,
                   juce::Justification::centred);
    }
    else
    {
        auto topHalf = textArea.removeFromTop(textArea.getHeight() / 2);
        auto bottomHalf = textArea;

        g.setColour(juce::Colour(0xffe0e0e0));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(juce::String(chord.name()), topHalf,
                   juce::Justification::centredBottom);

        g.setColour(juce::Colour(0xffaaaaaa));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText(juce::String(romanNumeral_), bottomHalf,
                   juce::Justification::centredTop);
    }
}

void PadComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        bool shiftHeld = event.mods.isShiftDown();
        pendingOctaveOffset_ = shiftHeld ? -1 : +1;

        const Chord& baseChord = (hasSubVariations && pressedQuadrant >= 0)
            ? subChords[static_cast<size_t>(pressedQuadrant)]
            : chord;

        Chord offsetChord = baseChord;
        offsetChord.octaveOffset = pendingOctaveOffset_;
        offsetChord.romanNumeral = romanNumeral_;   // carry current label

        isPressed = true;
        isDragInProgress = false;
        repaint();

        if (onPressStart) onPressStart(offsetChord);
        return;
    }

    isPressed = true;
    isDragInProgress = false;
    pressedQuadrant = quadrantAt(event.getPosition());
    repaint();

    const Chord& activeChord = (hasSubVariations && pressedQuadrant >= 0)
        ? subChords[static_cast<size_t>(pressedQuadrant)]
        : chord;
    if (onPressStart)
        onPressStart(activeChord);
}

void PadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 6)
        return;

    if (isDragInProgress)
        return;

    isDragInProgress = true;

    const Chord& activeChord = (hasSubVariations && pressedQuadrant >= 0)
        ? subChords[static_cast<size_t>(pressedQuadrant)]
        : chord;

    if (onPressEnd)
        onPressEnd(activeChord);

    dragChord_ = activeChord;
    dragChord_.octaveOffset = pendingOctaveOffset_;  // 0 for left-drag, ±1 for right-drag
    dragChord_.romanNumeral = romanNumeral_;          // carry pad's current Roman numeral label

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        container->startDragging(juce::var(juce::String(activeChord.name())), this, juce::ScaledImage{}, false);
}

void PadComponent::mouseUp(const juce::MouseEvent& event)
{
    isPressed = false;
    repaint();

    const Chord& baseChord = (hasSubVariations && pressedQuadrant >= 0)
        ? subChords[static_cast<size_t>(pressedQuadrant)]
        : chord;

    if (!isDragInProgress)
    {
        // Build release chord — if right-click pending, use its offset
        Chord releaseChord = baseChord;
        if (pendingOctaveOffset_ != 0)
            releaseChord.octaveOffset = pendingOctaveOffset_;

        if (onPressEnd) onPressEnd(releaseChord);

        // onClick only fires for left-click (pendingOctaveOffset_ == 0 for left-click)
        if (pendingOctaveOffset_ == 0 && event.getDistanceFromDragStart() < 6 && onClick)
            onClick(releaseChord);
    }

    isDragInProgress = false;
    pressedQuadrant = -1;
    pendingOctaveOffset_ = 0;
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
    pressedQuadrant = -1;
    repaint();
}

} // namespace chordpumper
