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
    juce::String desc = details.description.toString();
    if (desc.startsWith("REORDER:"))
        return true;
    return dynamic_cast<PadComponent*>(details.sourceComponent.get()) != nullptr;
}

void ProgressionStrip::itemDropped(const SourceDetails& details)
{
    juce::String desc = details.description.toString();

    if (desc.startsWith("REORDER:"))
    {
        int fromIdx = desc.fromFirstOccurrenceOf(":", false, false).getIntValue();
        int toIdx   = insertionIndex;  // gap index (0..N) set by itemDragMove

        if (overwriteIndex >= 0 && overwriteIndex != fromIdx &&
            overwriteIndex < static_cast<int>(chords.size()))
        {
            // Swap the two slots
            std::swap(chords[static_cast<size_t>(fromIdx)],
                      chords[static_cast<size_t>(overwriteIndex)]);
            {
                const juce::ScopedLock sl(stateLock);
                persistentState.progression = chords;
            }
            updateClearButton();
            updateExportButton();
        }
        else if (fromIdx >= 0 && fromIdx < static_cast<int>(chords.size()) &&
                 toIdx   >= 0 && toIdx   <= static_cast<int>(chords.size()) &&
                 toIdx != fromIdx && toIdx != fromIdx + 1)
        {
            auto chord = chords[static_cast<size_t>(fromIdx)];
            chords.erase(chords.begin() + fromIdx);
            // After erase, if toIdx was after fromIdx, it shifts back by 1
            if (toIdx > fromIdx)
                toIdx--;
            chords.insert(chords.begin() + toIdx, chord);

            {
                const juce::ScopedLock sl(stateLock);
                persistentState.progression = chords;
            }
            updateClearButton();
            updateExportButton();
        }

        reorderDragFromIndex = -1;
        insertionIndex = -1;
        overwriteIndex = -1;
        isReceivingDrag = false;
        repaint();
        return;
    }

    // Pad-drop logic with overwrite/insert support
    if (auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get()))
    {
        const auto& chord = pad->getDragChord();
        if (overwriteIndex >= 0 && overwriteIndex < static_cast<int>(chords.size()))
        {
            // Overwrite in place
            chords[static_cast<size_t>(overwriteIndex)] = chord;
            {
                const juce::ScopedLock sl(stateLock);
                persistentState.progression = chords;
            }
            updateClearButton();
            updateExportButton();
            if (onChordDropped)
                onChordDropped(chord);
        }
        else if (insertionIndex >= 0 && insertionIndex <= static_cast<int>(chords.size()))
        {
            // Insert at insertionIndex
            if (chords.size() >= static_cast<size_t>(kMaxChords))
                chords.erase(chords.begin());
            int idx = juce::jlimit(0, static_cast<int>(chords.size()), insertionIndex);
            chords.insert(chords.begin() + idx, chord);
            {
                const juce::ScopedLock sl(stateLock);
                persistentState.progression = chords;
            }
            updateClearButton();
            updateExportButton();
            if (onChordDropped)
                onChordDropped(chord);
        }
        else
        {
            addChord(chord);
            if (onChordDropped)
                onChordDropped(chord);
        }
    }
    insertionIndex = -1;
    overwriteIndex = -1;
    isReceivingDrag = false;
    repaint();
}

void ProgressionStrip::itemDragEnter(const SourceDetails&)
{
    isReceivingDrag = true;
    repaint();
}

