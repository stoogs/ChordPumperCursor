---
phase: 10-strip-interaction-and-octave-control
plan: 01
subsystem: engine
tags: [chord, data-model, persistence, serialisation, juce]

# Dependency graph
requires:
  - phase: 06-state-persistence
    provides: PersistentState toValueTree/fromValueTree for progression chords
provides:
  - Chord struct with octaveOffset (int, default 0) and romanNumeral (std::string, default empty)
  - Serialisation round-trip for octaveOffset and romanNumeral on progression chord nodes
affects:
  - 10-02-octave-control-ui
  - 10-03-strip-roman-numeral-display

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Zero-default struct fields for backward-compatible data model extension"
    - "Safe-default ValueTree getProperty reads ensure forward/backward state file compatibility"

key-files:
  created: []
  modified:
    - src/engine/Chord.h
    - src/PersistentState.cpp

key-decisions:
  - "octaveOffset and romanNumeral added with zero/empty defaults so all existing aggregate initialisers (e.g. {pitches::C, ChordType::Major}) compile unchanged"
  - "Progression chord serialisation uses property key 'roman' (matching existing grid pad convention) and 'octaveOffset' with default 0 for backward compatibility"

patterns-established:
  - "Extend Chord struct fields with zero defaults — no call site changes required"

requirements-completed:
  - STRIP-04
  - STRIP-05

# Metrics
duration: 3min
completed: 2026-02-20
---

# Phase 10 Plan 01: Chord Data Model Extension Summary

**Extended Chord struct with octaveOffset and romanNumeral fields, fully persisted with safe defaults for forward/backward state file compatibility.**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-02-20T15:50:07Z
- **Completed:** 2026-02-20T15:53:24Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added `int octaveOffset = 0` and `std::string romanNumeral` to the `Chord` struct with zero/empty defaults
- All existing Chord construction sites compile unchanged (aggregate initialisation works as before)
- PersistentState serialises both fields on progression chord nodes in `toValueTree`
- PersistentState deserialises both fields with safe defaults (0, "") in `fromValueTree` — old state files load correctly

## Task Commits

Each task was committed atomically:

1. **Task 1: Add octaveOffset and romanNumeral to Chord** - `532ec52` (feat)
2. **Task 2: Persist octaveOffset and romanNumeral on progression chords** - `c914790` (feat)

**Plan metadata:** (to follow)

## Files Created/Modified

- `src/engine/Chord.h` - Added `octaveOffset` and `romanNumeral` fields to the `Chord` struct
- `src/PersistentState.cpp` - Added write/read of `octaveOffset` and `roman` properties on progression chord child nodes

## Decisions Made

- Used property key `"roman"` for romanNumeral (matching existing grid pad serialisation convention) and `"octaveOffset"` for clarity
- Default values 0 and `""` ensure both forward and backward compatibility with existing saved state

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 10-02 (octave control UI) and 10-03 (strip Roman numeral display) can now reference `chord.octaveOffset` and `chord.romanNumeral` without compilation errors
- No blockers

---
*Phase: 10-strip-interaction-and-octave-control*
*Completed: 2026-02-20*
