# Phase 9: Chord Depth & UI Polish - Research

**Researched:** 2026-02-20
**Domain:** JUCE 8 C++ UI rendering, JUCE DnD, music theory chord extensions
**Confidence:** HIGH (all source files read directly, JUCE 8.0.12 modules read directly)

---

## Summary

Phase 9 adds three distinct features to the ChordPumper plugin: sub-variation quadrant splits within pads (7th/9th/11th/13th extensions), bolder/glowing pad borders, and drag-to-reorder within the progression strip. Each feature is technically independent but all touch PadComponent or ProgressionStrip.

The sub-variation quadrant split is the most architecturally significant change. It requires 9 new ChordTypes (Maj9, Maj11, Maj13, Min9, Min11, Min13, Dom9, Dom11, Dom13), expanding the kIntervals array from `[4]` to `[6]` per entry, updating kAllChords in PitchClassSet.h, and adding quadrant paint/hit-test logic to PadComponent. The sub-variations are derived locally in PadComponent from the base chord — no MorphEngine changes are needed for display, though kAllChords should also be extended so extension chords can appear as primary pads in future morphs.

The glow effect on hover is straightforward: JUCE 8 provides `GlowEffect` but it requires non-opaque components. The paint-based concentric-ring approach (drawing 3-4 translucent thick rounded rects at decreasing alpha) is simpler, more performant, and requires no opacity changes. Border thickness increase from 1.5f to 3.0f is a one-line change.

The progression strip drag-to-reorder requires adding `mouseDrag` and `itemDragMove` to ProgressionStrip. The strip detects when a drag starts from an existing slot, starts a JUCE DnD session with a "REORDER:N" description, tracks insertion index during move, draws an insertion cursor, and reorders the vector on drop. Existing pad-to-strip appending is unchanged.

State persistence stores ChordType as integer ordinal (`static_cast<int>(chord.type)`) — confirmed in PersistentState.cpp. New enum values 9-17 are fully forward-compatible with existing saved state. No state version bump needed.

**Primary recommendation:** Implement in order — (1) ChordType extension in ChordType.h + kAllChords in PitchClassSet.h, (2) pad quadrant paint/hit-test in PadComponent, (3) border glow in PadComponent::paint, (4) strip reorder in ProgressionStrip. Zero new dependencies.

---

## Standard Stack

### Core (already in project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_gui_basics | 8.0.12 | Component, paint, mouse events, DnD | Already used throughout |
| JUCE juce_graphics | 8.0.12 | GlowEffect, DropShadow, ColourGradient | Already linked |
| C++ std | C++17 | vector, array, algorithm | Already in use |

### No New Dependencies
All required JUCE APIs are already linked. Zero new CMake changes needed.

---

## Architecture Patterns

### Files to Modify (no new files needed)
```
src/engine/
├── ChordType.h          # ADD 9 new ChordType enum values, extend kIntervals to [6], update kChordSuffix, update noteCount()
├── PitchClassSet.h      # UPDATE allChords() to include 9 new types (currently hardcodes 9-element types array)
src/ui/
├── PadComponent.h       # ADD: bool hasSubVariations, std::array<Chord,4> subChords, int pressedQuadrant
├── PadComponent.cpp     # ADD: quadrant paint (separator lines + labels), quadrant hit-test in mouseDown/mouseUp
├── ProgressionStrip.h   # ADD: int reorderDragFromIndex, int insertionIndex; ADD mouseDrag() declaration
├── ProgressionStrip.cpp # ADD: mouseDrag(), itemDragMove(), reorder logic in itemDropped(), cursor in paint()
├── ChordPumperLookAndFeel.h  # ADD 9 accent colour entries for extension types, extend accentForType() array
```

---

## Feature 1: Chord Sub-Variation Quadrants

### Music Theory: Which Chords Benefit from Extensions

**Show quadrant split (musically meaningful extensions):**

| Base Type | Quadrant 1 (TL) | Q2 (TR) | Q3 (BL) | Q4 (BR) |
|-----------|-----------------|---------|---------|---------|
| Major | Major | Maj7 | Maj9 | Maj13 |
| Minor | Minor | Min7 | Min9 | Min11 |
| Maj7 | Maj7 | Maj9 | Maj11 | Maj13 |
| Min7 | Min7 | Min9 | Min11 | Min13 |
| Dom7 | Dom7 | Dom9 | Dom11 | Dom13 |

**No quadrant split (extensions not musically useful or chord is symmetric):**

