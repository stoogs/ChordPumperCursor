#include "midi/MidiFileBuilder.h"

namespace chordpumper {

void MidiFileBuilder::buildSequence(juce::MidiMessageSequence& seq,
                                     const Chord& chord, int octave,
                                     float velocity) {
    seq.addEvent(juce::MidiMessage::tempoMetaEvent(kTempoMicrosecondsPerBeat), 0.0);

    for (int note : chord.midiNotes(octave)) {
        seq.addEvent(juce::MidiMessage::noteOn(kChannel, note, velocity), 0.0);
        seq.addEvent(juce::MidiMessage::noteOff(kChannel, note, 0.0f),
                     static_cast<double>(kBarLengthTicks));
    }

    seq.updateMatchedPairs();
}

bool MidiFileBuilder::writeToFile(const juce::MidiMessageSequence& seq,
                                   const juce::File& file) {
    juce::MidiFile midi;
    midi.setTicksPerQuarterNote(kTicksPerQuarterNote);
    midi.addTrack(seq);

    file.deleteFile();
    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    bool ok = midi.writeTo(*stream);
    stream->flush();
    stream.reset();
    return ok;
}

juce::File MidiFileBuilder::createMidiFile(const Chord& chord, int octave,
                                            float velocity) {
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto file = tempDir.getChildFile(
        "chordpumper_" + juce::String::toHexString(
            juce::Random::getSystemRandom().nextInt64()) + ".mid");

    juce::MidiMessageSequence seq;
    buildSequence(seq, chord, octave, velocity);

    if (!writeToFile(seq, file))
        return {};

    return file;
}

bool MidiFileBuilder::exportProgression(const std::vector<Chord>& chords,
                                        int octave, const juce::File& file,
                                        float velocity) {
    if (chords.empty())
        return false;

    juce::MidiMessageSequence seq;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent(kTempoMicrosecondsPerBeat), 0.0);

    for (size_t i = 0; i < chords.size(); ++i)
    {
        double startTick = static_cast<double>(i * kBarLengthTicks);
        double endTick = startTick + static_cast<double>(kBarLengthTicks);

        for (int note : chords[i].midiNotes(octave))
        {
            seq.addEvent(juce::MidiMessage::noteOn(kChannel, note, velocity), startTick);
            seq.addEvent(juce::MidiMessage::noteOff(kChannel, note, 0.0f), endTick);
        }
    }

    seq.updateMatchedPairs();
    return writeToFile(seq, file);
}

juce::File MidiFileBuilder::exportToDirectory(const Chord& chord, int octave,
                                               const juce::File& directory,
                                               float velocity) {
    directory.createDirectory();

    auto file = directory.getChildFile(
        juce::String(chord.name()) + ".mid");

    juce::MidiMessageSequence seq;
    buildSequence(seq, chord, octave, velocity);

    if (!writeToFile(seq, file))
        return {};

    return file;
}

} // namespace chordpumper
