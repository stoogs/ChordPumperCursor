# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-19)

**Core value:** The morphing grid — play a chord, see what comes next, follow the thread.
**Current focus:** Phase 9: Chord Depth & UI Polish

## Current Position

Phase: 9 of 9 (Chord Depth & UI Polish)
Plan: 2 of ? — complete
Status: Phase 9 in progress — Plan 02 executed (quadrant sub-variations, glow borders)
Last activity: 2026-02-20 - Completed 09-02: Quadrant sub-variation selector and hover glow on pads

Progress: [██████████] 100% (phases 1-8 complete, phase 9 started)

## Performance Metrics

**Velocity:**
- Total plans completed: 18
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

| 6. State Persistence | 3 | ~22 min | ~7 min |
| 7. UX Polish & Progression | 3 | ~18 min | ~6 min |

| 8. Grid UX Overhaul | 3 | ~33 min | ~11 min |

**Recent Trend:**
- Last 5 plans: 5min, 8min, 6min, 7min, 20min
- Trend: → (steady, 08-03 longer due to LTO link times)

*Updated after each plan completion*
| Phase 09 P03 | 4 | 2 tasks | 2 files |

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
- [06-03]: LTO disabled for Release glibc-compat builds (-fno-lto) — --wrap linker fails with LTO on glibc symbols
- [06-03]: CLAP pluginval unsupported on platform — documented; VST3 validation suffices for production gate
- [07-01]: mouseUp-based onClick with isDragInProgress guard and 6px threshold for click/drag disambiguation
- [07-01]: startDragging on DragAndDropContainer for intra-plugin DnD instead of performExternalDragDropOfFiles
- [07-01]: shouldDropFilesWhenDraggedExternally on editor provides external DnD fallback transparently
- [07-01]: Kept MidiFileBuilder include in PadComponent.cpp — right-click export still needs it (plan deviation)
- [07-02]: getChordIndexAtPosition accounts for 120px button area and 4px inter-slot gaps
- [07-02]: SafePointer<ChordPumperEditor> in Timer callback prevents crash if editor destroyed during 300ms note
- [07-02]: FileChooser stored as std::unique_ptr member — prevents premature destruction in async context
- [07-02]: exportProgression places each chord at i*kBarLengthTicks for sequential bar playback
- [07-03]: 9 chord-type accent colours as inline constexpr in PadColours with accentForType() zero-overhead lookup
- [07-03]: Gradient brighter/darker offset 0.05f (pads) / 0.03f (strip) — subtle, non-distracting
- [07-03]: Accent border opacity varies by state: 0.4f normal, 0.6f hovered, 0.8f pressed
- [08-01]: onClick kept on PadComponent but unwired to morph — preserved for future use
- [08-01]: morphTo uses 32-pad loop; plan 08-02 will expand to 64
- [08-01]: Strip onChordClicked calls morphTo first so grid updates before chord plays
- [08-01]: onChordDropped fires after addChord so strip UI updates before morph triggers
- [08-02]: Score normalization divides composite by weightSum for full [0,1] range
- [08-02]: Variety minimum raised from 2→4 per quality category for 64-pad grid
- [08-02]: Pool size cap widened 40→72 to feed 64 final slots
- [08-02]: v1→v2 state migration fills indices 32-63 from chromaticPalette
- [08-02]: Window height 1200px accommodates 8 rows at current pad size
- [08-03]: 5-stop gradient with interpolation between stops for smooth colour transitions
- [08-03]: Score sentinel -1.0f triggers chord-type accent fallback — no boolean flag needed
- [08-03]: Restored sessions reset scores to -1 rather than re-morphing — user re-morphs on next strip interaction
- [Phase quick-1]: Window resized 1000x1200->1000x600; pad fonts scaled 14/14/11->12/11/9pt
- [09-01]: kIntervals expanded to [18][6] with -1 sentinels — backward-compatible, existing types iterate only noteCount() slots
- [09-01]: noteCount() multi-range dispatch: t<4 triads, t<9 7ths, indices 9/12/15 for 9ths, else 6-note 11ths/13ths
- [09-01]: kAllChords expanded 108->216 (12 roots x 18 types); MorphEngine reserve updated to match
- [09-01]: Extension accent colours use lighter shades of family colour by depth (Maj=blue, Min=purple, Dom=teal)
- [Phase 09]: REORDER:N drag prefix distinguishes intra-strip reorder from pad drops — no boolean state flag needed
- [Phase 09]: insertionIndex is gap index (0..N) not slot index — toIdx>fromIdx correction after erase ensures correct final position
- [09-02]: Quadrant index TL=0 TR=1 BL=2 BR=3 — bottom adds 2, right adds 1; clear bit layout
- [09-02]: pressedQuadrant reset in mouseExit as well as mouseUp — handles fast mouse moves during drag
- [09-02]: applySubVariations() as anonymous-namespace free function in GridPanel.cpp — keeps GridPanel class interface clean
- [09-02]: Glow rings painted before solid border so border sits cleanly on top
- [quick-2]: onPressStart/onPressEnd callbacks on ProgressionStrip mirror pad hold pattern; pressedIndex tracks chord across mouseDown/mouseUp
- [quick-2]: stripActiveNotes vector in PluginEditor owns MIDI state for immediate noteOff on release — no timer
- [quick-2]: itemDragMove insertionIndex set for all drag sources; REORDER: prefix guard was unnecessary

### Roadmap Evolution

- Phase 7 added: UX Polish & Progression Workflow (drag-to-add strip, click-to-play, MIDI export, visual polish)
- Phase 8 added: Grid UX Overhaul (hold-to-preview, drag-to-morph flow, harmonic similarity colours, 8×8 grid)

### Pending Todos

None yet.

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 1 | Resize plugin window to 1000x600 and fit 8x8 grid | 2026-02-20 | e9346e5 | [1-resize-plugin-window-to-1000x600-and-fit](./quick/1-resize-plugin-window-to-1000x600-and-fit/) |
| 2 | Progression strip hold-to-play and drag insertion line | 2026-02-20 | cd521d5 | [2-progression-strip-hold-to-play-and-drag-](./quick/2-progression-strip-hold-to-play-and-drag-/) |

### Blockers/Concerns

- Linux drag-and-drop from embedded plugin windows is unreliable (X11). Validated in Phase 5 — Bitwig recognizes chords but drop doesn't finalize. Fallback export to ~/ChordPumper-Export/ works.
- Morph algorithm implemented and verified — tuning may continue based on user feedback.

## Session Continuity

Last session: 2026-02-20
Stopped at: Completed quick-2 (progression strip hold-to-play and drag insertion line)
Resume file: None
