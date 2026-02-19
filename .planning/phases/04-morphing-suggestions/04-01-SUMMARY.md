---
phase: 04-morphing-suggestions
plan: 01
subsystem: engine
tags: [pitch-class-set, voice-leading, roman-numeral, music-theory, constexpr, catch2]

requires:
  - phase: 02-chord-engine
    provides: "Chord, PitchClass, ChordType structs; kIntervals table; ChordPumperEngine static lib"
provides:
  - "PitchClassSet: 12-bit bitmask chord representation with O(1) common-tone counting"
  - "ScaleDatabase: constexpr mode patterns for all 7 diatonic modes"
  - "VoiceLeader: brute-force permutation voice leading distance + optimal voicing"
  - "RomanNumeral: interval-to-Roman-numeral lookup with quality suffixes"
  - "kAllChords: compile-time catalog of all 108 candidate chords"
affects: [04-02-PLAN (MorphEngine scoring uses all four classes), 04-03-PLAN (integration wiring)]

tech-stack:
  added: []
  patterns: [header-only constexpr lookup tables, bitwise pitch-class-set operations, brute-force permutation search]

key-files:
  created:
    - src/engine/PitchClassSet.h
    - src/engine/ScaleDatabase.h
    - src/engine/VoiceLeader.h
    - src/engine/VoiceLeader.cpp
    - src/engine/RomanNumeral.h
    - src/engine/RomanNumeral.cpp
    - tests/test_pitch_class_set.cpp
    - tests/test_voice_leader.cpp
    - tests/test_roman_numeral.cpp
  modified:
    - CMakeLists.txt

key-decisions:
  - "Tritone disambiguation: ♯IV for major/augmented qualities, ♭v for minor/diminished at interval 6"
  - "VoiceLeader uses centroid-based placement with ±12 octave search (3^n combinations) for optimal voicing"
  - "Roman numeral suffixes: Δ for maj7, °7 for dim7, ø7 for half-dim7, 7 for dom7/min7, + for augmented, ° for diminished"

patterns-established:
  - "Header-only constexpr for lookup tables (PitchClassSet, ScaleDatabase) — zero runtime cost"
  - "Bitwise AND + popcount for O(1) common-tone counting between any two chords"
  - "Brute-force permutation search for voice leading (max 24 iterations for 4-note chords)"

requirements-completed: [CHRD-03, GRID-04, GRID-06]

duration: 17min
completed: 2026-02-19
---

# Phase 4 Plan 01: Foundation Engine Classes Summary

**Bitset pitch-class-set operations, brute-force voice leading optimizer, constexpr scale database, and Roman numeral generator — all zero-JUCE-dependency engine primitives with 40 passing Catch2 tests**

## Performance

- **Duration:** 17 min
- **Started:** 2026-02-19T20:30:55Z
- **Completed:** 2026-02-19T20:47:29Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- PitchClassSet with O(1) common-tone counting via bitwise AND + popcount, plus compile-time kAllChords catalog (108 entries)
- ScaleDatabase with constexpr mode patterns for all 7 diatonic modes including triad and seventh chord qualities per degree
- VoiceLeader with brute-force permutation distance calculation and centroid-based optimal voicing with octave search
- RomanNumeral with full 12-interval lookup, tritone disambiguation, and quality suffixes for all chord types
- 40 total tests passing (17 new + 23 existing), covering bitmask construction, common tones, voice leading distance, optimal voicing, transposition invariance, all Roman numeral intervals, and edge cases

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement engine foundation classes** - `24b75ac` (feat)
2. **Task 2: Add Catch2 tests and update CMakeLists.txt** - `cb259f2` (test)

## Files Created/Modified
- `src/engine/PitchClassSet.h` — 12-bit bitmask pitch-class-set type, pitchClassSet(), commonToneCount(), kAllChords catalog
- `src/engine/ScaleDatabase.h` — Constexpr kModePatterns for all 7 diatonic modes with triad/seventh qualities
- `src/engine/VoiceLeader.h` — VoicedChord struct, voiceLeadingDistance(), optimalVoicing() declarations
- `src/engine/VoiceLeader.cpp` — Brute-force permutation voice leading + centroid-based optimal voicing with ±octave search
- `src/engine/RomanNumeral.h` — kRomanNumerals lookup table, isUpperCase(), romanNumeral() declaration
- `src/engine/RomanNumeral.cpp` — Roman numeral generation with tritone disambiguation and quality suffixes
- `tests/test_pitch_class_set.cpp` — Bitmask construction, common tones, catalog validation, enharmonic equivalence
- `tests/test_voice_leader.cpp` — Distance calculation, permutation optimality, size mismatch, transposition invariance
- `tests/test_roman_numeral.cpp` — All 12 intervals, case sensitivity, 7th suffixes, tritone ambiguity
- `CMakeLists.txt` — Added VoiceLeader.cpp + RomanNumeral.cpp to engine lib, 3 test files to test executable

## Decisions Made
- Tritone disambiguation: ♯IV for major/augmented qualities, ♭v for minor/diminished at interval 6 — follows research recommendation
- VoiceLeader optimal voicing uses centroid-based initial placement with exhaustive ±12 octave search (3^n combinations for n notes) — guarantees global optimum within one octave of centroid
- Roman numeral quality suffixes use music theory standard notation: Δ (maj7), °7 (dim7), ø7 (half-dim7), + (augmented), ° (diminished)

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness
- All four foundation classes are ready for MorphEngine (Plan 02) to use for hybrid scoring
- PitchClassSet provides the common-tone dimension, VoiceLeader provides the voice-leading dimension, ScaleDatabase provides the diatonic dimension, RomanNumeral provides the labeling
- kAllChords catalog gives MorphEngine its 108 candidates to score and rank

---
*Phase: 04-morphing-suggestions*
*Completed: 2026-02-19*
