# Roadmap: Chord-Grid

## Overview

Chord-Grid goes from empty project to shippable MIDI chord exploration plugin in 6 phases. We start with build infrastructure and plugin formats, then build the music theory engine as a testable core, wire up a playable grid with MIDI output, implement the morphing suggestion algorithm (the product's soul), add capture/export for the DAW workflow, and close with state persistence and validation. Phases 4 and 5 can run in parallel — the morph engine and capture workflow are independent once the playable grid exists.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Plugin Foundation** - Buildable VST3/CLAP/Standalone plugin that loads in Bitwig on Linux
- [x] **Phase 2: Chord Engine** - Music theory primitives: chord construction, naming, and querying
- [x] **Phase 3: Playable Grid** - 8x4 pad grid with chromatic palette, click-to-play via MIDI output
- [x] **Phase 4: Morphing Suggestions** - Grid morphs after each chord with voice-led transitions and Roman numerals
- [x] **Phase 5: Capture & Export** - Drag chords to DAW as MIDI clips, progression strip
- [x] **Phase 6: State Persistence & Validation** - Session recall, pluginval, real-time safety verification
- [x] **Phase 7: UX Polish & Progression Workflow** - Drag-to-add progression strip, click-to-play strip chords, MIDI file export, visual polish
- [ ] **Phase 8: Grid UX Overhaul** - Hold-to-preview, drag-to-morph flow, harmonic similarity colours, 8×8 grid

**Parallelization:** Phase 5 can execute in parallel with Phase 4 (both depend on Phase 3, neither depends on the other).

## Phase Details

### Phase 1: Plugin Foundation
**Goal**: Plugin builds on Linux and loads correctly in Bitwig as VST3, CLAP, and Standalone
**Depends on**: Nothing (first phase)
**Requirements**: PLAT-01, PLAT-02, PLAT-03, PLAT-04, PLAT-05
**Success Criteria** (what must be TRUE):
  1. Plugin loads in Bitwig Studio as both VST3 and CLAP without errors
  2. Standalone application launches and displays a window on CachyOS
  3. CMake build with Ninja produces all three plugin formats from a single configuration
  4. Plugin passes pluginval at basic strictness (pre-feature baseline)
**Plans**: 3 plans

Plans:
- [x] 01-01-PLAN.md — Repository scaffold, git submodules (JUCE 8.0.12 + clap-juce-extensions), CMake build configuration
- [x] 01-02-PLAN.md — Plugin source shell (Processor, Editor, LookAndFeel) and first build of all formats
- [x] 01-03-PLAN.md — pluginval + clap-validator validation, Bitwig host loading verification

### Phase 2: Chord Engine
**Goal**: All v1 chord types can be constructed, voiced, and named programmatically with full test coverage
**Depends on**: Phase 1
**Requirements**: CHRD-01, CHRD-02
**Success Criteria** (what must be TRUE):
  1. All triads (major, minor, diminished, augmented) and 7th chords (maj7, min7, dom7, dim7, half-dim7) can be constructed from any root note
  2. Every constructed chord produces a correct human-readable name (e.g., "Dm7", "F#aug")
  3. Catch2 test suite covers all chord types, all 12 root notes, and enharmonic edge cases
**Plans**: 3 plans

Plans:
- [x] 02-01-PLAN.md — Engine library scaffold (ChordPumperEngine static lib) + Catch2 v3 test infrastructure
- [x] 02-02-PLAN.md — PitchClass TDD (note representation, semitone arithmetic, naming, MIDI conversion)
- [x] 02-03-PLAN.md — Chord TDD (construction, interval application, MIDI generation, naming for all 108 combinations)

### Phase 3: Playable Grid
**Goal**: User sees 32 labeled chord pads and can click any pad to hear that chord through their DAW instrument
**Depends on**: Phase 1, Phase 2
**Requirements**: GRID-01, GRID-02, MIDI-01, MIDI-02
**Success Criteria** (what must be TRUE):
  1. User sees an 8x4 grid of 32 labeled chord pads in the plugin window
  2. Grid displays a chromatic palette on first load (12 major, 12 minor, plus diminished/augmented/sus fills)
  3. Clicking any pad sends correct MIDI note-on/note-off messages to downstream instruments in Bitwig
  4. MIDI output uses proper velocity values and note durations
**Plans**: 2 plans

Plans:
- [x] 03-01-PLAN.md — Grid UI components: ChromaticPalette, PadComponent, GridPanel, PluginEditor integration
- [x] 03-02-PLAN.md — MIDI output pipeline: MidiKeyboardState in Processor, Timer-based note-off, pad-to-MIDI wiring

### Phase 4: Morphing Suggestions
**Goal**: After playing a chord, the grid morphs to show 32 harmonically related suggestions with smooth voice-led transitions
**Depends on**: Phase 2, Phase 3
**Requirements**: GRID-03, GRID-04, GRID-05, GRID-06, CHRD-03
**Success Criteria** (what must be TRUE):
  1. After clicking a pad, all 32 pads update to show contextually related chord suggestions
  2. Each pad displays a Roman numeral relative to the last-played chord (e.g., clicking C major makes F major show "IV")
  3. User can start from any chord and explore freely — no key or scale selection required
  4. Consecutive chord transitions use minimal note movement between voicings (voice leading)
  5. Suggestions blend music theory rules (diatonic, modal interchange) and harmonic proximity (common tones, voice-leading distance)
**Plans**: 3 plans

Plans:
- [x] 04-01-PLAN.md — Foundation engine classes (PitchClassSet, ScaleDatabase, VoiceLeader, RomanNumeral) + Catch2 tests
- [x] 04-02-PLAN.md — MorphEngine TDD: hybrid scoring algorithm (diatonic + common tones + voice leading), top-32 selection, variety filter
- [x] 04-03-PLAN.md — Integration: morph on pad click, Roman numeral display, voice-led MIDI, batch grid update

### Phase 5: Capture & Export
**Goal**: User can capture explored chord progressions and export individual chords to the DAW as MIDI clips
**Depends on**: Phase 3 (can run parallel with Phase 4)
**Requirements**: CAPT-01, CAPT-02, CAPT-03
**Success Criteria** (what must be TRUE):
  1. User can drag any individual pad to Bitwig and it appears as a MIDI clip in the arrangement
  2. A progression strip displays the sequence of chords the user has triggered (up to 8 chords)
  3. User can clear the progression strip and start a new exploration sequence
**Plans**: 3 plans

Plans:
- [x] 05-01-PLAN.md — MidiFileBuilder TDD: Chord-to-MIDI-file conversion with Catch2 tests
- [x] 05-02-PLAN.md — Drag-to-DAW with Linux DnD feasibility spike, click/drag disambiguation, file-export fallback
- [x] 05-03-PLAN.md — ProgressionStrip UI: horizontal chord sequence display (up to 8), clear button, wired to pad clicks

### Phase 6: State Persistence & Validation
**Goal**: Plugin state survives DAW session reload and meets production quality standards
**Depends on**: Phase 4, Phase 5
**Requirements**: PLAT-06, PLAT-07
**Success Criteria** (what must be TRUE):
  1. Closing and reopening a Bitwig session restores the plugin's grid state, morph state, and settings exactly
  2. Plugin passes pluginval validation at reasonable strictness level with no failures
  3. No audio dropouts or glitches during normal chord exploration workflow
**Plans**: 3 plans

Plans:
- [x] 06-01-PLAN.md — PersistentState struct with ValueTree serialization and round-trip unit tests
- [x] 06-02-PLAN.md — Editor↔Processor state wiring with change notification
- [x] 06-03-PLAN.md — pluginval validation at level 5, RT safety audit, Bitwig state persistence verification

### Phase 7: UX Polish & Progression Workflow
**Goal**: Progression strip becomes an intentional composition tool with drag-to-add, click-to-play, and MIDI file export — plus visual refinement across the plugin
**Depends on**: Phase 6
**Requirements**: UX-01, UX-02, UX-03, UX-04, UX-05
**Success Criteria** (what must be TRUE):
  1. Clicking a grid pad previews/plays the chord but does NOT auto-add to the progression strip
  2. Dragging a grid pad into the progression strip adds that chord to the sequence
  3. Clicking a chord in the progression strip sends MIDI note-on/note-off (plays the chord)
  4. A save/export button on the progression strip writes the current progression as a MIDI file to user-chosen location
  5. Plugin has visually polished UI with meaningful color differentiation
**Plans**: 3 plans

Plans:
- [x] 07-01-PLAN.md — Intra-plugin DnD + click/drag disambiguation (click plays, drag adds to strip)
- [x] 07-02-PLAN.md — Strip click-to-play + multi-chord MIDI export with async FileChooser
- [x] 07-03-PLAN.md — Visual polish with chord-type accent colours, gradient fills, refined styling

### Phase 8: Grid UX Overhaul
**Goal**: Refined grid interaction model with hold-to-preview, strip-driven morphing, harmonic similarity colour coding, and expanded 8×8 (64-pad) grid
**Depends on**: Phase 7
**Success Criteria** (what must be TRUE):
  1. Clicking a grid pad previews the chord with sustained note-on while held (note-off on mouse release)
  2. Grid does NOT morph on pad click — morphing only triggers when a chord is dragged into the progression strip
  3. Clicking a chord in the progression strip morphs the main grid to show suggestions for that chord
  4. Pad colours indicate harmonic similarity: blue (very similar) → green (good) → orange (bold) → purple (exotic) → red (extremely dissimilar)
  5. Grid is 8×8 (64 pads) while maintaining current individual pad size (plugin window grows to accommodate)
**Plans**: 3 plans

Plans:
- [ ] 08-01-PLAN.md — Hold-to-preview + strip-driven morph (decouple click from morph)
- [ ] 08-02-PLAN.md — 8×8 grid + engine expansion (64 pads, 7th chords, state v2 migration)
- [ ] 08-03-PLAN.md — Harmonic similarity colours (5-stop score gradient on pad borders)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 (parallel with 5) → 6 → 7 → 8

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Plugin Foundation | 3/3 | Complete | 2026-02-19 |
| 2. Chord Engine | 3/3 | Complete | 2026-02-19 |
| 3. Playable Grid | 2/2 | Complete | 2026-02-19 |
| 4. Morphing Suggestions | 3/3 | Complete | 2026-02-19 |
| 5. Capture & Export | 3/3 | Complete | 2026-02-19 |
| 6. State Persistence & Validation | 3/3 | Complete | 2026-02-20 |
| 7. UX Polish & Progression Workflow | 3/3 | Complete | 2026-02-20 |
| 8. Grid UX Overhaul | 0/3 | Planned | — |
