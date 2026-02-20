#include "ProgressionStrip.h"
#include "PadComponent.h"
#include "ChordPumperLookAndFeel.h"
#include "midi/MidiFileBuilder.h"

namespace chordpumper {

ProgressionStrip::ProgressionStrip(PersistentState& state, juce::CriticalSection& lock)
    : persistentState(state), stateLock(lock)
{
    {
        const juce::ScopedLock sl(stateLock);
        chords = persistentState.progression;
    }

    addAndMakeVisible(clearButton);
    addAndMakeVisible(exportButton);
    updateClearButton();
    updateExportButton();
    clearButton.onClick = [this]
    {
        clear();
        repaint();
    };
    exportButton.onClick = [this] { exportProgression(); };
}

void ProgressionStrip::addChord(const Chord& chord)
{
    if (chords.size() >= static_cast<size_t>(kMaxChords))
        chords.erase(chords.begin());

    chords.push_back(chord);

    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression = chords;
    }

    updateClearButton();
    updateExportButton();
    repaint();
}

void ProgressionStrip::setChords(const std::vector<Chord>& newChords)
{
    chords = newChords;

    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression = chords;
    }

    updateClearButton();
    updateExportButton();
    repaint();
}

void ProgressionStrip::clear()
{
    chords.clear();

    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression.clear();
    }

    updateClearButton();
    updateExportButton();
    repaint();
}

void ProgressionStrip::refreshFromState()
{
    {
        const juce::ScopedLock sl(stateLock);
        chords = persistentState.progression;
    }
    updateClearButton();
    updateExportButton();
    repaint();
}

const std::vector<Chord>& ProgressionStrip::getChords() const
{
    return chords;
}

bool ProgressionStrip::isEmpty() const
{
    return chords.empty();
}

bool ProgressionStrip::isInterestedInDragSource(const SourceDetails& details)
{
    return dynamic_cast<PadComponent*>(details.sourceComponent.get()) != nullptr;
}

void ProgressionStrip::itemDropped(const SourceDetails& details)
{
    if (auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get()))
    {
        const auto& chord = pad->getChord();
        addChord(chord);
        if (onChordDropped)
            onChordDropped(chord);
    }
    isReceivingDrag = false;
    repaint();
}

void ProgressionStrip::itemDragEnter(const SourceDetails&)
{
    isReceivingDrag = true;
    repaint();
}

void ProgressionStrip::itemDragExit(const SourceDetails&)
{
    isReceivingDrag = false;
    repaint();
}

int ProgressionStrip::getChordIndexAtPosition(juce::Point<int> pos) const
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);

    if (!slotArea.contains(pos))
        return -1;

    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int relX = pos.getX() - slotArea.getX();
    int cellWidth = slotWidth + 4;
    int index = relX / cellWidth;
    int posInCell = relX % cellWidth;

    if (posInCell >= slotWidth)
        return -1;
    if (index < 0 || index >= static_cast<int>(chords.size()))
        return -1;

    return index;
}

int ProgressionStrip::insertionIndexAtX(int xPos) const
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int relX = xPos - slotArea.getX();
    // Each cell = slotWidth + 4 gap. Map relX to nearest gap boundary.
    int cellWidth = slotWidth + 4;
    // Clamp relX to slot area
    relX = juce::jlimit(0, slotArea.getWidth(), relX);
    // Number of complete cells before this position
    int cell = relX / cellWidth;
    int posInCell = relX % cellWidth;
    // If we're in the second half of a cell, insertion is after it
    int insertion = (posInCell > cellWidth / 2) ? cell + 1 : cell;
    return juce::jlimit(0, static_cast<int>(chords.size()), insertion);
}

void ProgressionStrip::mouseDrag(const juce::MouseEvent& event)
{
    // 10px threshold — wider than pad's 6px to avoid accidental reorder on click
    if (event.getDistanceFromDragStart() < 10)
        return;

    if (reorderDragFromIndex >= 0)
        return;  // already dragging

    int index = getChordIndexAtPosition(event.getMouseDownPosition());
    if (index < 0)
        return;

    reorderDragFromIndex = index;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::String desc = "REORDER:" + juce::String(index);
        container->startDragging(juce::var(desc), this, juce::ScaledImage{}, false);
    }
}

void ProgressionStrip::itemDragMove(const SourceDetails& details)
{
    juce::String desc = details.description.toString();
    if (!desc.startsWith("REORDER:"))
        return;

    auto localPos = getLocalPoint(details.sourceComponent.get(), details.localPosition);
    insertionIndex = insertionIndexAtX(localPos.getX());
    repaint();
}

void ProgressionStrip::mouseDown(const juce::MouseEvent& event)
{
    if (isReceivingDrag)
        return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (container->isDragAndDropActive())
            return;
    }

    int index = getChordIndexAtPosition(event.getPosition());
    if (index >= 0 && onChordClicked)
        onChordClicked(chords[static_cast<size_t>(index)]);
}

void ProgressionStrip::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);

    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    auto font = juce::Font(juce::FontOptions(13.0f));
    g.setFont(font);

    for (int i = 0; i < kMaxChords; ++i)
    {
        auto x = slotArea.getX() + i * (slotWidth + 4);
        auto slot = juce::Rectangle<int>(x, slotArea.getY(), slotWidth, slotArea.getHeight());

        if (static_cast<size_t>(i) < chords.size())
        {
            auto slotF = slot.toFloat();
            auto grad = juce::ColourGradient::vertical(
                juce::Colour(PadColours::background).brighter(0.03f),
                juce::Colour(PadColours::background).darker(0.03f), slotF);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(slotF, 4.0f);

            g.setColour(juce::Colour(PadColours::accentForType(chords[static_cast<size_t>(i)].type)).withAlpha(0.5f));
            g.drawRoundedRectangle(slotF, 4.0f, 1.5f);

            g.setColour(juce::Colour(0xffe0e0e0));
            g.drawText(chords[static_cast<size_t>(i)].name(), slot,
                       juce::Justification::centred);
        }
        else
        {
            g.setColour(juce::Colour(0xff3a3a4a));
            g.drawRoundedRectangle(slot.toFloat().reduced(0.5f), 4.0f, 1.0f);
        }
    }

    if (isReceivingDrag)
    {
        g.setColour(juce::Colour(0xff6c8ebf).withAlpha(0.15f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff6c8ebf).withAlpha(0.5f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 2.0f);
    }
}

void ProgressionStrip::resized()
{
    auto area = getLocalBounds();
    clearButton.setBounds(area.removeFromRight(56).reduced(0, 4));
    area.removeFromRight(4);
    exportButton.setBounds(area.removeFromRight(56).reduced(0, 4));
}

void ProgressionStrip::updateClearButton()
{
    clearButton.setEnabled(!chords.empty());
}

void ProgressionStrip::updateExportButton()
{
    exportButton.setEnabled(!chords.empty());
}

void ProgressionStrip::exportProgression()
{
    if (chords.empty())
        return;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Progression as MIDI",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory)
            .getChildFile("ChordPumper-Progression.mid"),
        "*.mid", true, false, this);

    constexpr int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        auto file = chooser.getResult();
        if (file == juce::File())
            return;
        MidiFileBuilder::exportProgression(chords, 4, file);
    });
}

} // namespace chordpumper
