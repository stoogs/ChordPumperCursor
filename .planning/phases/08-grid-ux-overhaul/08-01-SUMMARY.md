---
phase: 08-grid-ux-overhaul
plan: 01
subsystem: ui
tags: [juce, midi, dnd, interaction-model, callbacks]

requires:
  - phase: 07-ux-polish
    provides: Click/drag disambiguation, progression strip, DnD infrastructure
provides:
  - Hold-to-preview pad interaction (onPressStart/onPressEnd sustained MIDI)
  - Public GridPanel::morphTo() method for external morph triggering
  - Strip-driven morph wiring (onChordClicked → morphTo, onChordDropped → morphTo)
  - Decoupled preview from morph — pads preview only, strip drives all morphing
affects: [08-02-PLAN, 08-03-PLAN]

tech-stack:
  added: []
  patterns:
    - "Hold-to-preview: onPressStart fires noteOn on mouseDown, onPressEnd fires noteOff on mouseUp or drag start"
    - "Strip-driven morph: morphTo() is public on GridPanel, called only from Editor lambdas wired to strip callbacks"
    - "Drag safety: onPressEnd fires before startDragging to prevent stuck MIDI notes"

key-files:
  created: []
  modified:
    - src/ui/PadComponent.h
    - src/ui/PadComponent.cpp
    - src/ui/GridPanel.h
    - src/ui/GridPanel.cpp
    - src/ui/ProgressionStrip.h
    - src/ui/ProgressionStrip.cpp
    - src/ui/PluginEditor.cpp

key-decisions:
  - "onClick kept but unwired — future use for non-morph pad click actions"
  - "morphTo uses 32 pads for now — plan 08-02 will expand to 64"
  - "Strip onChordClicked calls morphTo THEN plays chord — grid updates before audio fires"
  - "onChordDropped fires after addChord — strip UI updates before morph triggers"

patterns-established:
  - "Hold-to-preview: sustained MIDI via onPressStart/onPressEnd callbacks"
  - "Strip-driven morph: all morphing flows through Editor lambdas, not pad click handlers"

requirements-completed: []

duration: 6min
completed: 2026-02-20
---

# Phase 8 Plan 01: Hold-to-Preview & Strip-Driven Morph Summary

**Decoupled pad preview from morph — pads sustain MIDI on hold, strip clicks/drops drive all grid morphing via public GridPanel::morphTo()**

## Performance

- **Duration:** 6 min
- **Started:** 2026-02-20T11:53:54Z
- **Completed:** 2026-02-20T12:00:16Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Pads now sustain MIDI note-on while held (onPressStart) and release on mouseUp or drag start (onPressEnd) — no more fixed 300ms timer
- GridPanel no longer inherits juce::Timer; padClicked, timerCallback, and onChordPlayed all removed
- Public morphTo() method on GridPanel handles morph + state persistence, callable from Editor
- Strip click morphs grid then plays chord with 300ms timed release; strip drop morphs grid without playing

## Task Commits

Each task was committed atomically:

1. **Task 1: Refactor PadComponent for hold-to-preview and add GridPanel morphTo** - `aeca13d` (feat)
2. **Task 2: Wire strip-driven morph triggers in Editor and ProgressionStrip** - `e274012` (feat)

## Files Created/Modified
- `src/ui/PadComponent.h` - Added onPressStart/onPressEnd callback declarations
- `src/ui/PadComponent.cpp` - Fire onPressStart on mouseDown, onPressEnd on drag start and mouseUp
- `src/ui/GridPanel.h` - Removed Timer inheritance, added morphTo/startPreview/stopPreview
- `src/ui/GridPanel.cpp` - Replaced padClicked/timerCallback with startPreview/stopPreview/morphTo
- `src/ui/ProgressionStrip.h` - Added onChordDropped callback declaration
- `src/ui/ProgressionStrip.cpp` - Fire onChordDropped in itemDropped after addChord
- `src/ui/PluginEditor.cpp` - Wire onChordClicked to morphTo+play, onChordDropped to morphTo

## Decisions Made
- onClick kept on PadComponent but not wired to anything morph-related — preserved for potential future use
- morphTo uses 32-pad loop for now; plan 08-02 will expand to 64
- Strip onChordClicked calls morphTo first so grid updates before the chord plays
- onChordDropped fires after addChord so strip UI updates before morph triggers

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- morphTo() is public and ready for 08-02 to expand from 32 to 64 pads
- Interaction model fully decoupled — 08-03 can add similarity colours to existing pad infrastructure
- Build clean, all plugin formats compile and install

---
*Phase: 08-grid-ux-overhaul*
*Completed: 2026-02-20*
