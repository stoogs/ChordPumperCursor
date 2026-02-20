---
phase: 09-chord-depth-ui-polish
plan: 02
subsystem: ui
tags: [juce, pad, quadrant, sub-variations, glow, border, chord-extension]

# Dependency graph
requires:
  - phase: 09-01
    provides: "18 ChordType enum values including Maj7/Min7/Dom7 and all extension types"
provides:
  - "PadComponent quadrant split for Major/Minor/Maj7/Min7/Dom7 chord types"
  - "Per-quadrant chord label rendering with faint separator lines"
  - "Quadrant-aware press handling — clicking a quadrant fires the correct extension chord"
  - "3px solid border on all pads (was 1.5px)"
  - "3-ring concentric glow effect on pad hover using accent colour"
affects: ["09-03", "09-04", "future-ui-plans"]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "quadrantAt() hit-test using integer midpoint division"
    - "applySubVariations() anonymous-namespace helper in GridPanel.cpp for DRY wiring"
    - "Concentric glow rings via multiple drawRoundedRectangle calls with decreasing alpha"

key-files:
  created: []
  modified:
    - "src/ui/PadComponent.h"
    - "src/ui/PadComponent.cpp"
    - "src/ui/GridPanel.cpp"

key-decisions:
  - "Quadrant index layout: TL=0, TR=1, BL=2, BR=3 — matches reading order"
  - "pressedQuadrant reset in mouseExit as well as mouseUp to handle fast mouse movements"
  - "applySubVariations() as anonymous-namespace free function in GridPanel.cpp — avoids polluting GridPanel class interface"
  - "Glow rings drawn BEFORE solid border so border sits on top cleanly"
  - "Extension type pads (Dom9, Maj11, etc.) fall through to default: disabling sub-variations — correct since they are already an extension"

patterns-established:
  - "setSubVariations() takes bool + array — single call sets/clears state and triggers repaint"
  - "Both morphTo() and refreshFromState() must call applySubVariations to keep display consistent on reload"

requirements-completed:
  - DEPTH-01
  - DEPTH-02

# Metrics
duration: 15min
completed: 2026-02-20
---

# Phase 9 Plan 02: Quadrant Sub-Variations and Glow Borders Summary

**Quadrant chord-extension selector on Major/Minor/Maj7/Min7/Dom7 pads with concentric hover glow and 3px borders across all 64 pads**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-02-20T14:07:00Z
- **Completed:** 2026-02-20T14:22:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- PadComponent gains quadrant split rendering for 5 qualifying chord types — each quadrant shows the extension chord name at 7.5pt font with faint separator lines
- Pressing any quadrant fires onPressStart/onPressEnd with the correct extension chord (e.g. TR on Cmaj fires Cmaj7)
- All pads display a 3px solid border (up from 1.5px) for better visual weight at the 1000x600 window size
- Hovering any pad draws 3 concentric glow rings in the pad's accent colour at 0.07/0.13/0.25 alpha
- GridPanel wires sub-variations in both morphTo() and refreshFromState() so the quadrant layout is maintained across sessions

## Task Commits

1. **Task 1: Add quadrant fields and glow border to PadComponent** - `dea9f12` (feat)
2. **Task 2: Wire sub-variations in GridPanel** - `af71ff2` (feat)

## Files Created/Modified

- `/home/stoo/code/GSD/Plugins/ChordPumperCursor/src/ui/PadComponent.h` - Added hasSubVariations, subChords[4], pressedQuadrant fields; setSubVariations() setter; quadrantAt() helper declaration
- `/home/stoo/code/GSD/Plugins/ChordPumperCursor/src/ui/PadComponent.cpp` - Implemented quadrantAt(), setSubVariations(), quadrant label paint, separator lines, glow rings, 3px border, quadrant-aware mouse handlers
- `/home/stoo/code/GSD/Plugins/ChordPumperCursor/src/ui/GridPanel.cpp` - Added applySubVariations() anonymous-namespace helper, called after setChord() in morphTo() and refreshFromState()

## Decisions Made

- Quadrant index layout TL=0, TR=1, BL=2, BR=3 matches reading order and makes bit math clear (bottom adds 2, right adds 1)
- pressedQuadrant also reset in mouseExit so rapid mouse-exit during drag doesn't leave stale state
- Glow rings use drawRoundedRectangle at 9.0f/6.0f/4.0f thickness before the 3.0f solid border
- Extension type pads already ARE an extension — they correctly fall to default: which disables sub-variations

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - build succeeded on first attempt for both tasks.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Quadrant sub-variation infrastructure is fully in place for any future extension of qualifying types
- Glow effect is paint-only (no GlowEffect component) — safe with opaque pad backgrounds
- Ready for 09-03 and any further UI polish phases

## Self-Check: PASSED

- FOUND: src/ui/PadComponent.h
- FOUND: src/ui/PadComponent.cpp
- FOUND: src/ui/GridPanel.cpp
- FOUND: .planning/phases/09-chord-depth-ui-polish/09-02-SUMMARY.md
- FOUND: commit dea9f12 (feat(09-02): add quadrant fields and glow border to PadComponent)
- FOUND: commit af71ff2 (feat(09-02): wire sub-variations in GridPanel for qualifying chord types)
