# Project Research Summary

**Project:** Chord-Grid
**Domain:** JUCE C++ Audio Plugin (MIDI)
**Researched:** 2026-02-19
**Confidence:** HIGH

## Executive Summary

Chord-Grid is a MIDI chord exploration plugin for Linux (VST3/CLAP) built with JUCE 8 and C++20. The product's core differentiator — an 8x4 morphing grid that suggests harmonically related next chords after each trigger — has no direct competitor on any platform and faces zero competition on Linux, where the Bitwig/Ardour/REAPER ecosystem has no dedicated chord exploration tools. Research confirms the stack is mature (JUCE 8, CMake, clap-juce-extensions, Catch2), the feature scope is well-defined against a crowded competitor field (Scaler, Captain Chords, Cthulhu, Chordjam), and the architecture follows established JUCE audio plugin patterns with clear component boundaries.

The recommended approach is a phased build starting from a real-time-safe plugin skeleton, then layering a custom music theory engine (no viable C++ libraries exist), MIDI I/O, grid UI, the morph/suggestion algorithm, voice leading, and drag-to-DAW export. The theory engine and morph algorithm are the highest-complexity components and the product's core IP — they should be built as pure logic modules with exhaustive Catch2 tests, independent of JUCE's GUI layer. The architecture strictly separates audio-thread and message-thread concerns using lock-free state sharing, which research unanimously identifies as the single most critical design decision.

The key risk is Linux drag-and-drop from plugin windows. JUCE's `performExternalDragDropOfFiles()` is unreliable when the plugin is embedded in a host's X11 window hierarchy. This affects the MIDI-to-DAW drag feature — a core user workflow. Mitigation: test in Bitwig plugin context during the earliest prototype phase, investigate CLAP drag-and-drop extensions, and implement a clipboard/file-export fallback. All other risks (real-time safety, state versioning, thread safety) are well-understood with established prevention patterns that must be applied from Phase 1.

## Key Findings

### Recommended Stack

JUCE 8.0.12 is the clear choice — it's the industry-standard C++ audio framework with native Linux support, MIDI file I/O, and a mature GUI toolkit. CLAP format support comes via clap-juce-extensions (used by Surge XT, ChowDSP), bridging until JUCE 9 ships native CLAP. No viable C++ music theory library exists; the theory engine must be built from scratch (~500-1000 lines of well-scoped, testable code). The full toolchain is available on CachyOS rolling release.

**Core technologies:**
- **JUCE 8.0.12:** Plugin framework, MIDI I/O, GUI, state management — industry standard, dual-licensed AGPLv3/commercial
- **C++20:** Ranges, span, concepts, designated initializers — safe choice with GCC 15 on Linux-only target
- **CMake + Ninja:** JUCE's native build system since v6, 3-5x faster incremental builds than Make
- **clap-juce-extensions:** CLAP format support — mature, MIT licensed, pin to known-good commit
- **Catch2 v3:** Unit testing for theory engine, morph algorithm, voice leading — actively maintained, CMake-native
- **Custom music theory engine:** Intervals, scales, chords, harmonic proximity — no production-quality C++ library exists

### Expected Features

The competitor landscape is well-mapped (Scaler 3, Captain Chords, Cthulhu, Chordjam, ChordPotion, Ripchord, InstaChord). None of them run on Linux.

**Must have (table stakes):**
- Chord triggering from MIDI input with chord name display
- Multiple chord types (triads + 7ths minimum)
- Inversions / basic voicing control
- MIDI output to external instruments
- MIDI drag-and-drop to DAW (individual chords)
- Next-chord suggestions
- Progression capture strip (4-8 chords)

**Should have (differentiators):**
- Morphing 8x4 grid that recomputes after each selection (unique — no competitor has this)
- No fixed key required (removes biggest friction in competitors)
- Hybrid theory + proximity suggestions (diatonic + neo-Riemannian + common-tone)
- Contextual Roman numerals relative to last-played chord (unique pedagogical angle)
- Smart automatic voice leading pre-computed for all 32 pads
- Chromatic palette starting state before first selection

**Defer (v2+):**
- Extended chord types (9ths, 11ths, 13ths)
- Built-in sequencer/arranger (anti-feature — let the DAW handle this)
- Audio chord detection (separate product category)
- Arpeggiator/strumming (complementary tool, not core)
- Cross-platform (validate Linux-first, then expand)

