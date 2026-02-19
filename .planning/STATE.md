# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-19)

**Core value:** The morphing grid — play a chord, see what comes next, follow the thread.
**Current focus:** Phase 2: Chord Engine

## Current Position

Phase: 3 of 6 (Playable Grid)
Plan: 0 of 4 in current phase
Status: Phase 2 complete, ready for Phase 3
Last activity: 2026-02-19 — Completed 02-03 (Chord TDD — construction, MIDI, naming for 108 combinations)

Progress: [████░░░░░░] 28%

## Performance Metrics

**Velocity:**
- Total plans completed: 6
- Average duration: ~7 min
- Total execution time: ~0.7 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 1. Plugin Foundation | 3 | ~30 min | ~10 min |
| 2. Chord Engine | 3 | 11 min | ~4 min |

**Recent Trend:**
- Last 5 plans: 7min, 8min, ~15min, 7min, 2min
- Trend: ↓ (faster)

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Roadmap]: 6 phases derived from 21 requirements; Phases 4 & 5 parallelizable
- [Roadmap]: Voice leading (CHRD-03) grouped with morph engine (Phase 4) — both define the chord transition experience
- [Roadmap]: Linux DnD feasibility spike in Phase 5; clipboard/file-export fallback planned
- [01-01]: Added C language to project() — JUCE/CJE internals require C compiler; CMake 4.x errors without it
- [01-01]: Created placeholder .cpp files — CMake 4.x validates source file existence at configure time
- [01-01]: Plugin categorized as Instrument (IS_SYNTH TRUE) with MIDI output — user locked decision
- [01-02]: Added juce_audio_devices and juce_audio_utils — required by JUCE Standalone plugin client
- [01-02]: Used FontOptions constructor for juce::Font — deprecated float constructor in JUCE 8
- [01-02]: LookAndFeel member declared before Component members — C++ reverse destruction order ensures safe lifetime
- [01-03]: glibc symbol version pinning (cmake/glibc-compat/) for Bitwig sandbox — CachyOS glibc 2.43 emits GLIBC_2.35+/2.38/2.43; Bitwig runtime ~2.34 rejects; header interception + --wrap/.symver pins math symbols to 2.2.5
- [02-01]: Engine as static library (ChordPumperEngine) decoupled from JUCE — enables fast test compilation
- [02-01]: Catch2 v3.13.0 via FetchContent with GIT_SHALLOW — keeps test deps separate from plugin deps
- [02-01]: Test build guarded by CHORDPUMPER_BUILD_TESTS option — tests don't slow plugin builds
- [02-02]: Double-mod pattern ((x % 12 + 12) % 12) for negative accidental wrapping — handles Cb→11, Fb→4
- [02-02]: No refactor phase needed — TDD implementation was minimal and clean from the start
- [02-03]: No refactor phase needed — three one-liner methods delegating to lookup tables
- [02-03]: Chord::noteCount() delegates to free function rather than duplicating logic

### Pending Todos

None yet.

### Blockers/Concerns

- Linux drag-and-drop from embedded plugin windows is unreliable (X11). Phase 5 must validate feasibility early and have a fallback.
- Morph algorithm is novel — no reference implementation. Phase 4 will need iterative tuning.

## Session Continuity

Last session: 2026-02-19
Stopped at: Completed 02-03-PLAN.md (Chord TDD — Phase 2 complete, ready for Phase 3)
Resume file: None
