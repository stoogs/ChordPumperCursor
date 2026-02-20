#pragma once

#include "PersistentState.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace chordpumper {

class ChordPumperProcessor : public juce::AudioProcessor,
                              public juce::ChangeBroadcaster
{
public:
    ChordPumperProcessor();
    ~ChordPumperProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    using AudioProcessor::processBlock;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    PersistentState& getState() { return persistentState; }
    const PersistentState& getState() const { return persistentState; }
    juce::CriticalSection& getStateLock() { return stateLock; }

private:
    juce::MidiKeyboardState keyboardState;
    PersistentState persistentState;
    juce::CriticalSection stateLock;
};

} // namespace chordpumper