### Architecture Approach

The architecture follows standard JUCE plugin patterns with strict audio/message thread separation. The AudioProcessor owns all state and handles MIDI in `processBlock()` on the real-time audio thread. The AudioProcessorEditor owns the GUI and polls processor state via Timer at 30 Hz. Complex state (the 32-chord grid) crosses threads via lock-free snapshot swap — never a mutex. The MorphEngine runs on the message thread because it's computationally expensive (~32 candidate scoring passes); results are published to the audio thread atomically.

**Major components:**
1. **ChordGridProcessor** — AudioProcessor subclass; MIDI routing, chord voicing, state persistence
2. **ChordGridEditor** — AudioProcessorEditor; grid UI, Timer-based polling, drag source
3. **MusicTheoryEngine** — Pure logic (stateless); intervals, scales, chords, Roman numerals
4. **MorphEngine** — Grid recalculation algorithm; theory + proximity hybrid scoring
5. **VoiceLeading** — Minimal voice movement optimization; runs in audio thread fast path
6. **MidiRouter** — Maps incoming MIDI to pad triggers
7. **ChordVoicer** — Builds chord notes from grid state, applies voice leading, writes MIDI output
8. **GridPanel + PadComponent** — 8x4 interactive UI with chord labels, highlights, drag support
9. **LockFreeState** — Atomic snapshot swap for audio/message thread communication

### Critical Pitfalls

1. **Audio thread memory allocation** — Any `new`, `std::vector::push_back`, `juce::String`, or `DBG()` in `processBlock()` causes dropouts. Pre-allocate everything in `prepareToPlay()`. Run pluginval at strictness 10 to detect violations.
2. **Locks/mutexes in audio thread** — Priority inversion causes random dropouts. Use `std::atomic`, `juce::AbstractFifo`, and lock-free snapshot swap — never `std::mutex` or `CriticalSection` in audio code paths.
3. **Linux drag-and-drop from plugin windows** — `performExternalDragDropOfFiles()` fails from X11-embedded plugin windows on most Linux DEs. Test in Bitwig plugin context immediately; implement clipboard/file-export fallback.
4. **parameterChanged fires on audio thread** — DAW automation can trigger parameter callbacks on the audio thread. Only set atomic flags in callbacks; use Timer polling for UI updates.
5. **Plugin state versioning** — Include a version integer in serialized state from V1. Never remove parameter IDs. Write `migrateState()` even if empty. Old sessions must not break on updates.

## Implications for Roadmap

Based on combined research, the build should follow this phase structure:

### Phase 1: Plugin Skeleton + Build Infrastructure
**Rationale:** Architecture research identifies real-time safety patterns, state versioning, and lock-free communication as decisions that must be correct from day one. Retrofitting is extremely painful. CLAP setup should happen here too — catching build issues early is cheap.
**Delivers:** Buildable VST3/CLAP/Standalone plugin that loads in Bitwig, empty UI, correct CMake configuration, git submodules, clangd integration, pluginval passing at strictness 10.
**Addresses:** Core MIDI I/O foundation, build infrastructure
**Avoids:** Pitfalls 1 (allocation), 2 (locks), 4 (parameter threading), 5 (state versioning), 6 (singletons)

### Phase 2: Music Theory Engine
**Rationale:** The theory engine is the foundation for everything else (morph algorithm, chord voicing, Roman numerals, grid population). It has zero JUCE GUI dependencies and can be built and tested entirely in isolation with Catch2. Architecture research confirms this is ~500-1000 lines of well-defined logic.
**Delivers:** Interval arithmetic, chord construction (triads + 7ths), scale membership, Roman numeral calculation, chord quality types. Exhaustive unit test suite.
**Addresses:** Multiple chord types, scale/key awareness (internal), chord name generation
**Avoids:** Pitfall of building untestable monolithic code

