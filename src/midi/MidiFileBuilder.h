#pragma once

#include "engine/Chord.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace chordpumper {

class MidiFileBuilder {
public:
    static juce::File createMidiFile(const Chord& chord, int octave,
                                      float velocity = 0.8f);
    static juce::File exportToDirectory(const Chord& chord, int octave,
                                         const juce::File& directory,
                                         float velocity = 0.8f);

private:
    static void buildSequence(juce::MidiMessageSequence& seq,
                               const Chord& chord, int octave, float velocity);
    static bool writeToFile(const juce::MidiMessageSequence& seq,
                             const juce::File& file);

    static constexpr int kTicksPerQuarterNote = 480;
    static constexpr int kBarLengthTicks = 1920;
    static constexpr int kChannel = 1;
    static constexpr int kTempoMicrosecondsPerBeat = 500000; // 120 BPM
};

} // namespace chordpumper
