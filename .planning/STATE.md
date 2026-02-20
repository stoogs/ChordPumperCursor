# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-19)

**Core value:** The morphing grid — play a chord, see what comes next, follow the thread.
**Current focus:** Phase 6: State Persistence & Validation

## Current Position

Phase: 6 of 6 (State Persistence & Validation)
Plan: 2 of 3 in current phase
Status: Executing Phase 6
Last activity: 2026-02-20 — Completed 06-02: Editor↔Processor bidirectional state wiring with ChangeBroadcaster notification

Progress: [█████████░] 97%

## Performance Metrics

**Velocity:**
- Total plans completed: 14
- Average duration: ~7 min
- Total execution time: ~1.5 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 1. Plugin Foundation | 3 | ~30 min | ~10 min |
| 2. Chord Engine | 3 | 11 min | ~4 min |

| 3. Playable Grid | 2 | 4 min | 2 min |
| 4. Morphing Suggestions | 3 | ~52 min | ~17 min |
| 5. Capture & Export | 3 | ~14 min | ~5 min |

| 6. State Persistence | 2 | 14 min | 7 min |

**Recent Trend:**
- Last 5 plans: 2min, 5min, 7min, 10min, 4min
- Trend: → (steady)

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
- [03-01]: Hex colour literals in PadComponent matching LookAndFeel scheme; PadColours namespace provides named constants
- [03-01]: juce::Grid with Fr(1) tracks and 4px gap for responsive 8×4 layout
- [03-02]: MidiKeyboardState bridges GUI thread noteOn/Off to audio thread processBlock — JUCE's recommended pattern
- [03-02]: 300ms fixed note duration via juce::Timer; future plans may make this user-configurable
- [03-02]: midiMessages.clear() before processNextMidiBuffer — ChordPumper generates MIDI, does not pass through input
- [04-01]: Tritone disambiguation: ♯IV for major/augmented qualities, ♭v for minor/diminished at interval 6
- [04-01]: VoiceLeader centroid-based placement with ±12 octave search (3^n combinations) for optimal voicing
- [04-01]: Roman numeral quality suffixes: Δ (maj7), °7 (dim7), ø7 (half-dim7), + (augmented), ° (diminished)
- [04-02]: ±1 octave VL search in MorphEngine for transposition-invariant scoring
- [04-02]: Deterministic sort tiebreaker (score desc → interval asc → type asc) for consistent rankings
- [04-02]: PCS deduplication for symmetric chords (aug triads, dim7ths) — keep closest-to-I representative
- [04-02]: Diatonic ranking: vi in top 10, IV/V in top 20 (same-root variants outscore on VL dimension)
- [04-03]: PadComponent two-line layout for chord name + Roman numeral; morph on GUI thread, batch repaint for atomic grid update
- [05-03]: onChordPlayed callback fires before morph — strip captures the chord the user clicked, not the suggestion
- [05-03]: FIFO erase-front for overflow keeps most recent 8 chords visible
- [05-03]: Sibling component wiring via parent lambda: PluginEditor connects GridPanel.onChordPlayed to ProgressionStrip.addChord
- [05-01]: juce_audio_basics linked to ChordPumperTests for MIDI readback — minimal JUCE surface in test binary
- [05-01]: Temp file naming with random hex suffix to avoid collisions in concurrent use
- [05-01]: No refactor phase needed — implementation was minimal from the start
- [05-02]: 6px drag threshold for click/drag disambiguation — below triggers chord play, above initiates DnD
- [05-02]: Right-click fallback export to ~/ChordPumper-Export/ — reliable path regardless of platform DnD support
- [05-02]: Bitwig embedded DnD partially works on Linux X11 (recognizes chords, previews clip) but drop doesn't finalize — known limitation, fallback covers it
- [06-01]: Version=1 integer tag on root ValueTree for future schema migration
- [06-01]: MorphContext child only serialized when hasMorphed=true — avoids stale default data
- [06-01]: Grid pads indexed by 'index' property — deserialization is order-independent
- [06-01]: Default PersistentState initializes gridChords from chromaticPalette() to match visual default
- [06-02]: ChangeBroadcaster on Processor for state-restore notification — lightweight, JUCE-standard pattern
- [06-02]: refreshFromState() as public method on GridPanel and ProgressionStrip — decouples restore trigger from initialization
- [06-02]: State references stored in UI components rather than copied — single source of truth in Processor

### Pending Todos

None yet.

### Blockers/Concerns

- Linux drag-and-drop from embedded plugin windows is unreliable (X11). Validated in Phase 5 — Bitwig recognizes chords but drop doesn't finalize. Fallback export to ~/ChordPumper-Export/ works.
- Morph algorithm implemented and verified — tuning may continue based on user feedback.

## Session Continuity

Last session: 2026-02-20
Stopped at: Completed 06-02-PLAN.md (Editor↔Processor state wiring). Next: 06-03 (pluginval validation, RT safety audit).
Resume file: None
