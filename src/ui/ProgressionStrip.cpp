#include "ProgressionStrip.h"
#include "ChordPumperLookAndFeel.h"

namespace chordpumper {

ProgressionStrip::ProgressionStrip(PersistentState& state, juce::CriticalSection& lock)
    : persistentState(state), stateLock(lock)
{
    {
        const juce::ScopedLock sl(stateLock);
        chords = persistentState.progression;
    }

    addAndMakeVisible(clearButton);
    updateClearButton();
    clearButton.onClick = [this]
    {
        clear();
        repaint();
    };
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
    repaint();
}

void ProgressionStrip::refreshFromState()
{
    {
        const juce::ScopedLock sl(stateLock);
        chords = persistentState.progression;
    }
    updateClearButton();
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

void ProgressionStrip::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 60);

    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    auto font = juce::Font(juce::FontOptions(13.0f));
    g.setFont(font);

    for (int i = 0; i < kMaxChords; ++i)
    {
        auto x = slotArea.getX() + i * (slotWidth + 4);
        auto slot = juce::Rectangle<int>(x, slotArea.getY(), slotWidth, slotArea.getHeight());

        if (static_cast<size_t>(i) < chords.size())
        {
            g.setColour(juce::Colour(PadColours::background));
            g.fillRoundedRectangle(slot.toFloat(), 4.0f);
            g.setColour(juce::Colours::white);
            g.drawText(chords[static_cast<size_t>(i)].name(), slot,
                       juce::Justification::centred);
        }
        else
        {
            g.setColour(juce::Colour(0xff3a3a4a));
            g.drawRoundedRectangle(slot.toFloat().reduced(0.5f), 4.0f, 1.0f);
        }
    }
}

void ProgressionStrip::resized()
{
    auto area = getLocalBounds();
    clearButton.setBounds(area.removeFromRight(56).reduced(0, 4));
}

void ProgressionStrip::updateClearButton()
{
    clearButton.setEnabled(!chords.empty());
}

} // namespace chordpumper
