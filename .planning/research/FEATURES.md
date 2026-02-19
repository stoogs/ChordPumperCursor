# Feature Research

**Domain:** MIDI Chord Exploration Plugins
**Researched:** 2026-02-19
**Confidence:** HIGH (multiple competitors analyzed, features verified across official sources)

## Feature Landscape

### Table Stakes (Users Expect These)

Features every chord exploration plugin ships. Missing any of these = users won't take Chord-Grid seriously.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Chord triggering from MIDI input** | Core purpose — press a key/pad, hear a chord. Ripchord, Cthulhu, InstaChord, Captain Chords all do this. | Low | Chord-Grid's pads serve this role. Map incoming MIDI notes to pad triggers. |
| **Chord name display** | Users must see what they're playing. Every competitor labels chords (e.g., "Cm7", "Fmaj"). | Low | Show chord name on each pad. Users learning theory rely on this heavily. |
| **Multiple chord types** | Triads alone feel toy-like. Users expect at least: major, minor, diminished, augmented, sus2, sus4, 7ths. Scaler offers dozens of voicing types. | Med | v1 scope: triads + 7ths. Add sus/dim/aug in v1.x. Diminished 7ths and half-diminished are common asks. |
| **Inversions / voicing control** | Smooth progressions need inversions. Captain Chords, Scaler, InstaChord all offer inversion selection. Root position only sounds robotic. | Med | Smart voice leading covers this implicitly. User override (force root position, specific inversion) is a v1.x consideration. |
| **MIDI output to external instruments** | Chord plugins are MIDI tools, not sound sources. ChordPotion, Cthulhu, Scaler all output MIDI. Users choose their own sounds. | Low | Already a design decision. MIDI-only output. |
| **MIDI drag-and-drop to DAW** | How users capture work. Scaler, Captain Chords, Ripchord all support dragging MIDI clips into DAW timeline. Without this, users must manually record. | Med | Drag individual chords or entire progressions. Scaler users complain about generic clip names — include chord name in clip metadata. |
| **Next-chord suggestions** | Scaler's "Suggest" mode, Captain Chords' "Magic" buttons. Users expect the plugin to recommend what comes next, not just display static chord lists. | High | This IS Chord-Grid's core mechanic (morphing grid). But suggestion quality is the differentiator, not the presence of suggestions. |
| **Scale/key awareness** | Captain Chords, Scaler, Chordjam, InstaChord — all start with key/scale selection. Users expect harmonic context. | Med | Chord-Grid's twist: no fixed key required. But the system must still understand scales internally for theory-based suggestions. |
| **Progression capture/accumulation** | Users build progressions chord-by-chord. They need somewhere to see "here's what I've selected so far." Scaler's Section C, Captain Chords' canvas. | Med | A progression strip/timeline showing the current 4-8 chord sequence being built, ready for drag-to-DAW. |

### Differentiators (Competitive Advantage)

