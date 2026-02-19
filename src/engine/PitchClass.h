#pragma once

#include "engine/NoteLetter.h"
#include <cstdint>
#include <string>

namespace chordpumper {

struct PitchClass {
    NoteLetter letter;
    int8_t accidental;

    constexpr int semitone() const { return 0; }
    std::string name() const;
    int midiNote(int octave) const;

    constexpr bool operator==(const PitchClass&) const = default;
};

namespace pitches {
    inline constexpr PitchClass C  {NoteLetter::C,  0};
    inline constexpr PitchClass Cs {NoteLetter::C,  1};
    inline constexpr PitchClass D  {NoteLetter::D,  0};
    inline constexpr PitchClass Eb {NoteLetter::E, -1};
    inline constexpr PitchClass E  {NoteLetter::E,  0};
    inline constexpr PitchClass F  {NoteLetter::F,  0};
    inline constexpr PitchClass Fs {NoteLetter::F,  1};
    inline constexpr PitchClass G  {NoteLetter::G,  0};
    inline constexpr PitchClass Ab {NoteLetter::A, -1};
    inline constexpr PitchClass A  {NoteLetter::A,  0};
    inline constexpr PitchClass Bb {NoteLetter::B, -1};
    inline constexpr PitchClass B  {NoteLetter::B,  0};
} // namespace pitches

} // namespace chordpumper
