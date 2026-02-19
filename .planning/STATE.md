# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-19)

**Core value:** The morphing grid — play a chord, see what comes next, follow the thread.
**Current focus:** Phase 2: Chord Engine

## Current Position

Phase: 2 of 6 (Chord Engine)
Plan: 2 of 3 in current phase
Status: Executing
Last activity: 2026-02-19 — Completed 02-02 (PitchClass TDD — semitone, name, midiNote)

Progress: [████░░░░░░] 25%

## Performance Metrics

**Velocity:**
- Total plans completed: 5
- Average duration: ~8 min
- Total execution time: ~0.65 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 1. Plugin Foundation | 3 | ~30 min | ~10 min |
| 2. Chord Engine | 2 | 9 min | ~5 min |

**Recent Trend:**
- Last 5 plans: 2min, 7min, 8min, ~15min, 7min
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

### Pending Todos

None yet.

### Blockers/Concerns

- Linux drag-and-drop from embedded plugin windows is unreliable (X11). Phase 5 must validate feasibility early and have a fallback.
- Morph algorithm is novel — no reference implementation. Phase 4 will need iterative tuning.

## Session Continuity

Last session: 2026-02-19
Stopped at: Completed 02-02-PLAN.md (PitchClass TDD — semitone, name, midiNote)
Resume file: None
