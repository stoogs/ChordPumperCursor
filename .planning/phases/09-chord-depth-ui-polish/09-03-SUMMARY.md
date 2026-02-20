---
phase: 09-chord-depth-ui-polish
plan: 03
subsystem: ui
tags: [juce, drag-and-drop, progression-strip, reorder, cpp]

# Dependency graph
requires:
  - phase: 07-ux-polish-progression
    provides: ProgressionStrip with DragAndDropTarget, getChordIndexAtPosition, pad-drop logic
  - phase: 09-chord-depth-ui-polish
    provides: plan 02 context (if any strip changes)
provides:
  - Drag-to-reorder within ProgressionStrip via REORDER:N drag prefix
  - 3px blue insertion cursor feedback during reorder drag
  - erase+insert reorder with corrected toIdx after erase
  - itemDragExit and itemDragMove implementations for reorder lifecycle
affects: [future-progression-features, state-persistence]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "REORDER:N drag prefix distinguishes intra-strip reorder from pad-to-strip drops"
    - "insertionIndexAtX() maps pixel x to slot gap index using cellWidth = slotWidth + 4"
    - "erase-then-insert with toIdx correction when target was after source"

key-files:
  created: []
  modified:
    - src/ui/ProgressionStrip.h
    - src/ui/ProgressionStrip.cpp

key-decisions:
  - "REORDER:N string prefix in drag description differentiates intra-strip reorder from pad drops — no boolean state flag needed"
  - "10px mouseDrag threshold (vs 6px for pad) prevents accidental reorder on chord click"
  - "insertionIndex is a gap index (0..N) not a slot index — allows insertion before first or after last"
  - "toIdx > fromIdx correction after erase ensures chord lands at correct final position"
  - "toIdx == fromIdx+1 treated as no-op — drop at same or immediately-after position does nothing"

patterns-established:
  - "Insertion cursor: 3px blue fillRect between slots at insertionIndex * cellWidth offset"
  - "Drag lifecycle: mouseDrag initiates, itemDragMove tracks cursor, itemDropped commits, itemDragExit resets"

requirements-completed: [DEPTH-03]

# Metrics
duration: 4min
completed: 2026-02-20
---

# Phase 9 Plan 03: Chord Reorder Drag Summary

**Drag-to-reorder within ProgressionStrip using REORDER:N drag prefix, insertion cursor feedback, and corrected erase+insert logic**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-20T14:11:41Z
- **Completed:** 2026-02-20T14:15:52Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added mouseDrag() to ProgressionStrip that initiates REORDER:N drag from chord slots with 10px threshold
- Added itemDragMove() tracking insertionIndex and visual 3px blue insertion cursor in paint()
- Updated itemDropped() to handle REORDER: prefix with erase+insert reorder and toIdx correction
- Pad-to-strip drag-drop continues to append as before (no regression)
- State persisted to persistentState.progression after each reorder

## Task Commits

Each task was committed atomically:

1. **Task 1: Add reorder drag fields and mouseDrag() to ProgressionStrip** - `3b80cd7` (feat)
2. **Task 2: Reorder drop logic, insertion cursor paint, drag exit, isInterestedInDragSource update** - `72d55fc` (feat)

## Files Created/Modified

- `src/ui/ProgressionStrip.h` - Added reorderDragFromIndex, insertionIndex fields; mouseDrag(), itemDragMove(), insertionIndexAtX() declarations
- `src/ui/ProgressionStrip.cpp` - Implemented full reorder drag lifecycle: initiation, cursor tracking, drop with erase+insert, drag exit reset, paint cursor

## Decisions Made

- REORDER:N string prefix in drag description differentiates intra-strip reorder from pad drops — clean separation without boolean flags
- 10px mouseDrag threshold (vs 6px for pads) prevents accidental reorder on chord click
- insertionIndex is a gap index (0..N), not a slot index, allowing insertion before slot 0 or after last slot
- toIdx correction after erase: when toIdx > fromIdx, decrement by 1 to account for the removed element
- toIdx == fromIdx or toIdx == fromIdx+1 is a no-op (chord stays in same logical position)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Drag-to-reorder is fully functional in the release build
- ProgressionStrip now supports both append (pad drop) and reorder (strip-internal drag) workflows
- Phase 9 plan 03 complete; no further blockers for progression workflow

---
*Phase: 09-chord-depth-ui-polish*
*Completed: 2026-02-20*
