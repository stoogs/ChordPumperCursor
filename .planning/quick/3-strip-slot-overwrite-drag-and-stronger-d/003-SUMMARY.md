---
phase: quick-3
plan: "003"
subsystem: ui/progression-strip
tags: [drag-and-drop, ux, progression-strip, overwrite, visual-feedback]
dependency_graph:
  requires: []
  provides: [strip-overwrite-drop, strip-insert-indicator, strip-reorder-swap]
  affects: [ProgressionStrip]
tech_stack:
  added: []
  patterns: [overwrite-vs-insert hit-test, slot boundary detection from relX/cellWidth]
key_files:
  created: []
  modified:
    - src/ui/ProgressionStrip.h
    - src/ui/ProgressionStrip.cpp
decisions:
  - "overwriteIndex member alongside insertionIndex — mutually exclusive: only one is >= 0 at a time during drag"
  - "Cell hit-test uses posInCell < slotWidth to distinguish slot interior from 4px gap — same geometry as insertionIndexAtX"
  - "REORDER swap uses std::swap in-place rather than erase+insert — preserves strip length and avoids index correction math"
  - "Insertion line upgraded to 4px white (was 3px blue) — stronger contrast against dark background"
  - "Overwrite highlight: bright yellow (0xffffff00, alpha 0.9, 2.5px) drawn after accent border so it composites on top"
metrics:
  duration: "~15 min"
  completed: "2026-02-20T15:21:16Z"
  tasks_completed: 2
  files_modified: 2
---

# Quick Task 3: Strip Slot Overwrite Drag and Stronger Indicator Summary

**One-liner:** Slot-aware drag-and-drop on the progression strip — overwrite existing slots or insert at gaps, with bright white insertion line and yellow overwrite highlight.

## What Was Built

The progression strip drag-and-drop now distinguishes between two drop modes based on cursor position during drag:

- **Overwrite mode:** Cursor directly over an existing slot — highlights slot with bright yellow border, replaces on drop
- **Insert mode:** Cursor in a gap between slots or past the end — shows bright white 4px vertical line, inserts on drop

Both pad drops and intra-strip reorder drags respect this logic. Reorder drags that land on another existing slot **swap** the two chords rather than inserting.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add overwriteIndex and slot hit-test logic | 9649348 | ProgressionStrip.h, ProgressionStrip.cpp |
| 2 | Wire overwrite into itemDropped and update paint | 7028cec | ProgressionStrip.cpp |

## Key Changes

**ProgressionStrip.h:**
- Added `int overwriteIndex = -1;` member alongside `insertionIndex`

**ProgressionStrip.cpp — itemDragMove:**
- Computes `relX`, `cell`, `posInCell` from cursor position against slot geometry
- If cursor is inside an existing slot's pixel bounds and not the REORDER source: `overwriteIndex = cell`, `insertionIndex = -1`
- Otherwise: `overwriteIndex = -1`, `insertionIndex = insertionIndexAtX(xPos)`

**ProgressionStrip.cpp — itemDragExit:**
- Added `overwriteIndex = -1` reset alongside existing `insertionIndex = -1`

**ProgressionStrip.cpp — itemDropped (pad branch):**
- `overwriteIndex >= 0`: replace `chords[overwriteIndex]` in-place, persist, fire `onChordDropped`
- `insertionIndex >= 0`: cap-aware insert at index position, persist, fire `onChordDropped`
- fallback: `addChord()` as before

**ProgressionStrip.cpp — itemDropped (REORDER branch):**
- `overwriteIndex >= 0` and not same as source: `std::swap` the two slots
- Otherwise: existing erase+insert reorder logic unchanged
- Both paths reset `overwriteIndex = -1` in cleanup

**ProgressionStrip.cpp — paint:**
- Insertion line: `juce::Colours::white`, width 4px (was 3px blue `0xff6c8ebf`)
- Overwrite highlight: `juce::Colour(0xffffff00).withAlpha(0.9f)`, `drawRoundedRectangle` at 2.5px, drawn after accent border

## Deviations from Plan

None — plan executed exactly as written.

## Self-Check: PASSED

- `src/ui/ProgressionStrip.h` contains `overwriteIndex` member: confirmed
- `src/ui/ProgressionStrip.cpp` contains overwrite logic in itemDragMove, itemDropped, paint: confirmed
- Commit `9649348` exists: confirmed
- Commit `7028cec` exists: confirmed
- Build succeeded with no errors
