---
phase: quick-4
plan: 01
subsystem: ui
tags: [juce, pad, right-click, quadrant, drag]

requires: []
provides:
  - Right-click drag on sub-variation pads now reads the correct quadrant under the cursor at press time
affects: [PadComponent, sub-variations, octave-shift-right-click]

tech-stack:
  added: []
  patterns: []

key-files:
  created: []
  modified:
    - src/ui/PadComponent.cpp

key-decisions:
  - "pressedQuadrant assigned via quadrantAt() as the first statement in isPopupMenu() block — mirrors left-click path exactly"

patterns-established: []

requirements-completed: []

duration: 3min
completed: 2026-02-20
---

# Quick Task 4: Fix Right-Click Drag Quadrant Selection Summary

**One-line fix: `pressedQuadrant = quadrantAt(event.getPosition())` added as first statement in the `isPopupMenu()` mouseDown block so right-click drag always picks the sub-chord under the cursor, not the stale value from the last left-click.**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-02-20T (session start)
- **Completed:** 2026-02-20
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Identified root cause: right-click path in `mouseDown` read `pressedQuadrant` without ever setting it for the current event
- Added single assignment as first line of `isPopupMenu()` block, mirroring the existing left-click assignment
- Build succeeded cleanly with both VST3 and CLAP artefacts installed

## Task Commits

1. **Task 1: Set pressedQuadrant in right-click mouseDown path** - `e447474` (fix)

## Files Created/Modified
- `/home/stoo/code/GSD/Plugins/ChordPumperCursor/src/ui/PadComponent.cpp` - Added `pressedQuadrant = quadrantAt(event.getPosition())` as first statement inside `isPopupMenu()` block (line 143)

## Decisions Made
- No new decisions — fix follows the exact pattern already established by the left-click path

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Right-click drag quadrant selection is now correct
- Manual verification: right-click each quadrant of a pad with sub-variations, confirm dragged chord matches that quadrant regardless of prior left-click position

---
*Phase: quick-4*
*Completed: 2026-02-20*