Features that make Chord-Grid worth choosing over Scaler/Captain Chords/Cthulhu. These are either absent from competitors or executed poorly.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Morphing 8x4 chord grid** | No competitor uses a spatial grid that recomputes after each selection. Scaler shows linear lists. Captain Chords shows filtered chord buttons. GridChord (Drambo) has XY morphing but no contextual re-computation. The grid makes exploration feel like navigation, not menu-diving. | High | Core differentiator. Grid must recompute quickly (<50ms). Layout algorithm is the product's secret sauce. |
| **No fixed key required** | Every major competitor (Scaler, Captain Chords, Chordjam, InstaChord) starts with "select your key and scale." Chord-Grid starts from any chord and derives context dynamically. This matches how many musicians actually work — they find a chord they like and explore from there. | Med | Removes the biggest friction point in competitors. Users don't need to know they're "in D Dorian" to make music. |
| **Hybrid theory + proximity suggestions** | Scaler uses music-theory chord sets (diatonic, modal interchange). Chordjam uses randomization. Neither combines functional harmony with neo-Riemannian voice-leading proximity. The hybrid approach surfaces chords that sound good AND connect smoothly. | High | Combines: (1) diatonic relatives, (2) modal interchange / borrowed chords, (3) PLR-distance proximity (neo-Riemannian), (4) common-tone relationships. Weight each dimension. |
| **Contextual Roman numerals** | Scaler shows Roman numerals relative to a fixed key center. Chord-Grid shows them relative to the last-played chord, which is more useful for understanding harmonic movement ("this is a bVI relative to where I just was"). | Med | Unique pedagogical angle. Shows "how far" each option is from current position. Helps users build intuition about chord movement, not just key membership. |
| **Smart voice leading (automatic)** | Fluid Chords 2 has voice leading. Scaler 3 added auto voice leading. But neither integrates it into a suggestion engine — they apply it after chord selection. Chord-Grid's suggestions are pre-voiced for smooth transitions, so what you hear when you click a pad already sounds connected. | High | Voice leading must be computed for all 32 pads relative to the current chord. Performance matters. Cache aggressively. |
| **Linux native** | Scaler: no Linux. Captain Chords: no Linux. Cthulhu: no Linux. Chordjam: no Linux. InstaChord: no Linux. Ripchord: no Linux. The Linux DAW ecosystem (Bitwig, Ardour, Reaper, Qtractor) has zero dedicated chord exploration plugins. | Low | Built with JUCE 8, which supports Linux VST3/CLAP. This alone gives Chord-Grid a captive audience. Cross-platform is bonus. |
| **Chromatic palette starting point** | Competitors constrain to a key immediately. Starting chromatic (all 32 pads populated before any selection) lets users browse the full harmonic landscape first, then narrow organically as they make choices. | Med | First grid state = chromatic spread of useful chords. After first selection, grid morphs to show contextual suggestions. The transition from "open field" to "focused suggestions" is the core UX. |
| **Focused scope (exploration only)** | Scaler 3 became a DAW-within-a-DAW (sequencer, instrument hosting, multi-track arrangement). Captain Chords has 240+ built-in sounds. Chord-Grid does one thing: help you find 4-8 chords, drag them out, done. Simplicity is the feature. | Low | Anti-Scaler positioning. "Find chords fast, get out." No arrangement tools, no built-in sounds, no arpeggiator. The constraint IS the product. |

### Anti-Features (Commonly Requested, Often Problematic)

Features to deliberately NOT build. Each is present in competitors and either creates scope bloat, undermines Chord-Grid's design philosophy, or solves problems better handled by the DAW.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Built-in sequencer / arranger** | Scaler 3 has a full timeline with chord/melody/bass lanes. Users may expect similar. | Massive scope increase. Turns a focused exploration tool into a DAW competitor. Scaler 3's arrange page took years to build and is their v3 flagship. The DAW already has a timeline. | Progression strip for capture → drag entire progression to DAW. Let Bitwig/Reaper handle arrangement. |
| **Built-in instrument hosting** | Scaler 3 hosts VST/AU instruments internally. Captain Chords has 240+ built-in sounds. | Plugin-within-plugin hosting is fragile, adds enormous complexity, and duplicates DAW functionality. MIDI routing issues across DAWs (especially Maschine, FL Studio) are notorious. | MIDI output only. Users load instruments in their DAW. Include a simple sine/piano preview for auditioning (no latency-critical sound engine). |
| **Massive preset/progression library** | Scaler has 860+ chord sets. Captain Chords has 100+ famous progressions. "More presets" is an easy marketing bullet. | Users don't browse 860 presets — they use 5-10 and ignore the rest. Large libraries create decision paralysis. Chord-Grid's value is algorithmic suggestion, not curated content. | Seed the grid algorithm with a few "starting mood" presets (bright, dark, jazzy, tension) that set initial weights. The algorithm IS the preset. |
| **Audio chord detection** | Scaler Detector analyzes audio to identify chords/scales. Seems useful for "what key is this track in?" | Complex DSP (FFT, chromagram, ML classification). Separate product entirely. Scaler sells it as a $9 add-on for a reason. | Accept MIDI input to detect what the user is playing. If users want audio detection, point them to Scaler Detector or HoRNet SongKey. |
| **Arpeggiator / strumming engine** | Cthulhu's arp is "light years beyond usual" with polymetric sequencing. Chordjam has strumming. ChordPotion transforms chords into riffs. | Arpeggiators are a separate tool category. Adding one dilutes the exploration focus and competes with dedicated arp plugins users already own. | Output clean block chords via MIDI. Let users run output through Cthulhu, ChordPotion, or DAW arp for rhythmic treatment. |
| **Color-coded "safety" (in-key / out-of-key warnings)** | Scaler uses color to indicate diatonic vs. borrowed. Reason's Chord Sequencer color-codes suggestions. Seems helpful for beginners. | Chord-Grid's entire thesis is "no fixed key." Color-coding chords as "safe/dangerous" implies a fixed tonal center and discourages chromatic exploration. It contradicts the core design. | Use proximity/distance indicators instead: closer chords on the grid = smoother voice leading. Position IS the safety signal, not color. |
| **AI/ML-based chord generation** | Lemonaide, HookPad Aria, Notochord use trained models. "AI" is a marketing magnet in 2026. | Unpredictable outputs. Requires training data. Black-box behavior undermines the "understand what you're doing" pedagogical angle. Hard to debug when suggestions feel wrong. | Deterministic hybrid algorithm (theory + proximity). Users can reason about why a chord was suggested. Transparent > magical. |
| **Complexity/tension slider** | "Slide from simple triads to complex jazz voicings." Sounds intuitive. | Hides the actual decisions. What does "70% complexity" mean? It's a UX crutch that obscures the harmonic choices. Also hard to implement well — the mapping from scalar value to chord extensions is arbitrary. | Let users see triads and 7ths in v1. Future: show extensions as optional "add-ons" users can toggle per-pad (add 9th, add 13th). Explicit > implicit. |
| **MIDI passthrough (play notes through plugin)** | Some users want to play melodies through the plugin while it also triggers chords. | Creates ambiguity: is this note a pad trigger or a passthrough note? Splits the mental model. Every incoming MIDI note should mean "select this pad." | All incoming MIDI triggers pads (already a design decision). Users play melodies on a separate MIDI track in their DAW. |

