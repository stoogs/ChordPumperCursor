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
}

void ChordPumperProcessor::releaseResources()
{
}

void ChordPumperProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    buffer.clear();
}

juce::AudioProcessorEditor* ChordPumperProcessor::createEditor()
{
    return new ChordPumperEditor(*this);
}

void ChordPumperProcessor::getStateInformation(juce::MemoryBlock& /*destData*/)
{
}

void ChordPumperProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/)
{
}

} // namespace chordpumper

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new chordpumper::ChordPumperProcessor();
}
