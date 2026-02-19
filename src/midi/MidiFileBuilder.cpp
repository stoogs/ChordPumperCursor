#include "midi/MidiFileBuilder.h"

namespace chordpumper {

juce::File MidiFileBuilder::createMidiFile(const Chord& /*chord*/, int /*octave*/,
                                            float /*velocity*/) {
    return {};
}

juce::File MidiFileBuilder::exportToDirectory(const Chord& /*chord*/, int /*octave*/,
                                               const juce::File& /*directory*/,
                                               float /*velocity*/) {
    return {};
}

void MidiFileBuilder::buildSequence(juce::MidiMessageSequence& /*seq*/,
                                     const Chord& /*chord*/, int /*octave*/,
                                     float /*velocity*/) {}

bool MidiFileBuilder::writeToFile(const juce::MidiMessageSequence& /*seq*/,
                                   const juce::File& /*file*/) {
    return false;
}

} // namespace chordpumper
