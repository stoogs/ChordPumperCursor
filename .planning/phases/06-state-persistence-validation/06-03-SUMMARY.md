---
phase: 06-state-persistence-validation
plan: 03
subsystem: validation
tags: [pluginval, rt-safety, bitwig, vst3, clap, processBlock, state-persistence]

requires:
  - phase: 06-state-persistence-validation
    provides: PersistentState serialization (06-01), Editor↔Processor state wiring (06-02)
  - phase: 04-morphing-suggestions
    provides: MorphEngine, processBlock MIDI output path
provides:
  - pluginval level 5 validation for VST3 (all passed)
  - processBlock real-time safety audit — no allocations, midiMessages.ensureSize(2048) in prepareToPlay
  - Bitwig state persistence verification (human-approved)
affects: [06-state-persistence-validation]

tech-stack:
  added: []
  patterns: [processBlock RT-audit, midiMessages pre-allocation in prepareToPlay]

key-files:
  created: []
  modified: [CMakeLists.txt, src/PluginProcessor.cpp]

key-decisions:
  - "LTO disabled for Release glibc-compat builds (-fno-lto) — --wrap linker fails with LTO on glibc symbols"
  - "CLAP pluginval: documented as unsupported on platform — tool limitation, not plugin issue"

patterns-established:
  - "pluginval level 5 as production quality gate — no shipping without passing"
  - "processBlock audit: buffer.clear, midiMessages.clear, processNextMidiBuffer only; pre-allocate in prepareToPlay"

requirements-completed: [PLAT-06, PLAT-07]

duration: 8min
completed: 2026-02-20
---

# Phase 6 Plan 3: pluginval Validation and State Persistence Summary

**pluginval level 5 VST3 validation (all passed), processBlock RT safety audit with midiMessages pre-allocation, Bitwig state persistence verified by user**

## Performance

- **Duration:** ~8 min
- **Tasks:** 2
- **Files modified:** 2 (Task 1); Task 2 was human verification only

## Accomplishments

- pluginval VST3 strictness level 5: **ALL PASSED**
- CLAP: pluginval does not support CLAP on this platform — documented as tool limitation
- processBlock RT audit: Confirmed safe — `buffer.clear()`, `midiMessages.clear()`, `keyboardState.processNextMidiBuffer`; added `midiMessages.ensureSize(2048)` in prepareToPlay to preclude any growth during processBlock
- Bitwig state persistence: User verified grid and progression restore correctly on save/reload

## Task Commits

Each task was committed atomically:

1. **Task 1: pluginval validation + RT audit** - `9c3ef57` (chore)
2. **Task 2: Bitwig state persistence** - Human-verify checkpoint (approved by user)

## Files Created/Modified

- `CMakeLists.txt` - Added `-fno-lto` for Release glibc-compat build (LTO + --wrap linker fix)
- `src/PluginProcessor.cpp` - `midiMessages.ensureSize(2048)` in prepareToPlay

## Decisions Made

- LTO disabled for Release builds using glibc --wrap symbol interception — linker fails with LTO; -fno-lto applied to chordpumper target when glibc-compat enabled
- CLAP validation skipped in pluginval — tool does not support CLAP on Linux; VST3 validation suffices for production gate

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed LTO + glibc --wrap linker failure on Release builds**
- **Found during:** Task 1 (pluginval validation — Release build required)
- **Issue:** With glibc-compat (--wrap for math symbols), LTO caused undefined reference errors at link time
- **Fix:** Added `-fno-lto` for the chordpumper target when glibc-compat is enabled
- **Files modified:** CMakeLists.txt
- **Verification:** Release build completes, pluginval passes
- **Committed in:** 9c3ef57 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Necessary for Release build to succeed. No scope creep.

## Issues Encountered

- pluginval CLAP validation: Tool does not support CLAP on Linux. Documented; VST3 validation covers production gate.
- User noted UI hasn't changed visually after 06-02 — expected; Phase 6 scope is persistence/validation only, not UI changes.

## User Setup Required

None - no external service configuration required.

## Auth Gates

None.

## Next Phase Readiness

- **Phase 6 complete.** All state persistence and validation plans executed.
- Plugin is production-ready: pluginval level 5 passed, RT-safe processBlock, state persists in Bitwig.
- PLAT-06 and PLAT-07 satisfied.

## Self-Check: PASSED

- FOUND: .planning/phases/06-state-persistence-validation/06-03-SUMMARY.md
- FOUND: commit 9c3ef57

---
*Phase: 06-state-persistence-validation*
*Completed: 2026-02-20*
