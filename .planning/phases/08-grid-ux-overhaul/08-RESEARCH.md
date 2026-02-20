# Phase 8: Grid UX Overhaul - Research

**Researched:** 2026-02-20
**Domain:** JUCE GUI interaction model, harmonic colour mapping, grid expansion
**Confidence:** HIGH

## Summary

This phase transforms the ChordPumper grid interaction model across four axes: (1) hold-to-preview replaces click-to-play+timer, (2) morph decouples from pad click and moves to strip-driven triggers, (3) pad colours shift from chord-type accent to harmonic-similarity gradient, and (4) the grid doubles from 8×4 to 8×8. All four changes are well-supported by the existing JUCE API surface and codebase architecture. The primary complexity lies in cleanly decoupling the currently-monolithic `padClicked` method and expanding every `32`-sized container to `64`.

The MorphEngine already produces composite scores (0.0–1.0) that map directly to harmonic similarity. The kAllChords pool of 108 candidates yields ~91 unique pitch-class sets after dedup, comfortably filling 64 slots. JUCE's `mouseDown`/`mouseUp` guarantee (mouseUp always fires on the originating component) makes hold-to-preview reliable.

**Primary recommendation:** Decompose `GridPanel::padClicked` into three orthogonal operations (preview, morph, strip-add) and template-parameterize all `32`-sized containers to `64` via a shared constant.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8 | GUI framework, MIDI, audio processing | Already in use; provides Component mouse API, MidiKeyboardState, juce::Grid layout |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::MidiKeyboardState | (part of JUCE 8) | Thread-safe noteOn/noteOff bridging GUI→audio | Hold-to-preview: noteOn on mouseDown, noteOff on mouseUp |
| juce::Grid | (part of JUCE 8) | CSS Grid-style layout | 8×8 pad arrangement |
| juce::Colour | (part of JUCE 8) | HSV/RGB interpolation | Harmonic similarity colour gradient |
| juce::DragAndDropContainer | (part of JUCE 8) | Intra-plugin drag-and-drop | Existing drag-to-strip unchanged |

No new libraries are needed. All functionality uses JUCE APIs already in the project.

## Architecture Patterns

### Pattern 1: Hold-to-Preview via mouseDown/mouseUp

**What:** Replace Timer-based 300ms note duration with sustained note-on while mouse is held.

**JUCE guarantee (from Component.h):**
> "A mouseUp callback is sent to the component in which a button was pressed even if the mouse is actually over a different component when the button is released."

This means mouseUp is ALWAYS delivered to the PadComponent that received mouseDown, even if the user drags outside. Safe for reliable note-off.

**Current flow (GridPanel::padClicked):**
```
mouseUp → onClick callback → padClicked:
  1. releaseCurrentChord()
  2. noteOn() for all voiced notes
  3. startTimer(300ms) → timerCallback → releaseCurrentChord
  4. onChordPlayed (strip capture)
  5. morph()
  6. persist state
```

**New flow — split into callbacks:**
```
PadComponent gains two callbacks:
  - onPressStart(const Chord&)   ← fired from mouseDown
  - onPressEnd(const Chord&)     ← fired from mouseUp (if not drag)

GridPanel wires them:
  - onPressStart: releaseCurrentChord(), voicing, noteOn() for all notes
  - onPressEnd: releaseCurrentChord()
  - No timer needed

Drag safety:
  - mouseDrag (>6px threshold): call onPressEnd immediately, set isDragInProgress
  - mouseUp: only call onPressEnd if !isDragInProgress
```

**Key detail — drag interrupts preview:**
When `isDragInProgress` becomes true in `mouseDrag`, notes MUST be released immediately. The user is now dragging to the strip and doesn't want the chord sustained. This is already partially handled by the existing 6px threshold guard.

**Code pattern:**
```cpp
// PadComponent.h — new callbacks
std::function<void(const Chord&)> onPressStart;  // noteOn
std::function<void(const Chord&)> onPressEnd;    // noteOff

// PadComponent.cpp
void PadComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu()) { /* existing right-click export */ return; }
    isPressed = true;
    isDragInProgress = false;
    repaint();
    if (onPressStart)
        onPressStart(chord);
}

void PadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 6) return;
    if (isDragInProgress) return;
    isDragInProgress = true;
    if (onPressEnd)
        onPressEnd(chord);  // release notes before drag
    if (auto* container = DragAndDropContainer::findParentDragContainerFor(this))
        container->startDragging(var(String(chord.name())), this, ScaledImage{}, false);
}

void PadComponent::mouseUp(const juce::MouseEvent& event)
{
    isPressed = false;
    repaint();
    if (!isDragInProgress)
    {
        if (onPressEnd)
            onPressEnd(chord);
    }
    isDragInProgress = false;
}
```

