#include "PadComponent.h"
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

    if (isPressed)
        g.setColour(juce::Colour(0xff6c8ebf));
    else if (isHovered)
        g.setColour(juce::Colour(0xff353545));
    else
        g.setColour(juce::Colour(0xff2a2a3a));

    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

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
    repaint();
    if (onClick)
        onClick(chord);
}

void PadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 6)
        return;

    if (isDragInProgress)
        return;

    auto midiFile = MidiFileBuilder::createMidiFile(chord, 4);
    if (!midiFile.existsAsFile())
        return;

    isDragInProgress = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        { midiFile.getFullPathName() },
        false,
        this,
        [this, midiFile]() {
            isDragInProgress = false;
            juce::Timer::callAfterDelay(2000, [midiFile]() {
                midiFile.deleteFile();
            });
        }
    );
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
