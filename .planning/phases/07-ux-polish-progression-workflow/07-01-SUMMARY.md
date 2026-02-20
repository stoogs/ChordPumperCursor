---
phase: 07-ux-polish-progression-workflow
plan: 01
subsystem: ui
tags: [juce, drag-and-drop, DragAndDropTarget, DragAndDropContainer, midi]

requires:
  - phase: 05-capture-export
    provides: PadComponent click/drag, MidiFileBuilder, ProgressionStrip
  - phase: 06-state-persistence
    provides: PersistentState round-trip, Editor↔Processor wiring
provides:
  - Intra-plugin DnD from PadComponent to ProgressionStrip via JUCE DragAndDropTarget
  - Click-only-plays behavior (click does NOT add to strip)
  - External DnD fallback via shouldDropFilesWhenDraggedExternally on editor
affects: [07-02, 07-03]

tech-stack:
  added: []
  patterns: [JUCE DragAndDropTarget/Container for intra-plugin DnD, mouseUp click detection with drag guard]

key-files:
  created: []
  modified:
    - src/ui/PadComponent.cpp
    - src/ui/ProgressionStrip.h
    - src/ui/ProgressionStrip.cpp
    - src/ui/PluginEditor.h
    - src/ui/PluginEditor.cpp

key-decisions:
  - "mouseUp-based onClick with isDragInProgress guard and 6px threshold for click/drag disambiguation"
  - "startDragging on DragAndDropContainer for intra-plugin DnD instead of performExternalDragDropOfFiles"
  - "shouldDropFilesWhenDraggedExternally on editor provides external DnD fallback transparently"
  - "Kept MidiFileBuilder include in PadComponent.cpp — right-click export still needs it (plan deviation)"

patterns-established:
  - "JUCE intra-plugin DnD: PadComponent startDragging → ProgressionStrip DragAndDropTarget itemDropped"
  - "Visual drag-over feedback via isReceivingDrag bool + semi-transparent overlay in paint()"

requirements-completed: [UX-01, UX-02]

duration: 5min
completed: 2026-02-20
---

# Phase 7 Plan 1: Click/Drag Disambiguation & Intra-Plugin DnD Summary

**Rewired PadComponent so click-only-plays and drag-to-strip-adds via JUCE DragAndDropTarget, with external DnD fallback on shouldDropFilesWhenDraggedExternally**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-20T09:17:39Z
- **Completed:** 2026-02-20T09:22:36Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- PadComponent click (< 6px) plays chord and morphs grid without adding to strip
- PadComponent drag (>= 6px) initiates intra-plugin DnD via startDragging
- ProgressionStrip accepts drops from PadComponent and adds chord to sequence with visual feedback
- External DnD fallback creates MIDI files via shouldDropFilesWhenDraggedExternally override

## Task Commits

Each task was committed atomically:

1. **Task 1: Rework PadComponent click/drag disambiguation** - `e1baff7` (feat)
2. **Task 2: Make ProgressionStrip a DragAndDropTarget and disconnect auto-add** - `a2caa4b` (feat)

## Files Created/Modified
- `src/ui/PadComponent.cpp` - mouseDown no longer calls onClick; mouseUp click detection with drag guard; mouseDrag uses startDragging
- `src/ui/ProgressionStrip.h` - Added DragAndDropTarget inheritance and method declarations
- `src/ui/ProgressionStrip.cpp` - Implemented isInterestedInDragSource, itemDropped, itemDragEnter/Exit; drag-over highlight overlay
- `src/ui/PluginEditor.h` - Added shouldDropFilesWhenDraggedExternally declaration
- `src/ui/PluginEditor.cpp` - Removed auto-add wiring; implemented shouldDropFilesWhenDraggedExternally for external DnD

## Decisions Made
- mouseUp-based onClick with isDragInProgress guard and 6px threshold for clean click/drag disambiguation
- startDragging on DragAndDropContainer replaces performExternalDragDropOfFiles for intra-plugin DnD
- shouldDropFilesWhenDraggedExternally on ChordPumperEditor provides transparent external DnD fallback
- Kept MidiFileBuilder include in PadComponent.cpp (plan deviation) — right-click export still calls it

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Kept MidiFileBuilder include in PadComponent.cpp**
- **Found during:** Task 1 (PadComponent click/drag rework)
- **Issue:** Plan instructed removing `#include "midi/MidiFileBuilder.h"` from PadComponent.cpp, but right-click export path still calls `MidiFileBuilder::exportToDirectory` — removing it would break the build
- **Fix:** Kept the include; only the mouseDrag external DnD code was removed as intended
- **Files modified:** src/ui/PadComponent.cpp
- **Verification:** Build succeeds with no errors
- **Committed in:** e1baff7 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Necessary to keep right-click export working. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Intra-plugin DnD foundation complete for 07-02 (strip click-to-play + MIDI export)
- ProgressionStrip DragAndDropTarget pattern established for potential future expansion
- All existing functionality (morph, state persistence, right-click export) preserved

## Self-Check: PASSED

All 5 modified source files exist on disk. Both task commits (e1baff7, a2caa4b) present in git log. SUMMARY.md created. Build succeeds with no errors.

---
*Phase: 07-ux-polish-progression-workflow*
*Completed: 2026-02-20*
