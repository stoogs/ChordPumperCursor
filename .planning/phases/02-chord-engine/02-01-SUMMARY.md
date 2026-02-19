---
phase: 02-chord-engine
plan: 01
subsystem: engine
tags: [c++20, cmake, catch2, music-theory, static-library]

requires:
  - phase: 01-plugin-foundation
    provides: CMake build system, JUCE plugin targets, glibc compat layer
provides:
  - ChordPumperEngine static library (JUCE-independent)
  - NoteLetter enum with semitone/name lookup tables
  - PitchClass struct with 12 canonical pitch constants
  - ChordType enum with interval table, suffix table, noteCount()
  - Chord struct interface (root + type, stub methods)
  - Catch2 v3.13.0 test infrastructure via FetchContent
  - ChordPumperTests executable with CTest discovery
affects: [02-chord-engine, 03-playable-grid, 04-morphing-suggestions]

tech-stack:
  added: [Catch2 v3.13.0]
  patterns: [JUCE-independent engine library, constexpr lookup tables, FetchContent for test deps]

key-files:
  created:
    - src/engine/NoteLetter.h
    - src/engine/PitchClass.h
    - src/engine/PitchClass.cpp
    - src/engine/ChordType.h
    - src/engine/Chord.h
    - src/engine/Chord.cpp
    - tests/test_pitch_class.cpp
    - tests/test_chord.cpp
    - tests/test_chord_naming.cpp
  modified:
    - CMakeLists.txt

key-decisions:
  - "Engine as static library decoupled from JUCE — enables fast test compilation"
  - "Catch2 v3.13.0 via FetchContent with GIT_SHALLOW — no submodule overhead"
  - "Test build guarded by CHORDPUMPER_BUILD_TESTS option"

patterns-established:
  - "JUCE-independent engine library: pure C++20 static lib, plugin and tests both link against it"
  - "Constexpr lookup tables for music theory data (intervals, suffixes, semitones)"
  - "PitchClass as letter+accidental struct with canonical 12-pitch constant set"
  - "Catch2 smoke tests as build pipeline validation"

requirements-completed: []

duration: 7min
completed: 2026-02-19
---

# Phase 2 Plan 01: Engine Library Scaffold Summary

**ChordPumperEngine static library with complete C++20 type definitions (enums, constexpr tables, struct interfaces) and Catch2 v3.13.0 test infrastructure with 3 passing smoke tests**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-19T18:23:31Z
- **Completed:** 2026-02-19T18:30:30Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- ChordPumperEngine static library compiles with zero JUCE dependency
- All music theory type definitions complete: NoteLetter enum, PitchClass struct with 12 canonical constants, ChordType enum with interval/suffix tables, Chord struct
- Plugin target links against engine and still builds all formats (VST3, CLAP, Standalone)
- Catch2 v3.13.0 integrated via FetchContent, 3 smoke tests discovered by CTest and passing

## Task Commits

Each task was committed atomically:

1. **Task 1: Create ChordPumperEngine static library with complete type definitions** - `5a38d50` (feat)
2. **Task 2: Add Catch2 v3.13.0 via FetchContent and create test target with smoke tests** - `9ea3cc3` (feat)

## Files Created/Modified
- `src/engine/NoteLetter.h` - Note letter enum, semitone lookup table, letter name table
- `src/engine/PitchClass.h` - PitchClass struct with letter+accidental, 12 canonical constants
- `src/engine/PitchClass.cpp` - PitchClass method stubs (name, midiNote)
- `src/engine/ChordType.h` - ChordType enum, constexpr interval table, suffix table, noteCount()
- `src/engine/Chord.h` - Chord struct interface (root, type, noteCount, midiNotes, name)
- `src/engine/Chord.cpp` - Chord method stubs
- `CMakeLists.txt` - ChordPumperEngine static lib, plugin linking, Catch2 FetchContent, test target
- `tests/test_pitch_class.cpp` - PitchClass construction smoke test
- `tests/test_chord.cpp` - Chord construction smoke test
- `tests/test_chord_naming.cpp` - Chord naming smoke test (stub doesn't crash)

## Decisions Made
- Engine as static library decoupled from JUCE — enables fast test compilation without JUCE header overhead
- Catch2 v3.13.0 via FetchContent with GIT_SHALLOW — keeps test deps separate from plugin deps (JUCE/CJE submodules)
- Test build guarded by `CHORDPUMPER_BUILD_TESTS` option — tests don't slow down normal plugin builds
- Stub implementations return zero/empty — Plans 02-02 and 02-03 will replace with real logic via TDD

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Build infrastructure ready for TDD in Plans 02-02 (PitchClass) and 02-03 (Chord)
- All type definitions complete — TDD plans only need to fill in method implementations
- Test pipeline validated end-to-end (compile → link → discover → run)

## Self-Check: PASSED

All 9 created files verified on disk. Both task commits (5a38d50, 9ea3cc3) verified in git log.

---
*Phase: 02-chord-engine*
*Completed: 2026-02-19*
