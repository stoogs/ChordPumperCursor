---
phase: 07-ux-polish-progression-workflow
plan: 02
subsystem: ui
tags: [juce, midi, FileChooser, MidiFileBuilder, progression, click-to-play]

requires:
  - phase: 07-ux-polish-progression-workflow
    provides: Intra-plugin DnD, ProgressionStrip DragAndDropTarget, click/drag disambiguation
  - phase: 05-capture-export
    provides: MidiFileBuilder, PadComponent MIDI export, ProgressionStrip
provides:
  - Strip slot click-to-play via mouseDown + onChordClicked callback
  - Multi-chord MIDI progression export via async FileChooser
  - MidiFileBuilder::exportProgression placing chords at successive bar boundaries
affects: [07-03]

tech-stack:
  added: []
  patterns: [mouseDown slot hit-testing with gap-aware geometry, async FileChooser for plugin-safe file dialogs, SafePointer timer callback for editor lifetime safety]

key-files:
  created: []
  modified:
    - src/ui/ProgressionStrip.h
    - src/ui/ProgressionStrip.cpp
    - src/ui/PluginEditor.cpp
    - src/midi/MidiFileBuilder.h
    - src/midi/MidiFileBuilder.cpp

key-decisions:
  - "getChordIndexAtPosition accounts for 120px button area and 4px inter-slot gaps"
  - "SafePointer<ChordPumperEditor> in Timer callback prevents crash if editor destroyed during 300ms note"
  - "FileChooser stored as std::unique_ptr member to prevent premature destruction in async context"
  - "exportProgression places each chord at i*kBarLengthTicks for sequential bar playback"

patterns-established:
  - "Slot hit-testing: getChordIndexAtPosition maps pixel position to chord index with gap rejection"
  - "Async file dialog: FileChooser as member + launchAsync pattern for plugin-safe modal-free dialogs"

requirements-completed: [UX-03, UX-04]

duration: 5min
completed: 2026-02-20
---

# Phase 7 Plan 2: Strip Click-to-Play & MIDI Export Summary

**Clickable progression strip slots with MIDI note preview and Export button with async FileChooser writing multi-chord MIDI files at successive bar boundaries**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-20T09:49:18Z
- **Completed:** 2026-02-20T10:30:07Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Clicking a filled strip slot plays that chord via MidiKeyboardState noteOn/Off with 300ms duration
- Export button next to Clear saves full progression as a multi-chord MIDI file
- Each chord placed at successive bar boundaries (not stacked at beat 1)
- FileChooser uses launchAsync — no modal loops in plugin context

## Task Commits

Each task was committed atomically:

1. **Task 1: Strip click-to-play with slot hit detection** - `c3eddc7` (feat)
2. **Task 2: Multi-chord MIDI export with async FileChooser** - `8758eb7` (feat)

## Files Created/Modified
- `src/ui/ProgressionStrip.h` - Added onChordClicked, mouseDown, exportButton, fileChooser, getChordIndexAtPosition, updateExportButton, exportProgression
- `src/ui/ProgressionStrip.cpp` - Implemented slot hit-testing, mouseDown with DnD guards, export button wiring, async FileChooser dialog, updated layout to 120px for both buttons
- `src/ui/PluginEditor.cpp` - Wired onChordClicked callback to MidiKeyboardState noteOn/Off with SafePointer timer
- `src/midi/MidiFileBuilder.h` - Added exportProgression static method declaration
- `src/midi/MidiFileBuilder.cpp` - Implemented exportProgression with per-bar chord placement and tempo meta event

## Decisions Made
- getChordIndexAtPosition reserves 120px from right for Export + Clear buttons, matching the paint() and resized() layout
- SafePointer wraps the editor reference in the 300ms Timer callback to handle editor destruction gracefully
- FileChooser stored as member (not stack variable) to prevent destruction before async callback fires
- exportProgression uses kBarLengthTicks (1920 ticks = 1 bar at 480 TPQ) stride for sequential chord placement

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Strip is now a fully interactive composition tool: click to preview, drag to add, Export to save
- Ready for 07-03 (visual polish and final UX refinements)
- All prior functionality preserved (DnD, state persistence, right-click export)

## Self-Check: PASSED

All 5 modified source files exist on disk. Both task commits (c3eddc7, 8758eb7) present in git log. SUMMARY.md created. Build succeeds with no errors.

---
*Phase: 07-ux-polish-progression-workflow*
*Completed: 2026-02-20*
