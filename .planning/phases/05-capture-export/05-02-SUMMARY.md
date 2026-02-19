---
phase: 05-capture-export
plan: 02
subsystem: ui
tags: [juce, dnd, drag-and-drop, midi-export, x11, linux]

requires:
  - phase: 05-capture-export
    provides: "MidiFileBuilder::createMidiFile for temp .mid, exportToDirectory for fallback"
  - phase: 03-playable-grid
    provides: "PadComponent with mouseDown click-to-play behavior"
provides:
  - "Pad-to-external DnD via JUCE performExternalDragDropOfFiles"
  - "Click/drag disambiguation with 6px threshold"
  - "Right-click fallback export to ~/ChordPumper-Export/"
  - "DragAndDropContainer inheritance on PluginEditor"
affects: [06-state-persistence]

tech-stack:
  added: [juce::DragAndDropContainer, juce::DragAndDropContainer::performExternalDragDropOfFiles]
  patterns: [click-drag-disambiguation, fallback-export-path]

key-files:
  created: []
  modified:
    - src/ui/PadComponent.h
    - src/ui/PadComponent.cpp
    - src/ui/PluginEditor.h

key-decisions:
  - "6px drag threshold for click/drag disambiguation — below threshold triggers chord play, above initiates DnD"
  - "Right-click fallback export to ~/ChordPumper-Export/ — reliable path regardless of DnD platform support"
  - "Bitwig embedded plugin DnD partially works on Linux X11 (recognizes chords, previews clip) but drop doesn't finalize — known X11 limitation, fallback path covers this"
  - "2-second delayed temp file cleanup after DnD — gives DAW time to read the file"

patterns-established:
  - "Click/drag disambiguation: mouseDown for click, mouseDrag with distance threshold for DnD"
  - "Fallback export pattern: right-click context action to ~/ChordPumper-Export/ for platform-limited scenarios"

requirements-completed: [CAPT-01]

duration: 7min
completed: 2026-02-19
---

# Phase 5 Plan 2: Drag-to-DAW Summary

**Pad-to-DAW drag-and-drop with 6px click/drag disambiguation, X11 feasibility validated, right-click fallback export to ~/ChordPumper-Export/**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-19T22:15:05Z
- **Completed:** 2026-02-19T22:22:10Z
- **Tasks:** 2 (1 auto + 1 human-verify checkpoint)
- **Files modified:** 3

## Accomplishments
- Drag-and-drop from chord pads to external applications via JUCE performExternalDragDropOfFiles
- Click/drag disambiguation: left-click plays chord (preserved), drag past 6px initiates file DnD
- Right-click fallback export writes named .mid to ~/ChordPumper-Export/ — works perfectly in all contexts
- Linux X11 DnD feasibility validated: Standalone DnD works, Bitwig embedded partially works (recognizes/previews but doesn't finalize drop)

## Task Commits

Each task was committed atomically:

1. **Task 1: Wire DnD from PadComponent with click/drag disambiguation and fallback export** - `fc1c08c` (feat)
2. **Task 2: Verify DnD in Standalone and Bitwig** - checkpoint:human-verify (approved)

## Files Created/Modified
- `src/ui/PadComponent.h` — Added mouseDrag override and isDragInProgress flag
- `src/ui/PadComponent.cpp` — DnD initiation with distance threshold, right-click fallback export, temp file cleanup
- `src/ui/PluginEditor.h` — Added DragAndDropContainer to inheritance list

## Decisions Made
- 6px drag threshold balances click sensitivity vs accidental drag — standard JUCE practice
- Right-click fallback to ~/ChordPumper-Export/ ensures usability even when platform DnD is limited
- 2-second delayed temp file cleanup gives DAWs time to read the .mid before deletion
- Bitwig embedded DnD limitation accepted as known X11 issue — fallback path is the production workflow on Linux

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None — all three test scenarios behaved as predicted by the plan.

## User Verification Results

| Test | Description | Result | Notes |
|------|-------------|--------|-------|
| Test 1 | Standalone DnD | PASS | Drag shows stop-sign cursor (X11 cosmetic) but files land successfully, 54-byte .mid created |
| Test 2 | Bitwig embedded DnD | Partial | Bitwig recognizes chords, previews 1-bar clip with correct notes, but drop doesn't finalize — known Linux X11 embedded plugin limitation |
| Test 3 | Fallback export | PASS | Right-click exports .mid to ~/ChordPumper-Export/ — works perfectly |

**Acceptance:** Test 1 + Test 3 pass. Test 2 is a known platform limitation with a working fallback. Checkpoint approved.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness
- Phase 5 complete — all three plans (MidiFileBuilder, Drag-to-DAW, ProgressionStrip) done
- CAPT-01 fulfilled: user can drag pads to create MIDI clips (Standalone) or right-click export (universal fallback)
- Ready for Phase 6: State Persistence & Validation

## Self-Check: PASSED

- All 3 modified files exist on disk
- Commit fc1c08c present in git log
- User verification: 2/3 tests pass, 1 partial (known platform limitation with fallback)

---
*Phase: 05-capture-export*
*Completed: 2026-02-19*