| Type | Why Not |
|------|---------|
| Diminished | Symmetrical — extensions loop back enharmonically to other Dim roots |
| Augmented | Symmetrical — Aug7 = Dom chord; augmented extensions are rarely idiomatic |
| HalfDim7 | m7b5 context-specific; adding b9 fundamentally changes the chord meaning |
| Dim7 | Fully symmetrical — any "extension" is another Dim7 root enharmonically |

**Decision rule:** Only Major, Minor, Maj7, Min7, Dom7 base types show quadrant splits.

### New ChordTypes Required

Current `ChordType.h` has 9 types (ordinals 0-8). Add 9 more (ordinals 9-17):

```cpp
// Source: /src/engine/ChordType.h — extend existing enum
enum class ChordType : uint8_t {
    Major, Minor, Diminished, Augmented,
    Maj7, Min7, Dom7, Dim7, HalfDim7,
    // Phase 9 additions:
    Maj9,   // 9
    Maj11,  // 10
    Maj13,  // 11
    Min9,   // 12
    Min11,  // 13
    Min13,  // 14
    Dom9,   // 15
    Dom11,  // 16
    Dom13,  // 17
};
```

**Intervals (semitones from root):**

| Type | Intervals | Note Count |
|------|-----------|------------|
| Maj9 | 0, 4, 7, 11, 14 | 5 |
| Maj11 | 0, 4, 7, 11, 14, 17 | 6 |
| Maj13 | 0, 4, 7, 11, 14, 21 | 6 |
| Min9 | 0, 3, 7, 10, 14 | 5 |
| Min11 | 0, 3, 7, 10, 14, 17 | 6 |
| Min13 | 0, 3, 7, 10, 14, 21 | 6 |
| Dom9 | 0, 4, 7, 10, 14 | 5 |
| Dom11 | 0, 4, 7, 10, 14, 17 | 6 |
| Dom13 | 0, 4, 7, 10, 14, 21 | 6 |