## Feature Dependencies

```
Core MIDI I/O
├── Chord triggering from MIDI input
│   └── Pad-to-chord mapping
│       └── Chord name display
│       └── Multiple chord types (triads, 7ths)
│           └── Inversions / voicing control
│               └── Smart voice leading (automatic)
│
├── MIDI output to external instruments
│
└── MIDI drag-and-drop to DAW
    └── Progression capture strip
        └── Drag individual chord
        └── Drag full progression

Suggestion Engine (core algorithm)
├── Scale/key awareness (internal, not user-facing key lock)
│   └── Diatonic chord generation
│   └── Modal interchange / borrowed chord generation
│
├── Neo-Riemannian proximity calculation
│   └── PLR distance from current chord
│   └── Common-tone analysis
│
├── Hybrid scoring (theory weight + proximity weight)
│   └── Grid layout algorithm (place 32 best suggestions)
│       └── Morphing grid (recompute on each selection)
│           └── Contextual Roman numerals
│
└── Chromatic palette (initial state before first selection)

Voice Leading Engine
├── Current chord state tracking
├── Target chord voicing optimization
│   └── Minimize total voice movement
│   └── Avoid voice crossing
│   └── Maintain smooth contrary/oblique motion
└── Pre-compute voicings for all 32 pads
    └── Cache invalidation on chord change
```

## MVP Definition

### Launch With (v1)

These features define the minimum product that validates the core hypothesis: "a morphing grid finds chords faster than a list."

1. **8x4 grid with 32 chord pads** — the physical interface
2. **Chromatic palette initial state** — all pads populated before first selection
3. **Chord triggering from MIDI** — press key → play chord
4. **Chord name display on pads** — see what you're about to play
5. **Triads + 7ths** — major, minor, dim, aug, maj7, min7, dom7, dim7, half-dim7
6. **Morphing grid (recompute after each selection)** — core mechanic
7. **Hybrid suggestion algorithm** — theory + proximity scoring
8. **Smart voice leading** — automatic smooth transitions
9. **Contextual Roman numerals** — relative to last-played chord
10. **Progression capture strip** — see accumulated chords (4-8)
11. **MIDI drag-and-drop** — drag individual chords to DAW
12. **MIDI output** — route to external instruments
13. **Simple preview sound** — sine/triangle for auditioning without external instrument

### Add After Validation (v1.x)

Features that enhance the core experience once the morphing grid concept is validated.

| Feature | Rationale | Complexity |
|---------|-----------|------------|
| Drag full progression to DAW | Users will want to export all 4-8 chords as a single MIDI clip | Med |
| Sus2/sus4, add9, 6th chords | Expand palette beyond triads/7ths without overwhelming v1 | Med |
| User-adjustable suggestion weights | "Show me more borrowed chords" vs. "keep it diatonic" — tweak the algorithm | Med |
| Undo/redo on grid navigation | Go back to a previous grid state after exploring a dead end | Med |
| Grid "bookmarks" | Save a grid state to return to later — branching exploration | Med |
| Pad velocity sensitivity | Velocity of MIDI input affects output velocity | Low |
| Transpose selected progression | Shift entire captured progression up/down by semitones | Low |
| Named MIDI clip export | Include chord name in drag-and-drop clip metadata | Low |
| Multiple grid "pages" | Switch between grid states for different sections (verse/chorus) | Med |

