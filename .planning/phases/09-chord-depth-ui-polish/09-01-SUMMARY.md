---
phase: 09-chord-depth-ui-polish
plan: 01
subsystem: engine
tags: [chord-types, intervals, pitch-class-set, morph-engine, look-and-feel]

# Dependency graph
requires:
  - phase: 08-grid-ux-overhaul
    provides: MorphEngine returning std::array<ScoredChord, 64> over 64-pad grid
provides:
  - ChordType enum with 18 values (9 triads/7ths + 9 extension types)
  - kIntervals[18][6] and kChordSuffix[18] lookup tables
  - noteCount() returning 3/4/5/6 for triads/7ths/9ths/11ths+13ths
  - allChords() returning std::array<Chord, 216> and kAllChords constexpr
  - accentForType() with 18-entry colour array for extension types
  - MorphEngine reserve(216) for expanded chord pool
affects:
  - 09-02 (pad sub-variation UI — needs ChordType enum values and kAllChords)
  - any future code that switches on ChordType or indexes kChordSuffix/kIntervals

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "6-slot interval arrays with -1 sentinel for unused slots — backward-compatible with existing noteCount() iteration"
    - "noteCount() range-based dispatch: t<4 triads, t<9 7ths, 9/12/15 are 9ths, else 11ths/13ths"
    - "Colour progression by extension depth: each family (Maj/Min/Dom) gets lighter shades for 9th/11th/13th"

key-files:
  created: []
  modified:
    - src/engine/ChordType.h
    - src/engine/PitchClassSet.h
    - src/ui/ChordPumperLookAndFeel.h
    - src/engine/MorphEngine.cpp
    - src/engine/RomanNumeral.h

key-decisions:
  - "6-slot kIntervals with -1 sentinels: backward-compatible — existing types still iterate only noteCount() slots (3 or 4)"
  - "noteCount() multi-range dispatch for 18 types: t<4 triads, t<9 7ths, specific indices (9,12,15) for 9ths, else 6-note"
  - "kAllChords expanded from 108 to 216: 12 roots x 18 types, all extension types accessible via MorphEngine"
  - "Extension accent colours: lighter shades of family colour (Maj=blue, Min=purple, Dom=teal) by extension depth"

patterns-established:
  - "Extension type ordering: Maj9/Maj11/Maj13 then Min9/Min11/Min13 then Dom9/Dom11/Dom13"

requirements-completed:
  - DEPTH-01
  - DEPTH-02
  - DEPTH-03

# Metrics
duration: 4min
completed: 2026-02-20
---

# Phase 9 Plan 01: Chord Depth UI Polish — Expand to 18 Chord Types Summary

**18-type ChordType system with 6-slot intervals, 216 all-chords pool, and extension-family accent colours as foundation for pad sub-variations**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-02-20T14:05:17Z
- **Completed:** 2026-02-20T14:09:21Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- ChordType enum expanded from 9 to 18 values with Maj9/Maj11/Maj13, Min9/Min11/Min13, Dom9/Dom11/Dom13
- kIntervals expanded to [18][6] with -1 sentinels for unused slots; fully backward-compatible
- kAllChords now yields 216 chords (12 roots x 18 types); MorphEngine pool reservation updated to match
- accentForType() covers all 18 types with lighter-shade extension colours per family
- Full release build succeeds and installs VST3/CLAP

## Task Commits

Each task was committed atomically:

1. **Task 1: Expand ChordType.h to 18 types with 6-slot interval tables** - `2059a2e` (feat)
2. **Task 2: Update PitchClassSet.h, ChordPumperLookAndFeel.h, and MorphEngine.cpp for 18 types** - `b423807` (feat)

**Plan metadata:** (docs commit — see final commit)

## Files Created/Modified

- `src/engine/ChordType.h` - 18-value enum, kIntervals[18][6], kChordSuffix[18], noteCount() multi-range
- `src/engine/PitchClassSet.h` - allChords() returns array<Chord, 216>, types array extended to 18 entries
- `src/ui/ChordPumperLookAndFeel.h` - accentForType() accents[18] with 9 extension colours
- `src/engine/MorphEngine.cpp` - reserve(216) for expanded chord pool
- `src/engine/RomanNumeral.h` - isUpperCase() extended for Maj9/Maj11/Maj13/Dom9/Dom11/Dom13 (bug fix)

## Decisions Made

- 6-slot kIntervals with -1 sentinels chosen for backward compatibility — Chord.cpp and VoiceLeader.cpp iterate only up to noteCount() so extra slots are never read
- noteCount() multi-range dispatch: `t < 4` triads (3 notes), `t < 9` 7ths (4 notes), specific indices 9/12/15 for 9ths (5 notes), all others 11ths/13ths (6 notes)
- Extension accent colours use progressively lighter shades of each family's existing colour — visually groups extension types with their family

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed isUpperCase() in RomanNumeral.h for new major/dominant extension types**
- **Found during:** Task 1 (reviewing code that uses ChordType enum)
- **Issue:** isUpperCase() switch had `default: return false` — Maj9/Maj11/Maj13 and Dom9/Dom11/Dom13 would incorrectly return lowercase Roman numerals
- **Fix:** Added explicit cases for all 6 major/dominant extension types returning `true`
- **Files modified:** src/engine/RomanNumeral.h
- **Verification:** Build succeeds; Maj9/Dom9 family correctly uppercase in switch
- **Committed in:** 2059a2e (part of Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 correctness bug)
**Impact on plan:** Essential fix — without it, all major/dominant extension chords would show lowercase Roman numerals (vi instead of VI). No scope creep.

## Issues Encountered

None — build succeeded on first attempt for both tasks.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Foundation complete: all 18 ChordType values exist, kAllChords has 216 entries, noteCount() correct for all types
- Ready for 09-02: pad sub-variation UI can now present extension types (Maj9/Dom9/etc.) as depth choices per pad quadrant
- MorphEngine will now consider extension chords in morph results — may surface 9th/11th/13th chords on grid pads

---
*Phase: 09-chord-depth-ui-polish*
*Completed: 2026-02-20*