Note: Maj13 omits the 11th (17) from the voicing — this is standard practice (the #11 tension is typically avoided in maj13 without explicit #11 voicing intent). Similarly Min13 omits the 11th, Dom13 omits the 11th by default.

**kIntervals dimension change — CRITICAL:**

Current: `std::array<std::array<int, 4>, 9>`
Required: `std::array<std::array<int, 6>, 18>`

Existing sentinels (`-1`) continue to mark unused slots. Triad entries use `-1` in slots [3], [4], [5]; seventh entries use `-1` in [4] and [5]; ninth entries use `-1` in [5] only.

**Every site that reads kIntervals must be checked.** Confirmed sites from source scan:
- `ChordType.h` — definition
- `PitchClassSet.h` — `pitchClassSet()` iterates via `noteCount()`, safe if noteCount() is updated
- `Chord.cpp` — `midiNotes()` iterates via `noteCount()`, safe if noteCount() is updated

**noteCount() update required:**
```cpp
// Source: /src/engine/ChordType.h
inline constexpr int noteCount(ChordType type) {
    int t = static_cast<int>(type);
    if (t < 4)  return 3;  // triads: Major, Minor, Dim, Aug
    if (t < 9)  return 4;  // 7ths: Maj7 through HalfDim7
    // 9ths (Maj9=9, Min9=12, Dom9=15):
    if (t == 9 || t == 12 || t == 15) return 5;
    return 6;  // 11ths and 13ths
}
```

**kChordSuffix additions:**
```cpp
inline constexpr std::array<const char*, 18> kChordSuffix = {
    "", "m", "dim", "aug", "maj7", "m7", "7", "dim7", "m7b5",
    "maj9", "maj11", "maj13", "m9", "m11", "m13", "9", "11", "13"
};
```

### kAllChords in PitchClassSet.h — Must Be Extended

`kAllChords` is defined in `PitchClassSet.h` (NOT MorphEngine.cpp as initially suspected):

```cpp
// Current — PitchClassSet.h line 35-38:
constexpr std::array<ChordType, 9> types = {
    ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
    ChordType::Maj7, ChordType::Min7, ChordType::Dom7, ChordType::Dim7, ChordType::HalfDim7
};
std::array<Chord, 108> result{};  // 12 roots × 9 types = 108
```

**Must become:**
```cpp
constexpr std::array<ChordType, 18> types = {
    ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
    ChordType::Maj7, ChordType::Min7, ChordType::Dom7, ChordType::Dim7, ChordType::HalfDim7,
    ChordType::Maj9, ChordType::Maj11, ChordType::Maj13,
    ChordType::Min9, ChordType::Min11, ChordType::Min13,
    ChordType::Dom9, ChordType::Dom11, ChordType::Dom13
};
std::array<Chord, 216> result{};  // 12 roots × 18 types = 216
```

The `allChords()` return type and `kAllChords` type change from `std::array<Chord, 108>` to `std::array<Chord, 216>`. MorphEngine.cpp range-for loop `for (const auto& chord : kAllChords)` requires no change — it iterates the whole array regardless of size. The `all.reserve(108)` in MorphEngine should become `all.reserve(216)`.

### Quadrant Layout and Hit-Test in PadComponent

Pad dimensions at 1000×600 window: strip takes ~60px, grid gets ~540px height. 8 rows with 4px gaps: (540 - 7×4) / 8 ≈ 64px tall per pad. 8 cols with 4px gaps: (1000 - 7×4) / 8 ≈ 122px wide per pad. Each quadrant is ~61×32px — adequate for an 8pt label showing short suffix ("9", "11", "13").

**New fields in PadComponent:**
```cpp
// PadComponent.h additions:
bool hasSubVariations = false;
std::array<Chord, 4> subChords{};  // [0]=base, [1]=+7, [2]=+9/11, [3]=+13/11
int pressedQuadrant = 0;            // 0..3, set at mouseDown, used at mouseUp
```

**Static helper (in PadComponent.cpp, anonymous namespace):**
```cpp
static bool qualifiesForSubVariations(ChordType t) {
    return t == ChordType::Major || t == ChordType::Minor ||
           t == ChordType::Maj7  || t == ChordType::Min7  ||
           t == ChordType::Dom7;
}

static std::array<Chord, 4> buildSubVariations(const Chord& base) {
    using CT = ChordType;
    std::array<Chord, 4> result = {base, base, base, base};
    switch (base.type) {
        case CT::Major: result = {base, {base.root, CT::Maj7}, {base.root, CT::Maj9},  {base.root, CT::Maj13}}; break;
        case CT::Minor: result = {base, {base.root, CT::Min7}, {base.root, CT::Min9},  {base.root, CT::Min11}}; break;
        case CT::Maj7:  result = {base, {base.root, CT::Maj9}, {base.root, CT::Maj11}, {base.root, CT::Maj13}}; break;
        case CT::Min7:  result = {base, {base.root, CT::Min9}, {base.root, CT::Min11}, {base.root, CT::Min13}}; break;
        case CT::Dom7:  result = {base, {base.root, CT::Dom9}, {base.root, CT::Dom11}, {base.root, CT::Dom13}}; break;
        default: break;
    }
    return result;
}
```

**Updated setChord():**
```cpp
void PadComponent::setChord(const Chord& c) {
    chord = c;
    hasSubVariations = qualifiesForSubVariations(c.type);
    if (hasSubVariations)
        subChords = buildSubVariations(c);
    repaint();
}
```

**Quadrant hit-test helper:**
```cpp
int PadComponent::quadrantAt(juce::Point<int> pos) const {
    int q = 0;
    if (pos.getX() > getWidth() / 2)  q += 1;
    if (pos.getY() > getHeight() / 2) q += 2;
    return q;  // 0=TL, 1=TR, 2=BL, 3=BR
}
```

**Updated mouseDown/mouseUp (key change — store quadrant at press time):**
```cpp
void PadComponent::mouseDown(const juce::MouseEvent& event) {
    // ... existing popup menu check ...
    pressedQuadrant = hasSubVariations ? quadrantAt(event.getPosition()) : 0;
    const Chord& activeChord = hasSubVariations ? subChords[pressedQuadrant] : chord;
    isPressed = true;
    isDragInProgress = false;
    repaint();
    if (onPressStart)
        onPressStart(activeChord);
}

void PadComponent::mouseUp(const juce::MouseEvent& event) {
    isPressed = false;
    repaint();
    const Chord& activeChord = hasSubVariations ? subChords[pressedQuadrant] : chord;
    if (!isDragInProgress) {
        if (onPressEnd)
            onPressEnd(activeChord);
        if (event.getDistanceFromDragStart() < 6 && onClick)
            onClick(activeChord);
    }
    isDragInProgress = false;
}
```

**Quadrant paint (add to PadComponent::paint after background fill):**
```cpp
if (hasSubVariations) {
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    // Separator lines
    g.setColour(juce::Colour(0x28ffffff));
    g.drawLine(w * 0.5f, 2.0f, w * 0.5f, h - 2.0f, 0.5f);
    g.drawLine(2.0f, h * 0.5f, w - 2.0f, h * 0.5f, 0.5f);

    // Quadrant labels (tiny suffix only)
    g.setFont(juce::Font(juce::FontOptions(7.5f)));
    auto labelFor = [](const Chord& c) {
        // Show only the suffix part (e.g. "9", "11", "maj9")
        return juce::String(kChordSuffix[static_cast<int>(c.type)]);
    };
    int hw = getWidth() / 2, hh = getHeight() / 2;
    // Q1 (TL) — base chord, shown via main label below; just show suffix small
    g.setColour(juce::Colour(0xff707080));
    g.drawText(labelFor(subChords[0]), juce::Rectangle<int>(0, 0, hw, hh),
               juce::Justification::centred);
    g.drawText(labelFor(subChords[1]), juce::Rectangle<int>(hw, 0, hw, hh),
               juce::Justification::centred);
    g.drawText(labelFor(subChords[2]), juce::Rectangle<int>(0, hh, hw, hh),
               juce::Justification::centred);
    g.drawText(labelFor(subChords[3]), juce::Rectangle<int>(hw, hh, hw, hh),
               juce::Justification::centred);
}
// Then the root-name label in the centre (existing logic, slightly repositioned)
```

---

## Feature 2: Bolder Borders and Hover Glow

### Current Border State
```cpp
// Source: /src/ui/PadComponent.cpp line 48
g.drawRoundedRectangle(bounds, cornerSize, 1.5f);  // current weight = 1.5f
```

### JUCE GlowEffect — Why Not to Use It

`GlowEffect` in JUCE 8 (`juce_graphics/effects/juce_GlowEffect.h`) works by blurring the alpha channel of the component's rendered image. PadComponent fills its background fully opaque via `fillRoundedRectangle`. An opaque fill produces alpha=1.0 everywhere, leaving no transparent pixels for the blur to spread into. The glow would be invisible. Workaround (making component non-opaque) adds complexity and background bleed-through. **Do not use GlowEffect.**

### Approach: Paint-Based Concentric Rings

Draw multiple stacked translucent thick rounded rects inside the component bounds to simulate a glow. Drawn INSIDE the bounds to avoid clipping conflicts with adjacent pads.

```cpp
// Source: JUCE 8 Graphics::drawRoundedRectangle — verified in local JUCE source

void PadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    constexpr float cornerSize = 6.0f;

    // --- Background (existing) ---
    auto baseColour = isPressed ? juce::Colour(PadColours::pressed)
                    : isHovered ? juce::Colour(PadColours::hovered)
                                : juce::Colour(PadColours::background);
    auto gradient = juce::ColourGradient::vertical(
        baseColour.brighter(0.05f), baseColour.darker(0.05f), bounds);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);

    // --- Quadrant split (if applicable) ---
    // ... (see Feature 1 section) ...

    // --- Border + optional hover glow ---
    float accentAlpha = isPressed ? 0.85f : 0.45f;
    auto accentColour = (score_ >= 0.0f)
        ? PadColours::similarityColour(score_)
        : juce::Colour(PadColours::accentForType(chord.type));

    if (isHovered && !isPressed)
    {
        // Glow rings — drawn inside bounds, stacked from dim/wide to bright/narrow
        g.setColour(accentColour.withAlpha(0.07f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 9.0f);
        g.setColour(accentColour.withAlpha(0.13f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 6.0f);
        g.setColour(accentColour.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 4.0f);
        g.setColour(accentColour.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 3.0f);
    }
    else
    {
        g.setColour(accentColour.withAlpha(accentAlpha));
        g.drawRoundedRectangle(bounds, cornerSize, 3.0f);  // was 1.5f
    }

    // --- Text labels (existing, unchanged) ---
    // ...
}
```

### Accent Colours for New ChordTypes

`accentForType()` in `ChordPumperLookAndFeel.h` uses array indexing by `static_cast<int>(type)`. Add 9 entries (indices 9-17):

```cpp
// Extension types follow the colour family of their base:
// Maj9/Maj11/Maj13 → lighter blues (Maj7 family)
// Min9/Min11/Min13 → lighter purples (Min7 family)
// Dom9/Dom11/Dom13 → lighter teals (Dom7 family)
inline constexpr juce::uint32 maj9Accent  = 0xff7ec8e8;
inline constexpr juce::uint32 maj11Accent = 0xff9dd6ef;
inline constexpr juce::uint32 maj13Accent = 0xffb3e0f5;
inline constexpr juce::uint32 min9Accent  = 0xffcbb8e8;
inline constexpr juce::uint32 min11Accent = 0xffd8c8f0;
inline constexpr juce::uint32 min13Accent = 0xffe2d8f5;
inline constexpr juce::uint32 dom9Accent  = 0xff7dd0d0;
inline constexpr juce::uint32 dom11Accent = 0xff9adada;
inline constexpr juce::uint32 dom13Accent = 0xffb0e4e4;

// In accentForType() — extend constexpr array to 18 entries
```

---

## Feature 3: Progression Strip Drag-to-Reorder

### Current Strip Architecture (confirmed from source)
- `ProgressionStrip` inherits `DragAndDropTarget`
- `isInterestedInDragSource()` accepts only `PadComponent` source
- `itemDropped()` calls `addChord()` which appends (wraps at kMaxChords=8)
- `mouseDown()` fires `onChordClicked` — no `mouseDrag()` override exists
- `getChordIndexAtPosition()` already maps pixel X to slot index — reusable

### Required Changes to ProgressionStrip

**New header fields:**
```cpp
// ProgressionStrip.h — add to private section:
int reorderDragFromIndex = -1;
int insertionIndex = -1;
void mouseDrag(const juce::MouseEvent& event) override;
void itemDragMove(const SourceDetails& details) override;
```

### Drag Description Convention

Use a string prefix to distinguish internal reorder drags from pad drags. Pad drags use chord names as description ("Cmaj7", "Am" etc.) — these can never start with "REORDER:".

```cpp
// Starting the reorder drag in ProgressionStrip::mouseDrag:
void ProgressionStrip::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 10)  // 10px threshold (wider than pad's 6px)
        return;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (container->isDragAndDropActive())
            return;
    }
    int index = getChordIndexAtPosition(event.getMouseDownPosition());
    if (index < 0)
        return;

    reorderDragFromIndex = index;
    insertionIndex = -1;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        container->startDragging("REORDER:" + juce::String(index),
                                 this, juce::ScaledImage{}, false);
}
```

### Updated isInterestedInDragSource
```cpp
bool ProgressionStrip::isInterestedInDragSource(const SourceDetails& details)
{
    // Check description-based type FIRST (reorder drags from self)
    if (details.description.toString().startsWith("REORDER:"))
        return true;
    // Then check source component type (pad drops from grid)
    return dynamic_cast<PadComponent*>(details.sourceComponent.get()) != nullptr;
}
```

### itemDragMove — Insertion Tracking
```cpp
void ProgressionStrip::itemDragMove(const SourceDetails& details)
{
    if (!details.description.toString().startsWith("REORDER:"))
        return;

    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;

    int relX = details.localPosition.getX() - slotArea.getX();
    // Clamp to [0, chords.size()]
    int newInsert = juce::jlimit(0, static_cast<int>(chords.size()),
                                  relX / (slotWidth + 4));
    if (newInsert != insertionIndex)
    {
        insertionIndex = newInsert;
        repaint();
    }
}
```

### Updated itemDropped — Reorder vs. Append
```cpp
void ProgressionStrip::itemDropped(const SourceDetails& details)
{
    if (details.description.toString().startsWith("REORDER:"))
    {
        int fromIdx = details.description.toString()
                          .fromFirstOccurrenceOf(":", false, false).getIntValue();
        int toIdx = (insertionIndex >= 0) ? insertionIndex
                                          : static_cast<int>(chords.size());

        if (fromIdx >= 0 && fromIdx < static_cast<int>(chords.size()) && fromIdx != toIdx)
        {
            auto chord = chords[static_cast<size_t>(fromIdx)];
            chords.erase(chords.begin() + fromIdx);
            // Adjust target index if erasing before it shifted the position
            if (toIdx > fromIdx) toIdx--;
            toIdx = juce::jlimit(0, static_cast<int>(chords.size()), toIdx);
            chords.insert(chords.begin() + toIdx, chord);

            const juce::ScopedLock sl(stateLock);
            persistentState.progression = chords;
        }

        reorderDragFromIndex = -1;
        insertionIndex = -1;
        updateClearButton();
        updateExportButton();
        repaint();
        return;
    }

    // Existing pad-drop logic (append):
    if (auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get()))
    {
        addChord(pad->getChord());
        if (onChordDropped)
            onChordDropped(pad->getChord());
    }
    isReceivingDrag = false;
    reorderDragFromIndex = -1;
    insertionIndex = -1;
    repaint();
}
```

### Insertion Cursor in paint()
```cpp
// Add at end of ProgressionStrip::paint(), after drawing all slots:
if (insertionIndex >= 0 && reorderDragFromIndex >= 0)
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int cursorX = slotArea.getX() + insertionIndex * (slotWidth + 4) - 2;
    cursorX = juce::jlimit(slotArea.getX(), slotArea.getRight() - 3, cursorX);
    g.setColour(juce::Colour(0xff6c8ebf).withAlpha(0.9f));
    g.fillRect(cursorX, slotArea.getY() + 2, 3, slotArea.getHeight() - 4);
}
```

Also reset state on itemDragExit:
```cpp
void ProgressionStrip::itemDragExit(const SourceDetails& details)
{
    isReceivingDrag = false;
    if (details.description.toString().startsWith("REORDER:"))
    {
        reorderDragFromIndex = -1;
        insertionIndex = -1;
    }
    repaint();
}
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Blur/glow effect | Custom convolution kernel | Stacked translucent drawRoundedRectangle calls | GlowEffect requires non-opaque component; rings are simpler and tuneable |
| Chord interval math | Custom music theory library | Extend kIntervals/kChordSuffix in existing ChordType.h | All interval infrastructure already exists |
| Drag description parsing | XML/JSON serialization | "REORDER:N" string prefix convention | Sufficient for the use case; matches existing `juce::String(chord.name())` pattern |
| Insertion point HUD | Separate overlay Component | Paint directly in ProgressionStrip::paint() | No extra component lifetime to manage |
| Sub-variation chord generation | Storing 4× chords per pad in persistent state | Derive in PadComponent from base chord type | Pure function; no state or engine changes needed |

**Key insight:** All hard infrastructure (DnD container hierarchy, chord data model, MIDI note generation, paint system, state persistence) already works. Phase 9 extends what exists.

---

## Common Pitfalls

### Pitfall 1: kIntervals Array Dimension Expansion
**What goes wrong:** Changing `std::array<int, 4>` to `std::array<int, 6>` is a breaking change at all initialisation sites.
**Why it happens:** C++ aggregate initialisation requires matching dimension. Mismatches are compile errors — good, they are caught — but multiple files reference `kIntervals`.
**How to avoid:** Sites confirmed by source read: ChordType.h (definition), PitchClassSet.h (`pitchClassSet()` — safe, uses `noteCount()`), Chord.cpp (`midiNotes()` — safe, uses `noteCount()`). All three update atomically. Run `grep -r "kIntervals"` before submitting.
**Warning signs:** Compile errors across ChordType.h, PitchClassSet.h, Chord.cpp.

### Pitfall 2: kAllChords Size Change Requires allChords() Return Type Update
**What goes wrong:** `allChords()` returns `std::array<Chord, 108>`. Adding 9 types × 12 roots = 108 more chords requires `std::array<Chord, 216>`. The `kAllChords` constexpr deduced type changes automatically, but explicit size annotations in MorphEngine (e.g., `all.reserve(108)`) need updating.
**Why it happens:** Size is baked into the return type.
**How to avoid:** Update `allChords()` return type and the `result{}` initialiser simultaneously. Update `all.reserve(108)` to `all.reserve(216)` in MorphEngine.cpp.
**Warning signs:** Compile error in PitchClassSet.h if result size is wrong; reserve() is a runtime issue (no crash, just reallocation).

### Pitfall 3: Quadrant Click Intercepting the Wrong Preview Chord
**What goes wrong:** mouseDown fires onPressStart with the wrong quadrant's chord if pressedQuadrant is not stored — e.g., user presses top-left but the chord for top-right plays.
**Why it happens:** Without storing which quadrant was pressed, mouseUp can re-evaluate quadrant from release position which may differ from press position.
**How to avoid:** Store `pressedQuadrant` in `mouseDown`. Use it in `mouseUp` and `mouseDrag`. Never re-evaluate quadrant from release event.
**Warning signs:** Wrong note sounds on press-and-hold.

### Pitfall 4: Strip Reorder Drag Threshold Too Tight
**What goes wrong:** Setting drag threshold to 6px in ProgressionStrip::mouseDrag means incidental mouse movement during a click starts a reorder drag, preventing the click from firing `onChordClicked` (grid morph).
**Why it happens:** Strip slots are wide — users expect clicks on them to be reliable.
**How to avoid:** Use 10px threshold for strip drags. Keep 6px for pad drags (smaller targets).
**Warning signs:** Clicking strip chords intermittently fails to morph the grid.

### Pitfall 5: GlowEffect Invisible on Opaque Component
**What goes wrong:** Adding `setComponentEffect(new GlowEffect(...))` produces no visible change.
**Why it happens:** GlowEffect blurs the alpha channel. PadComponent fills with `fillRoundedRectangle` (alpha=1 everywhere). No transparent pixels = no glow halo.
**How to avoid:** Use paint-based stacked rings. Never attempt GlowEffect on a solid-filled component.
**Warning signs:** Component appearance unchanged after GlowEffect is attached.

### Pitfall 6: State Forward Compatibility with New ChordTypes
**What goes wrong:** Old plugin state (saved with 9 types) loads fine; new state (with types 9-17) loaded by old plugin version would cast unknown integers back to ChordType, producing garbage chords.
**Why it happens:** Enum cast from int has no bounds checking.
**How to avoid:** This is acceptable for a development plugin (no public release). For production: bump kCurrentStateVersion from 2 to 3 and add migration logic. For this phase: document the incompatibility — old state with new types is NOT loadable by pre-phase-9 builds. State format stays as integer — this is the right format (confirmed: `static_cast<int>(chord.type)`).
**Warning signs:** Chords display as silence or wrong notes after downgrading plugin version.

### Pitfall 7: Reorder Index Offset After Erase
**What goes wrong:** After erasing the dragged chord from position `fromIdx`, the target insertion `toIdx` may be off by one if `toIdx > fromIdx`.
**Why it happens:** Erasing element `fromIdx` shifts all subsequent indices down by one.
**How to avoid:** After erase: if `toIdx > fromIdx`, decrement `toIdx` by 1 before inserting. Already accounted for in the code example above.
**Warning signs:** Dropped chord lands one position to the right of the cursor.

---

## Code Examples

### kIntervals full extended table
```cpp
// Source: /src/engine/ChordType.h — complete replacement
inline constexpr std::array<std::array<int, 6>, 18> kIntervals = {{
    {0, 4, 7,  -1, -1, -1},  // Major
    {0, 3, 7,  -1, -1, -1},  // Minor
    {0, 3, 6,  -1, -1, -1},  // Diminished
    {0, 4, 8,  -1, -1, -1},  // Augmented
    {0, 4, 7,  11, -1, -1},  // Maj7
    {0, 3, 7,  10, -1, -1},  // Min7
    {0, 4, 7,  10, -1, -1},  // Dom7
    {0, 3, 6,   9, -1, -1},  // Dim7
    {0, 3, 6,  10, -1, -1},  // HalfDim7
    {0, 4, 7,  11, 14, -1},  // Maj9
    {0, 4, 7,  11, 14, 17},  // Maj11
    {0, 4, 7,  11, 14, 21},  // Maj13
    {0, 3, 7,  10, 14, -1},  // Min9
    {0, 3, 7,  10, 14, 17},  // Min11
    {0, 3, 7,  10, 14, 21},  // Min13
    {0, 4, 7,  10, 14, -1},  // Dom9
    {0, 4, 7,  10, 14, 17},  // Dom11
    {0, 4, 7,  10, 14, 21},  // Dom13
}};
```

### allChords() in PitchClassSet.h — updated
```cpp
// Source: /src/engine/PitchClassSet.h — extend to 18 types
inline constexpr std::array<Chord, 216> allChords() {
    constexpr std::array<PitchClass, 12> roots = { /* unchanged */ };
    constexpr std::array<ChordType, 18> types = {
        ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
        ChordType::Maj7,  ChordType::Min7,  ChordType::Dom7,  ChordType::Dim7, ChordType::HalfDim7,
        ChordType::Maj9,  ChordType::Maj11, ChordType::Maj13,
        ChordType::Min9,  ChordType::Min11, ChordType::Min13,
        ChordType::Dom9,  ChordType::Dom11, ChordType::Dom13
    };
    std::array<Chord, 216> result{};
    int idx = 0;
    for (const auto& root : roots)
        for (auto type : types)
            result[static_cast<size_t>(idx++)] = Chord{root, type};
    return result;
}
inline constexpr auto kAllChords = allChords();
```

### Paint-based glow (PadComponent::paint)
```cpp
// Source: JUCE 8 Graphics::drawRoundedRectangle — verified in local JUCE 8.0.12 source
if (isHovered && !isPressed)
{
    g.setColour(accentColour.withAlpha(0.07f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 9.0f);
    g.setColour(accentColour.withAlpha(0.13f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 6.0f);
    g.setColour(accentColour.withAlpha(0.25f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 4.0f);
    g.setColour(accentColour.withAlpha(0.65f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 3.0f);
}
else
{
    g.setColour(accentColour.withAlpha(isPressed ? 0.85f : 0.45f));
    g.drawRoundedRectangle(bounds, cornerSize, 3.0f);  // was 1.5f
}
```

---

## State of the Art

| Old Approach | Current Approach | Impact for Phase 9 |
|--------------|------------------|-------------------|
| 9 ChordTypes (triads + 7ths) | 18 ChordTypes (adds 9th/11th/13th families) | kIntervals [4] → [6], kAllChords 108 → 216 |
| 1.5px border | 3px border + stacked glow rings on hover | Paint-only change; no layout impact |
| Append-only strip DnD | Reorder-capable strip DnD | New mouseDrag + itemDragMove on ProgressionStrip |
| No quadrant split in pads | 4-quadrant sub-variation display | Quadrant hit-test + sub-chord derivation in PadComponent |
| GlowEffect (don't use) | Paint-based glow simulation | No setComponentEffect needed |

**State persistence:** ChordType stored as `static_cast<int>(chord.type)` — confirmed in PersistentState.cpp. New ordinals 9-17 round-trip correctly. No version bump needed if only forward-compatible changes are made (old state with types 0-8 loads fine in new plugin).

---

## Open Questions

1. **Pad quadrant label readability at ~61×32px per quadrant**
   - What we know: Pad dimensions are approx 122×64px based on 1000×600 window minus strip and gaps. Each quadrant is ~61×32px.
   - What's unclear: Whether 7.5pt font renders "maj9" legibly at that size. "maj13" is 5 characters.
   - Recommendation: Use suffix-only labels in quadrants ("9", "11", "13", "") — shorter strings, larger effective font. Use 8pt. The main chord name stays in the top-left quadrant area.

2. **State version bump for ChordType ordinal expansion**
   - What we know: State stores ChordType as int. Old state (types 0-8) loads fine in new code. New state with types 9-17 loaded by old code produces `static_cast<ChordType>(9..17)` which is undefined enum behaviour in C++, but practically maps to garbage chord types.
   - What's unclear: Whether anyone has old saved state they must preserve across the phase.
   - Recommendation: For a dev plugin with no public release, no state migration needed. Document the forward-incompatibility clearly.

---

## Sources

### Primary (HIGH confidence)
- `/src/ui/PadComponent.cpp` — border drawing (1.5f), mouse event lifecycle, drag detection at 6px
- `/src/ui/PadComponent.h` — current fields: chord, romanNumeral_, score_, isPressed, isHovered, isDragInProgress
- `/src/ui/ProgressionStrip.cpp` — getChordIndexAtPosition() layout maths, itemDropped(), DnD flow
- `/src/ui/ProgressionStrip.h` — current fields, kMaxChords=8, onChordDropped callback
- `/src/engine/ChordType.h` — current 9 types, kIntervals as `[4]`, kChordSuffix, noteCount()
- `/src/engine/PitchClassSet.h` — kAllChords definition (NOT MorphEngine.cpp), allChords() hardcodes 9 types
- `/src/engine/MorphEngine.cpp` — `for (const auto& chord : kAllChords)` confirmed; `all.reserve(108)` identified
- `/src/PersistentState.cpp` — ChordType stored as `static_cast<int>(chord.type)` — integer ordinal confirmed
- `/src/ui/ChordPumperLookAndFeel.h` — accentForType() uses indexed array, 9 current entries
- `/libs/JUCE/modules/juce_graphics/effects/juce_GlowEffect.h` — GlowEffect API; non-opaque requirement confirmed
- `/libs/JUCE/modules/juce_gui_basics/mouse/juce_DragAndDropTarget.h` — itemDragMove() confirmed to exist as virtual method
- `JUCE VERSION 8.0.12` — confirmed from libs/JUCE/CMakeLists.txt

### Secondary (MEDIUM confidence)
- Music theory interval tables (Maj9=0,4,7,11,14 etc.) — standard equal-temperament convention
- Maj13 omitting 11th — standard jazz voicing practice (avoiding natural 11 tension in major context)

### Tertiary (LOW confidence)
- Quadrant label readability at actual screen pixels — requires visual validation post-implementation

---

## Metadata

**Confidence breakdown:**
- ChordType extension (intervals, names): HIGH — direct code read + standard music theory
- kAllChords location and structure: HIGH — found in PitchClassSet.h, confirmed
- State persistence format: HIGH — PersistentState.cpp read; integer ordinal confirmed
- Pad quadrant paint/hit-test: HIGH — all JUCE APIs verified in local JUCE source
- Border glow (paint rings): HIGH — drawRoundedRectangle API confirmed; visual tuning LOW
- GlowEffect non-opaque limitation: HIGH — confirmed from JUCE source
- Strip drag-to-reorder: HIGH — itemDragMove API confirmed; logic is straightforward extension of existing pattern
- String prefix drag description: HIGH — matches existing string-based DnD convention in project
- Quadrant label readability: LOW — pixel estimate, needs visual validation

**Research date:** 2026-02-20
**Valid until:** 2026-03-20 (JUCE 8.0.12 is stable; no fast-moving ecosystem dependencies)