**GridPanel side:**
```cpp
// In constructor, for each pad:
pad->onPressStart = [this](const Chord& c) { startPreview(c); };
pad->onPressEnd   = [this](const Chord&)   { stopPreview(); };

void GridPanel::startPreview(const Chord& chord)
{
    releaseCurrentChord();
    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);
    for (auto note : voiced.midiNotes)
        keyboardState.noteOn(midiChannel, note, velocity);
    activeNotes.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
    // NO timer, NO morph, NO onChordPlayed
}

void GridPanel::stopPreview()
{
    releaseCurrentChord();
    // NO timer needed — notes released immediately
}
```

**Timer removal:** The `noteDurationMs` constant, `timerCallback`, `startTimer`, and `juce::Timer` inheritance on GridPanel can all be removed. The `stopTimer()` in `timerCallback` is no longer needed.

### Pattern 2: Strip-Driven Morphing

**What:** Morph triggers ONLY when (a) a chord is dragged into the progression strip, or (b) a chord in the strip is clicked.

**New public method on GridPanel:**
```cpp
void GridPanel::morphTo(const Chord& chord)
{
    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);
    auto suggestions = morphEngine.morph(chord, voiced.midiNotes);

    for (int i = 0; i < 64; ++i)
    {
        pads[i]->setChord(suggestions[i].chord);
        pads[i]->setRomanNumeral(suggestions[i].romanNumeral);
        pads[i]->setScore(suggestions[i].score);  // new: for colour
    }

    // Persist
    const juce::ScopedLock sl(stateLock);
    persistentState.lastPlayedChord = chord;
    persistentState.lastVoicing.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
    persistentState.hasMorphed = true;
    for (int i = 0; i < 64; ++i)
    {
        persistentState.gridChords[i] = suggestions[i].chord;
        persistentState.romanNumerals[i] = suggestions[i].romanNumeral;
    }
    repaint();
}
```

**Editor wiring:**
```cpp
// PluginEditor.cpp constructor

// Strip chord click → play + morph
progressionStrip.onChordClicked = [this](const Chord& c) {
    // Preview the chord (play it)
    auto& ks = processor.getKeyboardState();
    auto notes = c.midiNotes(4);
    for (auto n : notes) ks.noteOn(1, n, 0.8f);
    juce::Timer::callAfterDelay(300, [safeThis = SafePointer<ChordPumperEditor>(this), notes]() {
        if (!safeThis) return;
        auto& ks2 = safeThis->processor.getKeyboardState();
        for (auto n : notes) ks2.noteOff(1, n, 0.0f);
    });
    // Morph grid
    gridPanel.morphTo(c);
};

// Strip drop → morph (new callback on ProgressionStrip)
progressionStrip.onChordDropped = [this](const Chord& c) {
    gridPanel.morphTo(c);
};
```

**ProgressionStrip changes:**
```cpp
// New callback
std::function<void(const Chord&)> onChordDropped;

// In itemDropped:
void ProgressionStrip::itemDropped(const SourceDetails& details)
{
    if (auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get()))
    {
        const auto& chord = pad->getChord();
        addChord(chord);
        if (onChordDropped)
            onChordDropped(chord);  // trigger morph
    }
    isReceivingDrag = false;
    repaint();
}
```

**onChordPlayed removal from GridPanel:** The `onChordPlayed` callback on GridPanel was previously used to capture the clicked chord into the strip BEFORE morph. Since pad clicks no longer add to strip (only drag does), and pad clicks no longer morph, `onChordPlayed` can be removed from GridPanel entirely. The strip only receives chords via drag-and-drop.

### Pattern 3: Harmonic Similarity Colour Gradient

**What:** Map `ScoredChord.score` (0.0–1.0) to a 5-stop colour gradient indicating harmonic closeness.

