---
phase: 01-plugin-foundation
plan: 03
subsystem: infra
tags: [pluginval, clap-validator, bitwig, vst3, clap, glibc-compat, validation]

requires:
  - phase: 01-02
    provides: Plugin binaries (VST3, CLAP, Standalone) installed to standard paths
provides:
  - pluginval strictness level 5 validation proof for VST3
  - clap-validator validation (14/17 passed, 3 expected state failures)
  - Bitwig Studio host loading verification for both VST3 and CLAP
  - glibc symbol version pinning for Bitwig sandbox compatibility
affects: [Phase 1 success criteria, all subsequent host compatibility]

tech-stack:
  added: [cmake/glibc-compat/, cmake/glibc_compat_math.c]
  patterns: [compile-time header interception for glibc feature macros, link-time --wrap + .symver for math symbols]

key-files:
  created: [.planning/phases/01-plugin-foundation/validation-results.md, cmake/glibc-compat/bits/libc-header-start.h, cmake/glibc_compat_math.c]
  modified: [CMakeLists.txt]

key-decisions:
  - "glibc compatibility fix required for Bitwig sandbox — CachyOS glibc 2.43 emits GLIBC_2.35+/2.38/2.43 symbols that Bitwig's older runtime rejects"
  - "Two-layer fix: header interception zeros IEC_60559/C23 macros; --wrap + .symver pins math symbols to GLIBC_2.2.5"

patterns-established:
  - "Glibc symbol version pinning via cmake/glibc-compat/ for hosts with older runtimes (Bitwig, other DAWs)"

requirements-completed: [PLAT-02, PLAT-03]

duration: ~15min (Task 1 automated + glibc fix + Task 2 human-verify)
completed: 2026-02-19
---

# Phase 1 Plan 03: Plugin Validation & Bitwig Host Loading Summary

**pluginval (strictness 5) + clap-validator validation with Bitwig Studio host verification, plus glibc compatibility fix for sandboxed DAW runtime**

## Performance

- **Duration:** ~15 min
- **Tasks:** 2 (1 automated, 1 human-verify)
- **Files modified:** 4 (validation artifact + glibc fix)

## Accomplishments

- VST3 passes pluginval at strictness level 5 (maximum attempted) — all tests passed
- CLAP passes clap-validator with 14/17 tests; 3 state failures expected for minimal shell (no parameters)
- Standalone smoke test: launches without crash
- Bitwig Studio loads both VST3 and CLAP as Instruments, displays correct 1000x600 dark window with "ChordPumper v0.1.0" centered, no crash on close
- glibc symbol version pinning fix: max glibc requirement reduced from 2.43 to 2.34 for Bitwig sandbox compatibility

## Task Commits

Each task was committed atomically:

1. **Task 1: Automated plugin validation** - `adc0dd3` (test)
2. **Task 2: Bitwig host loading** - Human-verify checkpoint (approved by user)
3. ** Deviation fix (glibc compatibility)** - `c500af0` (fix) — applied between Task 1 and Task 2

## Files Created/Modified

- `.planning/phases/01-plugin-foundation/validation-results.md` - Validation results (pluginval, clap-validator, standalone)
- `cmake/glibc-compat/bits/libc-header-start.h` - Header interception to zero out IEC_60559/C23 feature macros
- `cmake/glibc_compat_math.c` - Wrapper symbols for sqrtf, atan2f, fmod, hypot, hypotf pinned to GLIBC_2.2.5
- `CMakeLists.txt` - glibc-compat include path, link flags (--wrap), glibc_compat_math.c source

## Decisions Made

- glibc compatibility fix required because CachyOS glibc 2.43 uses IEC 60559 / C23 feature macros that emit symbol version requirements (GLIBC_2.35, 2.38, 2.43). Bitwig Studio loads plugins in a sandboxed runtime with older glibc (~2.34) and rejects binaries with newer symbol versions.
- Two-layer approach: compile-time header interception prevents new symbol emission; link-time --wrap + .symver pins math functions to explicit GLIBC_2.2.5 versions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] glibc symbol version compatibility for Bitwig sandbox**
- **Found during:** Between Task 1 and Task 2 (Bitwig loading would have failed without fix)
- **Issue:** CachyOS glibc 2.43 causes object files to require GLIBC_2.43/2.38/2.35 via IEC 60559 and C23 feature macros. Bitwig's plugin sandbox uses older glibc (~2.34) and rejects the binary.
- **Fix:** Compile-time: intercept `bits/libc-header-start.h` to zero out `IEC_60559_BFP_EXT`, `IEC_60559_FUNCS_EXT`, `ISOC23`, and `C23_STRTOL` macros. Link-time: add `--wrap` + `.symver` for `sqrtf`, `atan2f`, `fmod`, `hypot`, `hypotf` to pin to GLIBC_2.2.5.
- **Files modified:** CMakeLists.txt, cmake/glibc-compat/bits/libc-header-start.h (created), cmake/glibc_compat_math.c (created)
- **Verification:** `readelf -V` shows max glibc requirement 2.34; Bitwig loads VST3 and CLAP without symbol errors
- **Committed in:** c500af0

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Fix essential for Bitwig host loading. Without it, Task 2 human-verify would have failed.

## Issues Encountered

None — validation tools ran successfully; glibc fix was a pre-emptive correction before Bitwig verification.

## User Setup Required

None — pluginval and clap-validator available; Bitwig verification done manually by user.

## Auth Gates

None — no external auth required.

## Next Phase Readiness

- Phase 1 success criteria fully met:
  1. Plugin loads in Bitwig as both VST3 and CLAP without errors ✓
  2. Standalone launches and displays window on CachyOS ✓
  3. CMake/Ninja produces all three formats ✓
  4. Plugin passes pluginval at basic strictness ✓
- Glibc compatibility pattern established for future host compatibility (other DAWs, Linux distros)
- validation-results.md provides baseline for Phase 6 pluginval re-validation

## Self-Check: PASSED

- FOUND: .planning/phases/01-plugin-foundation/01-03-SUMMARY.md
- FOUND: adc0dd3 (Task 1 commit)
- FOUND: c500af0 (glibc fix commit)

---
*Phase: 01-plugin-foundation*
*Completed: 2026-02-19*