void ProgressionStrip::itemDragExit(const SourceDetails& details)
{
    juce::String desc = details.description.toString();
    if (desc.startsWith("REORDER:"))
        reorderDragFromIndex = -1;
    insertionIndex = -1;
    overwriteIndex = -1;
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

        // Build a drag image showing only the dragged slot, not the whole strip
        auto area = getLocalBounds();
        auto slotArea = area.removeFromLeft(area.getWidth() - 120);
        auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
        auto slotHeight = slotArea.getHeight();

        juce::Image dragImg(juce::Image::ARGB, slotWidth, slotHeight, true);
        {
            juce::Graphics imgG(dragImg);
            auto slotF = juce::Rectangle<float>(0.0f, 0.0f, (float)slotWidth, (float)slotHeight);
            auto grad = juce::ColourGradient::vertical(
                juce::Colour(PadColours::background).brighter(0.03f),
                juce::Colour(PadColours::background).darker(0.03f), slotF);
            imgG.setGradientFill(grad);
            imgG.fillRoundedRectangle(slotF, 4.0f);
            imgG.setColour(juce::Colour(PadColours::accentForType(chords[static_cast<size_t>(index)].type)).withAlpha(0.7f));
            imgG.drawRoundedRectangle(slotF.reduced(0.5f), 4.0f, 1.5f);
            imgG.setColour(juce::Colour(0xffe0e0e0));
            imgG.setFont(juce::Font(juce::FontOptions(13.0f)));
            imgG.drawText(juce::String(chords[static_cast<size_t>(index)].name()),
                          juce::Rectangle<int>(0, 0, slotWidth, slotHeight),
                          juce::Justification::centred);
        }

        container->startDragging(juce::var(desc), this, juce::ScaledImage(dragImg), false);
    }
}

void ProgressionStrip::itemDragMove(const SourceDetails& details)
{
    auto localPos = getLocalPoint(details.sourceComponent.get(), details.localPosition);
    int xPos = localPos.getX();

    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int relX = xPos - slotArea.getX();
    int cellWidth = slotWidth + 4;

    // Determine which cell (0..kMaxChords-1) the cursor is in
    int cell = relX / cellWidth;
    int posInCell = relX % cellWidth;
    bool inSlotRegion = (posInCell < slotWidth) && (relX >= 0) && (relX < slotArea.getWidth());
    bool isExistingSlot = inSlotRegion && (cell >= 0) && (cell < static_cast<int>(chords.size()));

    // For REORDER drags, don't overwrite the slot being dragged
    juce::String desc = details.description.toString();
    int reorderFrom = -1;
    if (desc.startsWith("REORDER:"))
        reorderFrom = desc.fromFirstOccurrenceOf(":", false, false).getIntValue();

    if (isExistingSlot && cell != reorderFrom)
    {
        overwriteIndex = cell;
        insertionIndex = -1;
    }
    else
    {
        overwriteIndex = -1;
        insertionIndex = insertionIndexAtX(xPos);
    }

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
    pressedIndex = index;
    if (index >= 0 && onPressStart)
        onPressStart(chords[static_cast<size_t>(index)]);
}

void ProgressionStrip::mouseUp(const juce::MouseEvent& event)
{
    int index = pressedIndex;
    pressedIndex = -1;

    if (index < 0 || index >= static_cast<int>(chords.size()))
        return;

    if (onPressEnd)
        onPressEnd(chords[static_cast<size_t>(index)]);

    // Only trigger morph on a click (no significant drag)
    if (event.getDistanceFromDragStart() < 10 && onChordClicked)
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

            if (i == overwriteIndex)
            {
                g.setColour(juce::Colour(0xffffff00).withAlpha(0.9f)); // bright yellow
                g.drawRoundedRectangle(slotF.reduced(0.5f), 4.0f, 2.5f);
            }

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

    if (insertionIndex >= 0 && insertionIndex <= static_cast<int>(chords.size()))
    {
        auto area2 = getLocalBounds();
        auto slotArea2 = area2.removeFromLeft(area2.getWidth() - 120);
        auto slotWidth2 = (slotArea2.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
        int cellWidth2 = slotWidth2 + 4;
        int cursorX = slotArea2.getX() + insertionIndex * cellWidth2 - 2;
        g.setColour(juce::Colours::white);
        g.fillRect(cursorX, slotArea2.getY() + 2, 4, slotArea2.getHeight() - 4);
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
