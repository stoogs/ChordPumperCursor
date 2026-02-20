#pragma once

#include "engine/Chord.h"
#include "engine/ChordType.h"
#include <array>
#include <string>

namespace chordpumper {

struct RomanNumeralInfo {
    int interval;
    const char* upperCase;
    const char* lowerCase;
};

inline constexpr std::array<RomanNumeralInfo, 12> kRomanNumerals = {{
    { 0, "I",       "i"},
    { 1, "\u266dII",  "\u266dii"},
    { 2, "II",      "ii"},
    { 3, "\u266dIII", "\u266diii"},
    { 4, "III",     "iii"},
    { 5, "IV",      "iv"},
    { 6, "\u266fIV",  "\u266fiv"},   // tritone default; romanNumeral() handles ♭V/♭v
    { 7, "V",       "v"},
    { 8, "\u266dVI",  "\u266dvi"},
    { 9, "VI",      "vi"},
    {10, "\u266dVII", "\u266dvii"},
    {11, "VII",     "vii"},
}};

inline bool isUpperCase(ChordType type) {
    switch (type) {
        case ChordType::Major:
        case ChordType::Augmented:
        case ChordType::Maj7:
        case ChordType::Dom7:
        case ChordType::Maj9:
        case ChordType::Maj11:
        case ChordType::Maj13:
        case ChordType::Dom9:
        case ChordType::Dom11:
        case ChordType::Dom13:
            return true;
        default:
            return false;
    }
}

std::string romanNumeral(const Chord& reference, const Chord& suggestion);

} // namespace chordpumper
