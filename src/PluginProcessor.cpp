#include "PluginProcessor.h"
#include "ui/PluginEditor.h"

namespace chordpumper {

ChordPumperProcessor::ChordPumperProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ChordPumperProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    keyboardState.reset();
}

void ChordPumperProcessor::releaseResources()
{
}

void ChordPumperProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    midiMessages.ensureSize(2048);
    buffer.clear();
    midiMessages.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
}

juce::AudioProcessorEditor* ChordPumperProcessor::createEditor()
{
    return new ChordPumperEditor(*this);
}

void ChordPumperProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state;
    {
        const juce::ScopedLock sl(stateLock);
        state = persistentState.toValueTree();
    }
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void ChordPumperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr) return;

    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.isValid()) return;

    auto restored = PersistentState::fromValueTree(tree);
    {
        const juce::ScopedLock sl(stateLock);
        persistentState = std::move(restored);
    }
    sendChangeMessage();
}

} // namespace chordpumper

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new chordpumper::ChordPumperProcessor();
}