### Phase 3: MIDI Processing Pipeline
**Rationale:** Depends on Phase 1 (processBlock) + Phase 2 (chord building). This connects incoming MIDI to chord output — the fundamental I/O that makes the plugin useful. No GUI needed; testable by sending MIDI and verifying output.
**Delivers:** MidiRouter (input mapping), ChordVoicer (chord → MIDI notes), basic output to DAW. Playable via MIDI controller.
**Addresses:** Chord triggering, MIDI output to external instruments
**Avoids:** MidiBuffer timing pitfalls, MIDI output validation

### Phase 4: Grid UI (Static)
**Rationale:** Can start as soon as Phase 2 is done (needs ChordDefinition for labels). Depends on Phase 1 (editor shell). Initially displays a hardcoded grid; wiring to live data comes later. Gets the visual interface right before adding dynamic behavior.
**Delivers:** 8x4 GridPanel with PadComponent, chord labels, click-to-trigger, custom LookAndFeel. Static chord content.
**Addresses:** Chord name display, grid visual layout, pad interaction
**Avoids:** Paint allocation, cascading repaints, LookAndFeel destruction order, editor lifecycle bugs

### Phase 5: Lock-Free Wiring + Parameters
**Rationale:** Bridges audio thread and GUI thread. Depends on Phase 3 + Phase 4 both existing. This is the most architecturally sensitive phase — the lock-free state sharing pattern must be correct.
**Delivers:** LockFreeState implementation, Timer-based polling, APVTS parameter setup, grid highlights on pad play, live MIDI-driven grid display.
**Addresses:** Real-time UI ↔ processor communication
**Avoids:** Pitfalls 1, 2, 4 (all thread-safety critical)

### Phase 6: Morph Engine (Core Algorithm)
**Rationale:** This is the product's core IP and highest-complexity component. Depends on Phase 2 (theory) + Phase 5 (state sharing to publish new grids). The hybrid scoring algorithm (diatonic + modal interchange + neo-Riemannian proximity + common-tone) defines the product's value.
**Delivers:** Grid recomputes after each chord selection. 32 contextually relevant suggestions. Chromatic palette initial state. Scoring algorithm with configurable weights.
**Addresses:** Morphing grid, hybrid suggestions, no-fixed-key workflow, chromatic palette start
**Avoids:** Complex computation on audio thread (runs on message thread)

### Phase 7: Voice Leading
**Rationale:** Depends on Phase 3 (ChordVoicer) + Phase 2 (note sets). Can be built in parallel with Phase 6. Must be fast enough for audio-thread execution (sorting 3-6 notes).
**Delivers:** Automatic smooth voice leading between consecutive chords. Minimal voice movement, no voice crossing.
**Addresses:** Smart voice leading, inversions/voicing control
**Avoids:** Voice leading recalculation per sample (cache results)

### Phase 8: Drag-to-DAW + Progression Strip
**Rationale:** Independent of the morph engine — depends on Phase 4 (GridPanel) + Phase 2 (chord building). This is the capture/export workflow. Must validate Linux DnD feasibility early (see risk below).
**Delivers:** Progression capture strip (4-8 chords), MIDI file creation, drag individual chords to DAW. Clipboard/file-export fallback if X11 DnD fails.
**Addresses:** MIDI drag-and-drop, progression capture
**Avoids:** Linux DnD pitfall (by having fallback ready)

### Phase 9: State Persistence + Polish
**Rationale:** Depends on Phase 5 (APVTS) + Phase 6 (grid state). Full session recall: save grid contents, morph state, parameters. Preset support. Final pluginval validation.
**Delivers:** Complete state save/restore, version-tagged serialization, preset management, multi-DAW testing pass.
**Addresses:** State round-trip, session reliability
**Avoids:** State versioning pitfall, editor re-open crashes

### Phase Ordering Rationale