### Future Consideration (v2+)

| Feature | Why Wait | Complexity |
|---------|----------|------------|
| Extended chord types (9ths, 11ths, 13ths, altered) | Exponentially increases grid population algorithm complexity. Validate core with simpler chords first. | High |
| MIDI input chord detection ("what chord am I playing?") | Useful but not core to exploration. Different interaction mode. | Med |
| Preset "moods" that bias the algorithm | Nice-to-have starting points. Algorithm should work well without them first. | Med |
| Cross-platform (macOS, Windows) | JUCE supports this natively. Linux-first validates with underserved market, then expand. | Med |
| Tonnetz visualization mode | Show the neo-Riemannian Tonnetz as an alternative grid layout. Educational + beautiful. | High |
| MPE output support | Expressive chord output for MPE-capable synths. Niche but growing. | High |
| OSC output | For modular/hardware integration. Niche. | Med |

## Feature Prioritization Matrix

| Feature | User Impact | Differentiation | Complexity | Priority |
|---------|------------|-----------------|------------|----------|
| Morphing 8x4 grid | Critical | Unique | High | **P0** |
| Hybrid suggestion algorithm | Critical | Unique | High | **P0** |
| Smart voice leading | High | Strong | High | **P0** |
| Chord triggering + MIDI out | Critical | Table stakes | Low | **P0** |
| Chord name display | High | Table stakes | Low | **P0** |
| Triads + 7ths | High | Table stakes | Med | **P0** |
| Contextual Roman numerals | Med | Unique | Med | **P0** |
| Progression capture strip | High | Table stakes | Med | **P0** |
| MIDI drag-and-drop (single chord) | High | Table stakes | Med | **P0** |
| Chromatic palette initial state | Med | Strong | Med | **P0** |
| Preview sound | Med | Table stakes | Low | **P0** |
| Drag full progression | High | Table stakes | Med | **P1** |
| Undo/redo grid navigation | Med | Nice-to-have | Med | **P1** |
| Sus/dim/aug extensions | Med | Table stakes | Med | **P1** |
| Adjustable suggestion weights | Med | Strong | Med | **P1** |
| Named MIDI export | Low | Nice-to-have | Low | **P2** |
| Grid bookmarks | Med | Unique | Med | **P2** |
| Tonnetz visualization | Low | Unique | High | **P3** |

## Competitor Feature Analysis

### Feature Matrix

| Feature | Scaler 3 | Captain Chords | Cthulhu | Chordjam | ChordPotion | Ripchord | InstaChord | **Chord-Grid** |
|---------|----------|----------------|---------|----------|-------------|----------|------------|----------------|
| **Price** | $99 | $99 | $39 | €49 | €49 | Free | $30 | TBD |
| **Linux support** | No | No | No | No | No | No | No | **Yes** |
| Key/scale selection | Yes | Yes | Yes | Yes | N/A | N/A | Yes | No (intentional) |
| Chord triggering | Keys | Keys | Keys | Keys/Pads | MIDI input | Keys | Keys | **Grid pads** |
| Next-chord suggestions | Yes (Suggest mode) | Yes (Magic buttons) | No | Randomization | No | No | No | **Yes (morphing grid)** |
| Voice leading | Yes (v3, auto) | Inversions only | No | Per-voice params | No | No | Basic inversions | **Yes (automatic, pre-computed)** |
| Roman numerals | Fixed key | No | No | No | No | No | No | **Contextual (relative)** |
| Progression capture | Section C | Canvas | No | Sequencer | Sequencer | Recording | Pattern editor | **Strip** |
| MIDI drag-and-drop | Yes | Yes | No | No | Export only | No | No | **Yes** |
| Built-in sounds | 33 instruments | 240+ instruments | No | No | Piano preview | No | No | **Preview only** |
| Arpeggiator | Yes | Rhythm presets | Yes (advanced) | Yes | Pattern transforms | No | Pattern editor | **No (intentional)** |
| Sequencer/arranger | Yes (full timeline) | Multi-workspace | No | Auto/manual | Pattern sequencer | No | Sequencer | **No (intentional)** |
| Preset library | 860+ chord sets | 100+ progressions | 150+ presets | Factory presets | Hundreds | Community packs | 60+ shapes | **Algorithm, not presets** |
| Audio detection | Yes ($9 add-on) | No | No | No | No | No | No | **No** |
| Grid/spatial interface | No (linear lists) | No (buttons) | No | XY pad | No | No | No | **Yes (8x4 morphing)** |
| Borrowed/modal chords | Yes (dedicated tab) | No | No | Via scale selection | No | No | No | **Yes (in algorithm)** |

