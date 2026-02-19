# Requirements: Chord-Grid

**Defined:** 2026-02-19
**Core Value:** The morphing grid — play a chord, see what comes next, follow the thread. A discovery engine for chord progressions, not a theory textbook.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Grid & Suggestions

- [x] **GRID-01**: User can see an 8x4 grid of 32 chord pads on the plugin UI
- [x] **GRID-02**: Grid starts with a chromatic palette (all 12 major, all 12 minor, plus diminished/augmented/sus fills) before any selection
- [ ] **GRID-03**: After user clicks a pad, grid morphs to show 32 contextually related chord suggestions
- [ ] **GRID-04**: Suggestions use a hybrid algorithm combining music theory rules (diatonic, modal interchange) and harmonic proximity (common tones, voice-leading distance)
- [ ] **GRID-05**: User can start exploring from any chord — no key or scale selection required
- [ ] **GRID-06**: Each pad displays a contextual Roman numeral relative to the last-played chord (e.g., clicking C major makes F major show as "IV")

### Chord Engine

- [x] **CHRD-01**: Plugin supports triads (major, minor, diminished, augmented) and 7th chords (maj7, min7, dom7, dim7, half-dim7)
- [x] **CHRD-02**: Each pad displays the chord name (e.g., "Dm7", "F#aug")
- [ ] **CHRD-03**: Chord transitions use smart voice leading that minimizes note movement between consecutive chords

### MIDI

- [x] **MIDI-01**: Clicking a pad sends that chord's MIDI notes to the plugin's MIDI output (downstream instruments in DAW)
- [x] **MIDI-02**: MIDI output includes proper note-on/note-off messages with configurable velocity

### Capture & Export

- [ ] **CAPT-01**: User can drag an individual chord from any pad to the DAW as a MIDI clip
- [ ] **CAPT-02**: A progression strip displays the sequence of chords the user has triggered (up to 8 chords)
- [ ] **CAPT-03**: User can clear the progression strip and start a new sequence

### Platform & Build

- [x] **PLAT-01**: Plugin builds and runs on Linux x86_64 (CachyOS)
- [x] **PLAT-02**: Plugin available as VST3 format, loadable in Bitwig Studio 5+
- [x] **PLAT-03**: Plugin available as CLAP format via clap-juce-extensions, loadable in Bitwig Studio 5+
- [x] **PLAT-04**: Plugin available as standalone application
- [x] **PLAT-05**: Build system uses CMake with juce_add_plugin
- [ ] **PLAT-06**: Plugin state saves and restores correctly when DAW session is reloaded
- [ ] **PLAT-07**: Plugin passes pluginval validation at reasonable strictness level

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Extended Interaction

- **MIDI-03**: User can trigger pads via MIDI keyboard input (note-to-pad mapping)
- **MIDI-04**: Configurable MIDI split point — low notes trigger pads, high notes pass through
- **CAPT-04**: User can drag the entire progression strip to DAW as a single MIDI clip

### Extended Chord Types

- **CHRD-04**: Sus2, sus4, add9, and 6th chord types available on pads
- **CHRD-05**: Extended voicings (9ths, 11ths, 13ths) via complexity control

### Enhanced Grid

- **GRID-07**: User-adjustable suggestion weights (more diatonic vs. more chromatic)
- **GRID-08**: Undo/redo for grid navigation (return to previous grid state)
- **GRID-09**: Grid bookmarks to save and recall grid states
- **GRID-10**: Multiple grid pages for different song sections

### Quality of Life

- **QOL-01**: Pad velocity sensitivity from MIDI input
- **QOL-02**: Transpose captured progression by semitones
- **QOL-03**: Named MIDI clip export (chord name in metadata)
- **QOL-04**: Simple preview sound for auditioning without external instrument

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Built-in sequencer / arranger | Massive scope — let the DAW handle arrangement. Scaler 3's timeline took years. |
| Built-in instrument hosting | Plugin-in-plugin is fragile, duplicates DAW functionality |
| Massive preset/progression library | Algorithm IS the preset — curated content creates decision paralysis |
| Audio chord detection | Separate product category entirely (DSP/ML) |
| Arpeggiator / strumming | Separate tool category — use Cthulhu or DAW arp downstream |
| Color-coded safety indicators | Contradicts no-fixed-key design philosophy |
| AI/ML-based generation | Black box undermines pedagogical angle — deterministic algorithm is more transparent |
| Cross-platform (macOS/Windows) | Linux-first, expand after validation |
| Complexity/tension slider | Hides decisions — explicit chord types are clearer than a 0-100% knob |
| MIDI passthrough | Creates trigger/passthrough ambiguity — use separate DAW tracks |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| GRID-01 | Phase 3 | Complete |
| GRID-02 | Phase 3 | Complete |
| GRID-03 | Phase 4 | Pending |
| GRID-04 | Phase 4 | Pending |
| GRID-05 | Phase 4 | Pending |
| GRID-06 | Phase 4 | Pending |
| CHRD-01 | Phase 2 | Complete |
| CHRD-02 | Phase 2 | Complete |
| CHRD-03 | Phase 4 | Pending |
| MIDI-01 | Phase 3 | Complete |
| MIDI-02 | Phase 3 | Complete |
| CAPT-01 | Phase 5 | Pending |
| CAPT-02 | Phase 5 | Pending |
| CAPT-03 | Phase 5 | Pending |
| PLAT-01 | Phase 1 | Complete |
| PLAT-02 | Phase 1 | Complete |
| PLAT-03 | Phase 1 | Complete |
| PLAT-04 | Phase 1 | Complete |
| PLAT-05 | Phase 1 | Complete |
| PLAT-06 | Phase 6 | Pending |
| PLAT-07 | Phase 6 | Pending |

**Coverage:**
- v1 requirements: 21 total
- Mapped to phases: 21 ✓
- Unmapped: 0

---
*Requirements defined: 2026-02-19*
*Last updated: 2026-02-19 after roadmap traceability mapping*
