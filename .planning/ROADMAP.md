# Roadmap: Chord-Grid

## Overview

Chord-Grid goes from empty project to shippable MIDI chord exploration plugin in 6 phases. We start with build infrastructure and plugin formats, then build the music theory engine as a testable core, wire up a playable grid with MIDI output, implement the morphing suggestion algorithm (the product's soul), add capture/export for the DAW workflow, and close with state persistence and validation. Phases 4 and 5 can run in parallel — the morph engine and capture workflow are independent once the playable grid exists.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Plugin Foundation** - Buildable VST3/CLAP/Standalone plugin that loads in Bitwig on Linux
- [ ] **Phase 2: Chord Engine** - Music theory primitives: chord construction, naming, and querying
- [ ] **Phase 3: Playable Grid** - 8x4 pad grid with chromatic palette, click-to-play via MIDI output
- [ ] **Phase 4: Morphing Suggestions** - Grid morphs after each chord with voice-led transitions and Roman numerals
- [ ] **Phase 5: Capture & Export** - Drag chords to DAW as MIDI clips, progression strip
- [ ] **Phase 6: State Persistence & Validation** - Session recall, pluginval, real-time safety verification

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
- [ ] 01-01-PLAN.md — Repository scaffold, git submodules (JUCE 8.0.12 + clap-juce-extensions), CMake build configuration
- [ ] 01-02-PLAN.md — Plugin source shell (Processor, Editor, LookAndFeel) and first build of all formats
- [ ] 01-03-PLAN.md — pluginval + clap-validator validation, Bitwig host loading verification

### Phase 2: Chord Engine
**Goal**: All v1 chord types can be constructed, voiced, and named programmatically with full test coverage
**Depends on**: Phase 1
**Requirements**: CHRD-01, CHRD-02
**Success Criteria** (what must be TRUE):
  1. All triads (major, minor, diminished, augmented) and 7th chords (maj7, min7, dom7, dim7, half-dim7) can be constructed from any root note
  2. Every constructed chord produces a correct human-readable name (e.g., "Dm7", "F#aug")
  3. Catch2 test suite covers all chord types, all 12 root notes, and enharmonic edge cases
**Plans**: TBD

Plans:
- [ ] 02-01: Interval arithmetic, note representation, and enharmonic handling
- [ ] 02-02: Chord construction (triads + 7ths) and chord name generation
- [ ] 02-03: Catch2 test suite for theory engine

### Phase 3: Playable Grid
**Goal**: User sees 32 labeled chord pads and can click any pad to hear that chord through their DAW instrument
**Depends on**: Phase 1, Phase 2
**Requirements**: GRID-01, GRID-02, MIDI-01, MIDI-02
**Success Criteria** (what must be TRUE):
  1. User sees an 8x4 grid of 32 labeled chord pads in the plugin window
  2. Grid displays a chromatic palette on first load (12 major, 12 minor, plus diminished/augmented/sus fills)
  3. Clicking any pad sends correct MIDI note-on/note-off messages to downstream instruments in Bitwig
  4. MIDI output uses proper velocity values and note durations
**Plans**: TBD

Plans:
- [ ] 03-01: Grid UI component (8x4 GridPanel with PadComponent, chord labels)
- [ ] 03-02: Chromatic palette population and pad-to-chord mapping
- [ ] 03-03: MIDI output pipeline (MidiRouter, ChordVoicer, note-on/note-off)
- [ ] 03-04: Lock-free audio/GUI thread wiring and Timer-based polling

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
**Plans**: TBD

Plans:
- [ ] 04-01: MorphEngine — hybrid scoring algorithm (diatonic + modal interchange + proximity)
- [ ] 04-02: Contextual Roman numeral calculation and display
- [ ] 04-03: Voice leading optimization for chord transitions
- [ ] 04-04: Integration — morph triggers on pad click, grid updates atomically

### Phase 5: Capture & Export
**Goal**: User can capture explored chord progressions and export individual chords to the DAW as MIDI clips
**Depends on**: Phase 3 (can run parallel with Phase 4)
**Requirements**: CAPT-01, CAPT-02, CAPT-03
**Success Criteria** (what must be TRUE):
  1. User can drag any individual pad to Bitwig and it appears as a MIDI clip in the arrangement
  2. A progression strip displays the sequence of chords the user has triggered (up to 8 chords)
  3. User can clear the progression strip and start a new exploration sequence
**Plans**: TBD

Plans:
- [ ] 05-01: MIDI file creation from chord data
- [ ] 05-02: Drag-to-DAW implementation with Linux DnD feasibility and fallback
- [ ] 05-03: Progression strip UI (display, clear, chord sequence tracking)

### Phase 6: State Persistence & Validation
**Goal**: Plugin state survives DAW session reload and meets production quality standards
**Depends on**: Phase 4, Phase 5
**Requirements**: PLAT-06, PLAT-07
**Success Criteria** (what must be TRUE):
  1. Closing and reopening a Bitwig session restores the plugin's grid state, morph state, and settings exactly
  2. Plugin passes pluginval validation at reasonable strictness level with no failures
  3. No audio dropouts or glitches during normal chord exploration workflow
**Plans**: TBD

Plans:
- [ ] 06-01: State serialization with ValueTree and version tagging
- [ ] 06-02: pluginval validation pass and real-time safety audit
- [ ] 06-03: Integration testing across VST3/CLAP/Standalone formats

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 (parallel with 5) → 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Plugin Foundation | 0/3 | Planned | - |
| 2. Chord Engine | 0/3 | Not started | - |
| 3. Playable Grid | 0/4 | Not started | - |
| 4. Morphing Suggestions | 0/4 | Not started | - |
| 5. Capture & Export | 0/3 | Not started | - |
| 6. State Persistence & Validation | 0/3 | Not started | - |
