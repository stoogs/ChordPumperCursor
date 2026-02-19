---
phase: 05-capture-export
plan: 01
subsystem: midi
tags: [juce, midi-file, smf, tdd, catch2]

requires:
  - phase: 02-chord-engine
    provides: "Chord struct with midiNotes(octave) for note data"
provides:
  - "MidiFileBuilder::createMidiFile — temp .mid from any Chord"
  - "MidiFileBuilder::exportToDirectory — named .mid in target directory"
  - "JUCE linked to test target for MIDI readback verification"
affects: [05-capture-export, 06-state-persistence]

tech-stack:
  added: [juce::MidiFile, juce::MidiMessageSequence, juce::FileOutputStream]
  patterns: [static-builder-utility, tdd-red-green]

key-files:
  created:
    - src/midi/MidiFileBuilder.h
    - src/midi/MidiFileBuilder.cpp
    - tests/test_midi_file_builder.cpp
  modified:
    - CMakeLists.txt

key-decisions:
  - "juce_audio_basics linked to ChordPumperTests for MIDI readback — minimal JUCE surface in test binary"
  - "Temp file naming with random hex suffix to avoid collisions in concurrent use"
  - "No refactor phase needed — implementation was minimal from the start"

patterns-established:
  - "JUCE-linked test pattern: add juce::juce_audio_basics to ChordPumperTests for MIDI verification"
  - "Static builder utility: stateless methods with private buildSequence/writeToFile decomposition"

requirements-completed: []

duration: 5min
completed: 2026-02-19
---

# Phase 5 Plan 1: MidiFileBuilder Summary

**TDD-verified Chord-to-MIDI-file builder using juce::MidiFile — SMF Type 0, 480 TPQN, 120 BPM, one-bar clips**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-19T22:04:40Z
- **Completed:** 2026-02-19T22:10:07Z
- **Tasks:** 2 (RED + GREEN; refactor skipped — already clean)
- **Files modified:** 4

## Accomplishments
- MidiFileBuilder creates valid .mid files from any Chord with correct note-on/off events, timestamps, TPQN, and tempo
- 10 Catch2 tests verify all MIDI content by reading back files with juce::MidiFile
- JUCE juce_audio_basics linked to test target — first test with MIDI readback capability
- Both createMidiFile (temp) and exportToDirectory (named) paths tested and working

## Task Commits

Each task was committed atomically:

1. **Task 1: RED — failing tests** - `5620048` (test)
2. **Task 2: GREEN — implementation** - `8e7d632` (feat)

_Refactor phase skipped — implementation was minimal and well-factored from the start._

## Files Created/Modified
- `src/midi/MidiFileBuilder.h` — Static utility: createMidiFile, exportToDirectory, buildSequence, writeToFile
- `src/midi/MidiFileBuilder.cpp` — MIDI file creation using juce::MidiFile + MidiMessageSequence
- `tests/test_midi_file_builder.cpp` — 10 Catch2 tests verifying note count, note numbers, timestamps, TPQN, tempo, file naming
- `CMakeLists.txt` — Added MidiFileBuilder.cpp to plugin + test targets, linked juce_audio_basics to tests

## Decisions Made
- Linked juce_audio_basics to ChordPumperTests for MIDI readback — minimal JUCE surface area in the test binary
- Temp files use random hex suffix for collision avoidance
- File naming uses Chord::name() directly (C Major → "C.mid", not "Cmaj.mid") — matches existing naming convention

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness
- MidiFileBuilder ready for drag-to-DAW integration (Plan 05-02)
- createMidiFile provides the temp file path needed for DnD ExternalDragAndDrop
- exportToDirectory provides the fallback export path
- CAPT-01 not yet complete — requires 05-02 (drag mechanism) to fulfill the user-facing requirement

## Self-Check: PASSED

- All 3 created files exist on disk
- Both commits (5620048, 8e7d632) present in git log
- All 10 MidiFileBuilder tests pass (29 assertions)
- All 64 project tests pass (1439 assertions) — zero regressions
- Plugin builds cleanly: VST3, Standalone, CLAP

---
*Phase: 05-capture-export*
*Completed: 2026-02-19*