**Colour stops (from phase spec):**
| Score Range | Colour | Meaning |
|-------------|--------|---------|
| 0.85–1.0 | Blue (#4A9EFF) | Very similar |
| 0.65–0.85 | Green (#4CAF50) | Good |
| 0.45–0.65 | Orange (#FF9800) | Bold choice |
| 0.25–0.45 | Purple (#9C27B0) | Exotic |
| 0.0–0.25 | Red (#F44336) | Extremely dissimilar |

**Implementation — interpolation between stops:**
```cpp
// In PadColours namespace (ChordPumperLookAndFeel.h)
inline juce::Colour similarityColour(float score)
{
    struct Stop { float pos; juce::Colour colour; };
    static const Stop stops[] = {
        { 0.00f, juce::Colour(0xffF44336) },  // red
        { 0.25f, juce::Colour(0xff9C27B0) },  // purple
        { 0.45f, juce::Colour(0xffFF9800) },  // orange
        { 0.65f, juce::Colour(0xff4CAF50) },  // green
        { 0.85f, juce::Colour(0xff4A9EFF) },  // blue
        { 1.00f, juce::Colour(0xff4A9EFF) },  // blue (clamp)
    };

    score = juce::jlimit(0.0f, 1.0f, score);
    for (int i = 0; i < 5; ++i)
    {
        if (score <= stops[i + 1].pos)
        {
            float t = (score - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            return stops[i].colour.interpolatedWith(stops[i + 1].colour, t);
        }
    }
    return stops[5].colour;
}
```

**PadComponent changes:**
- Add `float score_ = -1.0f;` member (negative = no score / chromatic palette)
- Add `void setScore(float s)` method
- In `paint()`, use `similarityColour(score_)` for the accent border when `score_ >= 0`, fall back to `accentForType(chord.type)` when `score_ < 0` (unmorphed state)

**Score range analysis (from MorphEngine):**
The composite score = `0.40 * diatonic + 0.25 * commonTones + 0.25 * voiceLeading`. Max = 0.40 * 1.0 + 0.25 * 1.0 + 0.25 * 1.0 = 0.90 (not 1.0, because remaining 10% weight is unaccounted — the weights sum to 0.90). This means the practical score range is roughly [0.0, 0.90]. The colour stops should be calibrated to this range, or the weights should be normalized to sum to 1.0.

**Weight normalization consideration:** Current weights sum to 0.90 (0.40 + 0.25 + 0.25). If we want score range [0, 1], the weights should sum to 1.0. Adding 0.10 to diatonic (making it 0.50) or distributing evenly would normalize. However, this changes morph ranking behaviour. Alternative: divide composite by weight sum (0.90) to normalize to [0, 1]. This preserves ranking order while giving a full colour range. **Recommend: normalize by dividing by weight sum.**

### Pattern 4: 8×8 Grid Expansion

**What:** Double grid from 32 to 64 pads. MorphEngine returns 64 suggestions. Window height grows.

**Constant propagation — define once:**
```cpp
// Shared constant (e.g., in a GridConstants.h or at namespace scope)
inline constexpr int kGridSize = 64;
inline constexpr int kGridCols = 8;
inline constexpr int kGridRows = 8;
```

All `32` literals in GridPanel, PersistentState, MorphEngine, and ChromaticPalette become `kGridSize`.

**MorphEngine changes:**
- Return type: `std::array<ScoredChord, 32>` → `std::array<ScoredChord, 64>`
- `kFinal` constant: `32` → `64`
- Pool size cap: `40` → `72` (or remove cap; 91 unique PCS < 108 total)
- Variety balancing: minimum per category from `2` → `4` (proportional)

**Pool feasibility:** kAllChords has 108 entries. After PitchClassSet dedup:
- 12 roots × 5 types (Major, Minor, Maj7, Min7, Dom7) = 60 unique PCS
- 12 Diminished triads = 12 unique PCS (each has distinct interval set)
- 12 HalfDim7 = 12 unique PCS
- Augmented triads: 4 unique PCS (C/E/Ab share, Db/F/A share, D/F#/Bb share, Eb/G/B share)
- Dim7: 3 unique PCS (C/Eb/F#/A share, Db/E/G/Bb share, D/F/Ab/B share)
- **Total: 60 + 12 + 12 + 4 + 3 = 91 unique PCS**

91 unique candidates → 64 selected = 70% coverage. Comfortable margin.

**Window sizing calculation:**
Current: `setSize(1000, 650)`, grid area = 980×534 (after reduce(10), top 40, bottom 56)
- Current pad: ~119×130 (width × height, with 4px gaps)
- Target: maintain ~130px pad height with 8 rows
- Grid height needed: 8 × 130 + 7 × 4 = 1068px
- Total window: 10 + 40 + 1068 + 6 + 50 + 10 = 1184px → round to **1200px**
- New: `setSize(1000, 1200)`

**GridPanel::resized() change:**
```cpp
// 8 rows instead of 4
for (int i = 0; i < kGridRows; ++i)
    grid.templateRows.add(Track(Fr(1)));
```

### Anti-Patterns to Avoid

- **Splitting noteOn/Off across threads:** Both MUST happen on the GUI thread via MidiKeyboardState. Never call noteOn from one thread and noteOff from another.
- **Leaving notes stuck on drag:** If drag starts after mouseDown (which has already sent noteOn), notes MUST be released in the mouseDrag handler before DnD begins.
- **Morphing on pad click:** The entire point of this phase is to decouple click from morph. Guard against accidentally re-adding morph in padClicked/onPressStart.
- **Hardcoding 32 in new code:** Use the shared `kGridSize` constant everywhere. Grep for remaining `32` literals after implementation.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Colour interpolation | Manual RGB lerp | `juce::Colour::interpolatedWith()` | Handles gamma, alpha correctly |
| Grid layout for 8×8 | Manual setBounds arithmetic | `juce::Grid` with 8 columns × 8 rows | Already used; CSS Grid handles gaps automatically |
| Note-on/off thread safety | Manual mutex around MIDI | `juce::MidiKeyboardState::noteOn/noteOff` | Already thread-safe, injects into processBlock buffer |
| Drag-and-drop detection | Manual mouse distance tracking | `juce::DragAndDropContainer::startDragging` | Already in use with 6px threshold |

**Key insight:** Every capability needed is already available in JUCE or the existing codebase. No new dependencies required.

## Common Pitfalls

### Pitfall 1: Stuck Notes on Drag
**What goes wrong:** User presses pad (noteOn fires), drags to strip, but noteOff never fires because drag path bypasses mouseUp logic.
**Why it happens:** mouseDown sends noteOn, mouseDrag starts DnD, and the code path that sends noteOff is guarded by `!isDragInProgress`.
**How to avoid:** In `mouseDrag`, when `isDragInProgress` first becomes true, immediately call `onPressEnd` to release notes BEFORE starting the drag operation.
**Warning signs:** Notes keep ringing after releasing mouse during drag.

### Pitfall 2: Score Range Mismatch
**What goes wrong:** All pads appear blue or all appear orange — poor colour differentiation.
**Why it happens:** The composite score from MorphEngine doesn't span the full [0, 1] range because weights sum to 0.90, and many chords cluster in the 0.3–0.7 range.
**How to avoid:** Normalize the composite score by dividing by the weight sum (0.90). Consider also applying a spread function (e.g., remap the actual min/max of the returned array to [0, 1]) for maximum visual contrast.
**Warning signs:** During testing, if most pads are the same colour, the mapping needs adjustment.

### Pitfall 3: State Version Backward Compatibility
**What goes wrong:** Loading a v1 state (32 chords) into v2 code (64 chords) crashes or shows empty pads.
**Why it happens:** `fromValueTree` loop expects 64 entries but v1 state only has 32.
**How to avoid:** Bump state version to 2. In `fromValueTree`, check version: if v1, load 32 chords into first 32 slots, fill remaining 32 with chromatic palette defaults. The existing range check (`if (index < 0 || index >= 32)`) needs to become `>= 64` for v2.
**Warning signs:** DAW session saved with old version crashes on load.

### Pitfall 4: Strip Click Plays AND Morphs Simultaneously
**What goes wrong:** Clicking a strip chord both plays the chord AND morphs the grid, but the play uses the old 300ms Timer pattern while morph recomputes voicing — potentially with stale activeNotes.
**Why it happens:** Strip click handler and morphTo both compute voicing independently.
**How to avoid:** Strip click handler should: (1) morph first (which sets activeNotes/voicing), (2) then play the chord using the voicing from morphTo. Or better: morphTo returns the voicing, and the editor plays using that voicing.
**Warning signs:** Strip click produces different voicing from what the grid assumes.

### Pitfall 5: ChromaticPalette Size Mismatch
**What goes wrong:** Compile error or runtime index out of bounds because ChromaticPalette returns 32 but GridPanel expects 64.
**Why it happens:** ChromaticPalette must grow from 32 to 64, but is easy to forget.
**How to avoid:** Change ChromaticPalette return type to `std::array<Chord, 64>` and add 32 more chords (7th chords). Update PersistentState constructor which initializes gridChords from chromaticPalette().
**Warning signs:** Compiler error on array size mismatch, or empty pads in bottom 4 rows.

### Pitfall 6: mouseExit Clearing isPressed During Hold
**What goes wrong:** User holds a pad, mouse drifts slightly outside pad bounds, and isPressed resets to false — visual glitch but notes keep playing.
**Why it happens:** Current mouseExit sets `isPressed = false`. But during a button-held state, JUCE does NOT call mouseExit (only mouseDrag). So this is actually safe — BUT if the user releases outside the pad, mouseUp fires first (on the original component), THEN mouseExit fires. The mouseExit `isPressed = false` is redundant but harmless.
**How to avoid:** Keep mouseExit as-is (it's safe). The JUCE docs confirm: "When the mouse button is pressed and held down while being moved in or out of a component, no mouseEnter or mouseExit callbacks are made."
**Warning signs:** None expected — this is a non-issue with JUCE's model.

## Code Examples

### MorphEngine 32→64 Change

```cpp
// MorphEngine.h — change return type
std::array<ScoredChord, 64> morph(const Chord& reference,
                                   const std::vector<int>& currentVoicing) const;

// MorphEngine.cpp — key changes
std::array<ScoredChord, 64> MorphEngine::morph(/* ... */) const {
    // ... (candidate generation unchanged) ...

    size_t poolSize = std::min(pool.size(), size_t(72));  // was 40
    constexpr size_t kFinal = 64;                          // was 32
    size_t selectEnd = std::min(poolSize, kFinal);

    // Variety: minimum 4 per category (was 2, proportional to 64/32)
    // ...

    std::array<ScoredChord, 64> result{};
    for (size_t i = 0; i < kFinal && i < selectEnd; ++i)
        result[i] = std::move(pool[i]);

    return result;
}
```

### ChromaticPalette Expansion to 64

```cpp
inline std::array<Chord, 64> chromaticPalette()
{
    using namespace pitches;
    using CT = ChordType;
    return {{
        // Rows 0–3: existing 32 (Major, Minor, Dim, Aug)
        {C, CT::Major}, {Cs, CT::Major}, {D, CT::Major}, {Eb, CT::Major},
        {E, CT::Major}, {F, CT::Major},  {Fs, CT::Major}, {G, CT::Major},
        {Ab, CT::Major}, {A, CT::Major}, {Bb, CT::Major}, {B, CT::Major},
        {C, CT::Minor},  {Cs, CT::Minor}, {D, CT::Minor}, {Eb, CT::Minor},
        {E, CT::Minor},  {F, CT::Minor},  {Fs, CT::Minor}, {G, CT::Minor},
        {Ab, CT::Minor}, {A, CT::Minor},  {Bb, CT::Minor}, {B, CT::Minor},
        {C, CT::Diminished}, {D, CT::Diminished}, {E, CT::Diminished}, {Fs, CT::Diminished},
        {C, CT::Augmented},  {Eb, CT::Augmented}, {G, CT::Augmented},  {Bb, CT::Augmented},
        // Row 4: Dominant 7th (most common 7th chord)
        {C, CT::Dom7}, {D, CT::Dom7}, {E, CT::Dom7}, {F, CT::Dom7},
        {G, CT::Dom7}, {A, CT::Dom7}, {Bb, CT::Dom7}, {B, CT::Dom7},
        // Row 5: Minor 7th
        {C, CT::Min7}, {D, CT::Min7}, {E, CT::Min7}, {F, CT::Min7},
        {G, CT::Min7}, {A, CT::Min7}, {Bb, CT::Min7}, {B, CT::Min7},
        // Row 6: Major 7th
        {C, CT::Maj7}, {D, CT::Maj7}, {E, CT::Maj7}, {F, CT::Maj7},
        {G, CT::Maj7}, {A, CT::Maj7}, {Bb, CT::Maj7}, {B, CT::Maj7},
        // Row 7: Half-diminished 7th (4) + Diminished 7th (4)
        {C, CT::HalfDim7}, {D, CT::HalfDim7}, {E, CT::HalfDim7}, {F, CT::HalfDim7},
        {C, CT::Dim7}, {Eb, CT::Dim7}, {Fs, CT::Dim7}, {A, CT::Dim7},
    }};
}
```

### PersistentState 32→64 with Version Migration

```cpp
struct PersistentState {
    std::array<Chord, 64> gridChords;       // was 32
    std::array<std::string, 64> romanNumerals; // was 32
    // ... rest unchanged ...
};

// In toValueTree: loop to 64 instead of 32, set version = 2
// In fromValueTree:
//   - If version == 1: load 32 pads, fill indices 32–63 with chromaticPalette()[32..63]
//   - If version == 2: load 64 pads
//   - Range check: index >= 64 instead of >= 32
```

### Window Size Adjustment

```cpp
// PluginEditor.cpp constructor
setSize(1000, 1200);  // was setSize(1000, 650)
```

## State of the Art

| Old Approach (Phase 7) | New Approach (Phase 8) | Impact |
|------------------------|------------------------|--------|
| Click = play + morph (coupled) | Click = preview only; strip = morph trigger | Clean separation of concerns |
| 300ms Timer note duration | Sustained while held (mouseDown→mouseUp) | More musical, tactile feel |
| Chord-type accent colours (9 fixed) | Harmonic similarity gradient (continuous) | Shows relationship to reference chord |
| 8×4 grid (32 pads) | 8×8 grid (64 pads) | More exploration, includes 7th chords |

## Open Questions

1. **Score normalization strategy**
   - What we know: Weights sum to 0.90, so max composite score is 0.90, not 1.0
   - What's unclear: Should we (a) normalize by dividing by weight sum, (b) adjust weights to sum to 1.0, or (c) use a dynamic min/max spread from the actual result set?
   - Recommendation: Option (a) — divide by weight sum. Preserves ranking, gives full [0, 1] range, minimal code change. If visual spread is still poor, apply option (c) as a secondary pass.

2. **Chromatic palette score display**
   - What we know: Before any morph, grid shows chromatic palette (no scores)
   - What's unclear: What colour should unmorphed pads use?
   - Recommendation: Use the existing chord-type accent colours for unmorphed state (score < 0 sentinel). This preserves the familiar look until the user starts morphing.

3. **Strip click voicing vs grid voicing**
   - What we know: Strip clicks currently play with `c.midiNotes(4)` (fixed octave 4). Grid uses `optimalVoicing()` with `activeNotes` context.
   - What's unclear: Should strip-click-triggered morph use optimal voicing or fixed octave?
   - Recommendation: Use `optimalVoicing()` for consistency. The morphed grid should reflect the same voicing context that the morph was computed with.

4. **Pool size cap with 64 slots**
   - What we know: After dedup, pool has ~91 entries. Selecting 64 out of 91.
   - What's unclear: Whether the variety post-filter (min per category) needs stronger enforcement with 64 slots, since most of the pool is selected anyway.
   - Recommendation: Keep variety filter but raise minimum to 4 per category. With 91→64 selection, there's less headroom for variety swaps, but the existing logic handles under-represented categories gracefully.

## Sources

### Primary (HIGH confidence)
- **juce::Component.h** (local source `libs/JUCE/modules/juce_gui_basics/components/juce_Component.h`) — mouseDown/mouseUp/mouseExit behaviour, drag semantics
- **juce::MidiKeyboardState.h** (local source `libs/JUCE/modules/juce_audio_basics/midi/juce_MidiKeyboardState.h`) — noteOn/noteOff API, thread safety guarantees
- **Existing codebase** — PadComponent.cpp, GridPanel.cpp, MorphEngine.cpp, PersistentState.cpp, ChromaticPalette.h, ChordPumperLookAndFeel.h, PluginEditor.cpp

### Secondary (MEDIUM confidence)
- **MorphEngine scoring analysis** — composite score range [0, 0.90] based on weight sum analysis of current code (weights = 0.40 + 0.25 + 0.25 = 0.90)
- **PitchClassSet dedup count** — 91 unique PCS calculated from combinatorial analysis of 12 roots × 9 types with known symmetries (aug/3, dim7/4)

## Metadata

**Confidence breakdown:**
- Hold-to-preview: HIGH — JUCE mouseDown/mouseUp guarantees verified in Component.h source
- Strip-driven morph: HIGH — straightforward callback rewiring, all interfaces exist
- Harmonic similarity colours: HIGH — juce::Colour::interpolatedWith is well-established; score mapping is a design decision more than a technical one
- Grid expansion 32→64: HIGH — MorphEngine pool (91 unique) comfortably exceeds 64; all 32-sized containers are known and enumerable
- State persistence migration: HIGH — version bump pattern is already used (v1 exists), well-understood
- Window sizing: MEDIUM — exact pixel calculation is sound but visual feel may need tuning after implementation

**Research date:** 2026-02-20
**Valid until:** 2026-03-20 (JUCE 8 stable, no API changes expected)