- **Phases 1-3 form the critical path:** Skeleton → theory → MIDI I/O. Each strictly depends on the prior.
- **Phase 4 (Grid UI) parallelizes with Phase 3** once Phase 2 is complete, since it only needs ChordDefinition types for display.
- **Phases 6 and 7 can run in parallel** — morph engine and voice leading are independent algorithms that both consume the theory engine.
- **Phase 8 (Drag-to-DAW) is structurally independent** but has the highest risk item (Linux DnD). A feasibility spike should happen during Phase 1; full implementation in Phase 8.
- **Architecture pitfalls (real-time safety, lock-free patterns, state versioning) are all addressed in Phase 1** because they cannot be retrofitted without rewrites.
- **The morph algorithm (Phase 6) is deliberately late** because it needs all infrastructure in place, but it's the core product value — budget extra time here.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 6 (Morph Engine):** The hybrid scoring algorithm (diatonic + neo-Riemannian + common-tone weighting) is novel. No direct implementation reference exists. Needs algorithmic design research and iterative tuning.
- **Phase 8 (Drag-to-DAW):** Linux X11 DnD from embedded plugin windows is documented as unreliable. Needs feasibility validation and fallback design. CLAP drag-and-drop extension support in Bitwig needs investigation.
- **Phase 5 (Lock-Free Wiring):** Lock-free state sharing patterns are well-documented in isolation but tricky to integrate correctly. Specific pattern (triple-buffer vs atomic swap) needs validation with actual grid state size.

Phases with standard patterns (skip research-phase):
- **Phase 1 (Skeleton):** Well-documented JUCE CMake setup. Pamplejuce template and JUCE examples cover this comprehensively.
- **Phase 2 (Theory Engine):** Music theory is a well-defined domain. Interval arithmetic, chord construction, and scale membership are textbook algorithms.
- **Phase 3 (MIDI Processing):** JUCE's MidiBuffer/MidiMessage API is thoroughly documented. Arpeggiator tutorial covers the pattern.
- **Phase 4 (Grid UI):** Standard JUCE Component hierarchy. Custom LookAndFeel is well-documented.
- **Phase 9 (State Persistence):** JUCE's ValueTree serialization is well-documented with multiple tutorials.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | JUCE 8, CMake, C++20, Catch2 — all verified against official sources. Versions confirmed on CachyOS. |
| Features | HIGH | 8 competitors analyzed with official sources. Feature gaps and anti-features clearly identified. |
| Architecture | HIGH | Standard JUCE plugin patterns with extensive official documentation. Lock-free patterns confirmed across multiple authoritative sources. |
| Pitfalls | HIGH (core), MEDIUM (Linux DnD) | Core pitfalls (real-time safety, threading) universally documented. Linux DnD issues confirmed but exact workarounds need validation. |

**Overall confidence:** HIGH

### Gaps to Address

- **Linux drag-and-drop feasibility:** Must validate `performExternalDragDropOfFiles()` from a JUCE plugin embedded in Bitwig during Phase 1 prototyping. If it fails, the entire DnD feature design changes to clipboard/file-export.
- **CLAP drag-and-drop extensions:** Bitwig co-created CLAP and may support native drag-and-drop extensions that bypass X11 DnD issues. Needs investigation.
- **Morph algorithm design:** The hybrid theory + proximity scoring algorithm is novel. No reference implementation exists. Will require iterative tuning with musical validation — budget for experimentation.
- **Wayland compatibility:** JUCE is X11-only on Linux. XWayland works but has known focus and input issues. Document the requirement; test under both X11 and XWayland.
- **JUCE licensing decision:** JUCE 8 is AGPLv3 or commercial. Project must decide licensing intent before first release — affects distribution model.

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 official releases and API docs — framework, MIDI, GUI, state management
- JUCE CMake API documentation — build system configuration
- JUCE Linux Dependencies documentation — system requirements
- clap-juce-extensions GitHub repository — CLAP format support, known issues
- Catch2 v3.13.0 official releases — testing framework
- Bitwig CLAP announcement — DAW integration
- Steinberg VST3 documentation — plugin locations and format

### Secondary (MEDIUM confidence)
- melatonin.dev JUCE tips and tricks — real-time safety, pluginval, UI performance
- JUCE Forum threads — audio thread allocation, Linux DnD, parameter threading, state issues
- Dave Rowland (ADC) presentations — lock-free audio patterns
- Competitor product pages and documentation — Scaler 3, Captain Chords, Cthulhu, Chordjam, etc.
- Neo-Riemannian theory (Wikipedia, music21) — harmonic proximity algorithms

### Tertiary (LOW confidence)
- CLAP drag-and-drop extension support in Bitwig — needs validation
- XWayland plugin embedding behavior — limited test reports
- JUCE 9 native CLAP timeline — announced, no release date

---
*Research completed: 2026-02-19*
*Ready for roadmap: yes*