### Competitive Positioning

**vs. Scaler 3:** Scaler is the Swiss Army knife — does everything, costs $99, no Linux. Chord-Grid is the scalpel — does one thing (exploration) better via spatial grid + no-key-required workflow. Target users: people who find Scaler overwhelming, Linux users, musicians who don't want to commit to a key upfront.

**vs. Captain Chords:** Captain Chords is instrument-first (240+ sounds) with chord building as secondary. Chord-Grid is exploration-first, MIDI-only. Different philosophies, minimal overlap.

**vs. Cthulhu:** Cthulhu is a chord memorizer + arpeggiator, not an exploration tool. It plays predefined chords, doesn't suggest new ones. Complementary, not competitive. Users could run Chord-Grid → Cthulhu for exploration → rhythmic treatment.

**vs. Chordjam:** Closest competitor in spirit — uses randomization for discovery. But Chordjam's randomization is undirected (stochastic), while Chord-Grid's morphing is directed (theory + proximity). Chordjam has no grid concept.

**vs. Ripchord:** Free preset player. No suggestion engine, no exploration. Different product category entirely.

**vs. Nothing (Linux):** This is the real competitive landscape. Linux DAW users (Bitwig, Ardour, Reaper) have ZERO dedicated chord exploration plugins. Chord-Grid has no competition on Linux.

## Sources

- Scaler 3 official site and user guide: https://scalermusic.com/products/scaler-3 (MEDIUM confidence — marketing material)
- Scaler 2 changelog: https://help.pluginboutique.com/hc/en-us/articles/6232885883412 (HIGH confidence — official changelog)
- Scaler forum feature requests: https://forum.scalermusic.com/ (MEDIUM confidence — user discussion)
- Captain Chords official: https://mixedinkey.com/captain-plugins/captain-chords/ (MEDIUM confidence — marketing)
- Captain Chords epic guide: https://mixedinkey.com/captain-epic-tutorials/captain-chords-epic-how-to-guide (HIGH confidence — official tutorial)
- ChordPotion manual: https://feelyoursound.com/chordpotion-manual/ (HIGH confidence — official docs)
- Cthulhu official: https://www.xferrecords.com/products/cthulhu (HIGH confidence — official)
- Ripchord GitHub: https://github.com/trackbout/ripchord (HIGH confidence — source code)
- InstaChord Plugin Boutique: https://www.pluginboutique.com/product/3-Studio-Tools/93-Music-Theory-Tools/4249-InstaChord (MEDIUM confidence)
- Chordjam official: https://audiomodern.com/shop/plugins/chordjam (MEDIUM confidence — marketing)
- Fluid Chords 2: https://pitchinnovations.com/blog/fluid-chords-chord-bending/ (MEDIUM confidence — blog post)
- Neo-Riemannian theory: https://en.wikipedia.org/wiki/Neo-Riemannian_theory (HIGH confidence — reference)
- music21 neo-Riemannian: https://music21.org/music21docs/moduleReference/moduleAnalysisNeoRiemannian.html (HIGH confidence — library docs)
- Chord embeddings research: https://dash.harvard.edu/bitstreams/7312037e-0c06-6bd4-e053-0100007fdf3b/download (HIGH confidence — academic)
- Scaler vs Captain Chords comparison: https://www.audiocipher.com/post/scaler-2-vs-captain-chords (MEDIUM confidence — third-party review)
- PluginErds 2026 comparison: https://pluginerds.com/10-best-chord-generator-plugins/ (LOW confidence — aggregator)
- GridChord (Drambo): https://patchstorage.com/gridchord-midi-rack (MEDIUM confidence — community)
- ChordPolyPad: https://dev.laurentcolson.com/chordpolypad.html (MEDIUM confidence — developer site)
