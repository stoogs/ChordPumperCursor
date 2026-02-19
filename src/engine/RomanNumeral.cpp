#include "engine/RomanNumeral.h"

namespace chordpumper {

std::string romanNumeral(const Chord& reference, const Chord& suggestion) {
    int interval = (suggestion.root.semitone() - reference.root.semitone() + 12) % 12;
    bool upper = isUpperCase(suggestion.type);

    // Tritone ambiguity: ♯IV for major/augmented, ♭V for minor/diminished
    if (interval == 6) {
        if (upper) {
            return suggestion.type == ChordType::Augmented
                ? std::string("\u266fIV+")
                : std::string("\u266fIV");
        }
        bool isDim = (suggestion.type == ChordType::Diminished ||
                      suggestion.type == ChordType::Dim7 ||
                      suggestion.type == ChordType::HalfDim7);
        if (isDim)
            return std::string("\u266dv\u00b0");
        return std::string("\u266dv");
    }

    std::string base = upper ? kRomanNumerals[static_cast<size_t>(interval)].upperCase
                             : kRomanNumerals[static_cast<size_t>(interval)].lowerCase;

    // Quality suffixes for 7th chords
    int typeIdx = static_cast<int>(suggestion.type);
    if (typeIdx >= 4) { // 7th chord types start at index 4
        switch (suggestion.type) {
            case ChordType::Maj7:    base += "\u0394"; break;    // Δ
            case ChordType::Min7:    base += "7"; break;
            case ChordType::Dom7:    base += "7"; break;
            case ChordType::Dim7:    base += "\u00b07"; break;   // °7
            case ChordType::HalfDim7: base += "\u00f87"; break;  // ø7
            default: break;
        }
    }

    // Quality suffixes for triads
    if (suggestion.type == ChordType::Augmented)
        base += "+";
    if (suggestion.type == ChordType::Diminished)
        base += "\u00b0"; // °

    return base;
}

} // namespace chordpumper
