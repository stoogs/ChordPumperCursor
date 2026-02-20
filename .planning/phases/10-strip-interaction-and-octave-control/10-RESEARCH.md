# Phase 10: Strip Interaction & Octave Control - Research

**Researched:** 2026-02-20
**Domain:** JUCE C++ UI — DragAndDrop, mouse event handling, MIDI octave offsets, strip rendering
**Confidence:** HIGH (all findings from direct codebase inspection + JUCE API knowledge)

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| STRIP-01 | Dragging a chord from grid to strip deposits it at the slot under cursor (overwrite or insert at gap) | `itemDragMove` / `itemDropped` already compute `overwriteIndex` and `insertionIndex` from cursor X; bug is in hit-testing math |
| STRIP-02 | Drop visual indicators are centered accurately on the target slot during drag | Insertion line X calculation in `paint()` is off by one cell width; overwrite highlight already works |
| STRIP-03 | Right-clicking a slot in the progression strip clears/removes that chord | `mouseDown` checks `event.mods.isPopupMenu()` — just add erase + repaint there |
| STRIP-04 | Right-click on grid pad plays +1 octave, Shift+right-click plays -1 octave; octave offset persists into strip drag, shown as `+`/`-` | `PadComponent::mouseDown` already intercepts right-click (currently exports MIDI); needs replace + new `octaveOffset` field on `Chord` or carried via drag description |
| STRIP-05 | Strip chord slots display Roman numeral matching grid display | `ProgressionStrip::paint()` draws only `chord.name()`; needs second-line Roman numeral from stored `persistentState.romanNumerals` |
</phase_requirements>

---

## Summary

Phase 10 is entirely within existing code — no new libraries, no new JUCE subsystems. Every requirement maps to a small, targeted change in `ProgressionStrip`, `PadComponent`, `GridPanel`, and `PersistentState`.

The five requirements fall into three groups: (1) fixing the DnD hit-testing and visual feedback in `ProgressionStrip` (STRIP-01, STRIP-02), (2) adding right-click gestures in two components (STRIP-03 in `ProgressionStrip`, STRIP-04 in `PadComponent`), and (3) propagating Roman numeral text from state into the strip's paint pass (STRIP-05).

The most structurally interesting requirement is STRIP-04. Octave offset needs to be: (a) computed at the moment of right-click, (b) carried through the drag-and-drop description string, and (c) stored on the chord in the strip so it can be read back at playback time. Chord currently has no `octaveOffset` field — this must be added. The narrowest approach is adding `int octaveOffset = 0` to `Chord` and encoding it in the drag description string, e.g. `"OCTAVE:+1:Cmaj7"`.

**Primary recommendation:** Add `int octaveOffset = 0` to `Chord`, encode it in drag descriptions, fix strip hit-testing and insertion line math, add right-click handlers, and render Roman numerals in strip paint.

---

## Current Code Inventory (per requirement)

### STRIP-01 and STRIP-02: DnD hit-testing and insertion line

**What exists:**

`ProgressionStrip::itemDragMove()` (line 301–336) already computes both `overwriteIndex` and `insertionIndex`. The logic is:
- If cursor is inside an existing slot region AND not the slot being reordered → `overwriteIndex = cell`
- Otherwise → `insertionIndex = insertionIndexAtX(xPos)`

`insertionIndexAtX()` (line 236–252):
```cpp
int cell = relX / cellWidth;
int posInCell = relX % cellWidth;
int insertion = (posInCell > cellWidth / 2) ? cell + 1 : cell;
```
This is correct in isolation.

**The bug in `paint()` (line 419–423):**
```cpp
int cursorX = slotArea2.getX() + insertionIndex * cellWidth2 - 2;
g.fillRect(cursorX, slotArea2.getY() + 2, 4, slotArea2.getHeight() - 4);
```
`insertionIndex * cellWidth2` places the line at the *left edge of the cell* after the gap. The visual result is that the insertion cursor appears at the start of slot N rather than between slot N-1 and slot N (i.e., in the gap). The correct X is:
```cpp
// For insertion between slot i-1 and slot i:
// Left edge of slot i is: slotArea.getX() + i * (slotWidth + 4)
// Left edge of the gap before slot i is: that value - 4
// Centre of that gap is: that value - 2
int cursorX = slotArea2.getX() + insertionIndex * cellWidth2 - 2;
```
Actually the current formula looks approximately right but needs verification against actual slot layout. The `slotArea.getX()` offset needs to be included (it is), and `insertionIndex * cellWidth2` gives the X position of the left edge of slot `insertionIndex`. The gap is the 4px space just before that slot. So `- 2` centres within the gap. The formula appears correct for gaps *between* existing slots, but edge cases at index 0 and at `chords.size()` need to be verified — at index 0 there is no preceding gap, and at `chords.size()` the insertion is after the last slot.

**Hit-testing precision for STRIP-01:** `getChordIndexAtPosition()` (line 214–234) is used for mouse clicks but **not** for DnD hit detection — `itemDragMove()` recomputes position inline. The two code paths are inconsistent in how they handle the slot vs. gap boundary. This is the source of the "not always deposited at cursor" bug.

The fix: unify the hit-test logic by extracting a shared helper `slotAndGapAtX(int x)` that returns `{slotIndex, isInGap}`, and use it in both `getChordIndexAtPosition` and `itemDragMove`.

### STRIP-03: Right-click delete on strip slot

**What exists:** `ProgressionStrip::mouseDown()` (line 338–353) checks `isReceivingDrag` and drag-container state, then calls `getChordIndexAtPosition` and sets `pressedIndex`. It does NOT check for right-click.

**Where to add:** At the top of `mouseDown`, before the existing logic:
```cpp
if (event.mods.isPopupMenu())
{
    int idx = getChordIndexAtPosition(event.getPosition());
    if (idx >= 0 && idx < static_cast<int>(chords.size()))
    {
        chords.erase(chords.begin() + idx);
        // sync persistentState, updateClearButton, updateExportButton, repaint
    }
    return;
}
```

This mirrors the pattern in `PadComponent::mouseDown()` (line 141–159) which already intercepts `isPopupMenu()`.

### STRIP-04: Octave offset on grid pads

**What exists:**

`PadComponent::mouseDown()` right-click path (line 141–159) currently:
```cpp
if (event.mods.isPopupMenu())
{
    // exports MIDI file to directory
    isPressed = true; repaint();
    juce::Timer::callAfterDelay(200, ...);
    return;
}
```
This behavior needs to be **replaced** (not supplemented) with octave preview.

`GridPanel::startPreview()` (line 73–80):
```cpp
void GridPanel::startPreview(const Chord& chord)
{
    releaseCurrentChord();
    auto voiced = optimalVoicing(chord, activeNotes, defaultOctave);
    for (auto note : voiced.midiNotes)
        keyboardState.noteOn(midiChannel, note, velocity);
    activeNotes.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
}
```
The `defaultOctave = 4` (line 40). To preview at +1 octave, `defaultOctave + 1` must be passed. `optimalVoicing` takes octave as a parameter.

**Chord struct** (Chord.h):
```cpp
struct Chord {
    PitchClass root;
    ChordType type;
    // NO octaveOffset currently
};
```
`midiNotes(int octave)` takes octave as parameter — the offset must come from somewhere.

**Design decision required — two approaches:**

Option A: Add `int octaveOffset = 0` to `Chord`. Cleanest for persistence and strip display. Requires updating `toValueTree()`/`fromValueTree()`, and the strip paint to read `chord.octaveOffset`.

Option B: Encode offset in drag description string only, stored as an extra field on `ProgressionStrip` parallel to `chords`. More complex to maintain, breaks if strip is restored from state.

**Recommendation: Option A.** Add `int octaveOffset = 0` to `Chord`. The field is zero by default so all existing code is unaffected. Persistence handles it naturally. Strip can render `+` or `-` by checking `chord.octaveOffset`.

**Callback wiring:** `PadComponent` needs a way to signal octave-modified preview. Options:
- Add `onRightClickPressStart`/`onRightClickPressEnd` callbacks alongside the existing `onPressStart`/`onPressEnd`
- Or: pass a modifier (octave delta) through the existing callbacks by modifying the chord before passing

The simplest approach that minimises interface changes: `PadComponent` stores an `octaveOffset_` (set when right-click is detected in `mouseDown`) and `dragChord_` already captures the active chord. Modify `dragChord_.octaveOffset` before calling `startDragging`. The `onPressStart`/`onPressEnd` callbacks receive the chord — the octave delta needs to reach `GridPanel::startPreview`. Either:
1. `GridPanel` gains `onPressStartWithOctave(chord, delta)` callbacks — adds API surface
2. `PadComponent` includes octave in `dragChord_` via new field and `onPressStart` passes the chord with `octaveOffset` set — then `GridPanel::startPreview` reads `chord.octaveOffset` to offset its octave parameter

Option 2 is cleaner. `GridPanel::startPreview` becomes:
```cpp
void GridPanel::startPreview(const Chord& chord)
{
    releaseCurrentChord();
    int octave = defaultOctave + chord.octaveOffset;
    auto voiced = optimalVoicing(chord, activeNotes, octave);
    ...
}
```

**Shift+right-click detection in JUCE:**
```cpp
if (event.mods.isPopupMenu()) // right-click OR Ctrl+click
{
    bool shiftHeld = event.mods.isShiftDown();
    int octaveDelta = shiftHeld ? -1 : +1;
    ...
}
```
`isPopupMenu()` returns true for right-click on Linux. `isShiftDown()` checks shift modifier.

**Strip indicator:** In `ProgressionStrip::paint()`, after drawing chord name:
```cpp
if (chord.octaveOffset != 0)
{
    juce::String indicator = (chord.octaveOffset > 0) ? "+" : "-";
    g.setColour(juce::Colour(0xff88aaff)); // distinct colour from name
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(indicator, slot.removeFromTop(10), juce::Justification::centredTop);
}
```

### STRIP-05: Roman numeral in strip slots

**What exists:** Strip `paint()` draws only `chords[i].name()`. The strip stores `std::vector<Chord>` — no Roman numeral string stored alongside.

**Where Roman numerals live:** `persistentState.romanNumerals` (array of 64 strings) stores the Roman numeral for each *grid pad* position. The progression strip stores chords by value, copied from pads. There is no Roman numeral stored per progression chord.

**Design options:**

Option A: Store Roman numeral alongside each progression chord — add `std::string romanNumeral` to `Chord`, or store a `std::pair<Chord, std::string>` in the strip.

Option B: Recompute Roman numeral from `lastPlayedChord` at render time using `romanNumeral(persistentState.lastPlayedChord, chord)`.

Option B is simpler, has no storage cost, but produces the Roman numeral relative to the *current* morph context, which may not be the context the chord was captured under. This is likely acceptable since the grid display also updates relative to current context.

Option A with `std::string romanNumeral` directly on `Chord` is the cleanest solution and ensures the Roman numeral shown in the strip matches what was shown on the pad when the chord was captured. It also unifies the data model.

**Recommendation: Add `std::string romanNumeral` to `Chord`.** Then:
- When a pad is dragged to the strip, `dragChord_` includes the `romanNumeral` already set via `setRomanNumeral()`
- Strip's `paint()` reads `chords[i].romanNumeral` — if non-empty, draw second line
- Persistence serialises the field in `toValueTree()`

Alternatively, `ProgressionStrip` can hold a parallel `std::vector<std::string> romanNumerals_` matching `chords` index-for-index. This avoids modifying `Chord` but requires careful synchronisation on insert/erase/reorder.

**Narrowest approach:** Parallel vector in `ProgressionStrip`. Every operation that modifies `chords` (insert, erase, reorder, clear, setChords) also modifies the parallel vector. Persistence stores the Roman numeral on the progression chord element.

---

## Architecture Patterns

### DragAndDrop in JUCE

- `DragAndDropContainer::startDragging(var description, Component* source, ScaledImage, allowDraggingToOtherWindows)` — description is a `juce::var` (typically a `juce::String`)
- `DragAndDropTarget::itemDropped(SourceDetails)` — `details.sourceComponent` gives the originating component, `details.description` gives the var
- Drag image in `mouseDrag` is built by rendering to a `juce::Image` then wrapping in `juce::ScaledImage`
- `getLocalPoint(sourceComponent, localPosition)` converts `details.localPosition` (relative to source) into coordinates relative to the target — already used in `itemDragMove`

### Right-click detection in JUCE

```cpp
// In mouseDown:
if (event.mods.isPopupMenu())   // right-click on Linux, Ctrl+click on Mac
{
    bool shift = event.mods.isShiftDown();
    // handle
    return;
}
```
`isPopupMenu()` is the cross-platform idiom. Direct `isRightButtonDown()` also works on Linux.

### Mouse event modifiers

```cpp
event.mods.isShiftDown()     // Shift
event.mods.isCtrlDown()      // Ctrl
event.mods.isAltDown()       // Alt/Option
event.mods.isCommandDown()   // Cmd (macOS)
event.mods.isPopupMenu()     // right-click / Ctrl+click
```

### Slot geometry (existing, verified from code)

```
slotArea = fullBounds.removeFromLeft(fullBounds.getWidth() - 120)
slotWidth = (slotArea.getWidth() - (kMaxChords-1)*4) / kMaxChords
cellWidth = slotWidth + 4   // slot + gap
slot[i].x = slotArea.getX() + i * cellWidth
gap before slot[i]: x in [slot[i].x - 4, slot[i].x)
```

Insertion line at gap N (between slot N-1 and slot N):
```
lineX = slotArea.getX() + N * cellWidth - 2   // centre of 4px gap
```
For N=0 this gives `slotArea.getX() - 2` which is 2px left of the first slot — acceptable (before-all-slots drop).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Popup context menu for right-click | Custom overlay component | `juce::PopupMenu` | Built-in, handles keyboard/mouse dismissal |
| Octave shift playback | Custom note calculator | `chord.midiNotes(octave + delta)` | Already exists in `Chord::midiNotes(int)` |
| Roman numeral computation | Inline interval math | `romanNumeral(reference, chord)` in `RomanNumeral.cpp` | Already correct, handles tritone ambiguity |
| Drag image | Full component render | Manual `juce::Image` + `juce::Graphics` | Already done for reorder drag; same pattern for pad drag |

---

## Common Pitfalls

### Pitfall 1: Right-click consumed by popup menu before mouseDown
**What goes wrong:** On some platforms/hosts, `isPopupMenu()` triggers a system context menu before JUCE receives the event.
**How to avoid:** In JUCE, `mouseDown` always fires first with `isPopupMenu()` set. System menus are only shown if `getMouseClickGrabber()` delegates. No special handling needed on Linux.

### Pitfall 2: Chord vector and parallel Roman numeral vector going out of sync
**What goes wrong:** An insert, erase, or reorder on `chords` without the matching operation on `romanNumerals_` produces mismatched labels.
**How to avoid:** All mutation paths must touch both vectors atomically. The safest pattern is a helper method:
```cpp
void insertAt(int idx, const Chord& c, const std::string& rn);
void eraseAt(int idx);
void swapAt(int a, int b);
```
These helpers ensure the vectors stay in sync.

### Pitfall 3: Octave offset not cleared on strip playback
**What goes wrong:** Strip `onPressStart` in `PluginEditor.cpp` calls `c.midiNotes(4)` directly. If `Chord::octaveOffset` is added but strip playback ignores it, +/- pads play at the wrong octave.
**How to avoid:** Strip playback must use `c.midiNotes(4 + c.octaveOffset)`.

### Pitfall 4: Octave offset persisted but Roman numeral not (or vice versa)
**What goes wrong:** Partial persistence — `octaveOffset` saved but `romanNumeral` string not (or vice versa) — causes incorrect display after DAW session reload.
**How to avoid:** Both fields must be added to `toValueTree()` / `fromValueTree()` together.

### Pitfall 5: Drag description encoding breaks chord extraction
**What goes wrong:** If octave offset is encoded in the drag description string (e.g., `"OCTAVE:+1:Cmaj7"`), `ProgressionStrip::isInterestedInDragSource` currently uses `dynamic_cast<PadComponent*>` to identify pad drags. The description parse in `itemDropped` uses `pad->getDragChord()` — so the description string encoding is irrelevant as long as `dragChord_` has the correct `octaveOffset` set.
**How to avoid:** Set `dragChord_.octaveOffset` in `PadComponent::mouseDrag()` before calling `startDragging`. No description string changes needed.

### Pitfall 6: insertionIndex visual is drawn BEFORE the chord count is known
**What goes wrong:** The insertion line draw checks `insertionIndex <= static_cast<int>(chords.size())`. After a drop, `chords.size()` has increased but `insertionIndex` may still be valid from the last `itemDragMove`. The clear in `itemDropped` (`insertionIndex = -1`) happens after the repaint, so a brief flash may occur.
**How to avoid:** Already handled — `insertionIndex` is set to -1 before `repaint()` in `itemDropped`.

---

## Code Examples

### Erase at index in ProgressionStrip (STRIP-03 pattern)

```cpp
// In ProgressionStrip::mouseDown, before existing logic:
if (event.mods.isPopupMenu())
{
    int idx = getChordIndexAtPosition(event.getPosition());
    if (idx >= 0 && idx < static_cast<int>(chords.size()))
    {
        chords.erase(chords.begin() + idx);
        {
            const juce::ScopedLock sl(stateLock);
            persistentState.progression = chords;
        }
        updateClearButton();
        updateExportButton();
        repaint();
    }
    return;
}
```

### Octave offset right-click in PadComponent (STRIP-04 pattern)

```cpp
// Replace existing right-click export block in PadComponent::mouseDown:
if (event.mods.isPopupMenu())
{
    bool shiftHeld = event.mods.isShiftDown();
    int octaveDelta = shiftHeld ? -1 : +1;

    const Chord& baseChord = (hasSubVariations && pressedQuadrant >= 0)
        ? subChords[static_cast<size_t>(pressedQuadrant)]
        : chord;

    Chord offsetChord = baseChord;
    offsetChord.octaveOffset = octaveDelta;

    isPressed = true;
    repaint();
    if (onPressStart) onPressStart(offsetChord);
    // onPressEnd wired to mouseUp — store pending chord for release
    pendingRightClickChord_ = offsetChord;
    return;
}
```
`mouseUp` then calls `onPressEnd(pendingRightClickChord_)` when `isPopupMenu()` was the trigger.

### Roman numeral second line in strip paint (STRIP-05 pattern)

```cpp
// In ProgressionStrip::paint(), inside the existing-slot branch:
auto chordName = juce::String(chords[i].name());
auto roman = juce::String(chords[i].romanNumeral);  // new field

if (roman.isEmpty())
{
    g.setColour(juce::Colour(0xffe0e0e0));
    g.drawText(chordName, slot, juce::Justification::centred);
}
else
{
    auto top = slot.removeFromTop(slot.getHeight() / 2);
    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(chordName, top, juce::Justification::centredBottom);

    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(roman, slot, juce::Justification::centredTop);
}
```

### Octave-aware strip playback in PluginEditor (STRIP-04 playback)

```cpp
progressionStrip.onPressStart = [this](const Chord& c) {
    auto& ks = processor.getKeyboardState();
    auto notes = c.midiNotes(4 + c.octaveOffset);  // was: c.midiNotes(4)
    for (auto n : notes) ks.noteOn(1, n, 0.8f);
    stripActiveNotes = std::vector<int>(notes.begin(), notes.end());
};
```

---

## State of the Art

| Old Approach | Current Approach | Status |
|--------------|------------------|--------|
| Strip appends to end | Insert/overwrite at cursor (partially implemented) | Bug in visual feedback — fix needed |
| No right-click on strip | Right-click delete | New in Phase 10 |
| Right-click exports MIDI | Right-click previews +1 octave | Replace in Phase 10 |
| Strip shows chord name only | Strip shows chord name + Roman numeral | New in Phase 10 |

---

## Open Questions

1. **Should MIDI export on right-click be preserved?**
   - What we know: Currently right-click on a pad exports a MIDI file to `~/ChordPumper-Export/`. STRIP-04 replaces this with octave-up preview.
   - What's unclear: Whether the user wants to keep MIDI export accessible (e.g., via a different gesture).
   - Recommendation: Replace right-click with octave preview as specified. MIDI export via drag-to-DAW still works (left-drag). No user-facing loss.

2. **Roman numeral context for strip: captured-at or current morph?**
   - What we know: The grid computes Roman numerals relative to `lastPlayedChord`. Storing `romanNumeral` on `Chord` at capture time freezes the label relative to the chord that was active when the pad was dragged.
   - What's unclear: Whether the user wants Roman numerals to update when they morph to a new chord (so "vi" might become "I" if the morph target changes).
   - Recommendation: Freeze at capture time (store on Chord). This matches how the pads themselves work — the label is meaningful relative to the context in which the chord appeared. Dynamic updates would be confusing.

3. **PadComponent right-click press/release symmetry**
   - What we know: `mouseDown` handles right-click, but `mouseUp` does not currently differentiate right-click release. The `pendingRightClickChord_` approach described above adds state.
   - What's unclear: Whether right-click hold-to-preview is needed, or just a one-shot preview.
   - Recommendation: Match left-click hold-to-preview behaviour — right-click `mouseDown` fires `onPressStart(octaveChord)`, right-click `mouseUp` fires `onPressEnd`. This requires a `pendingRightClickChord_` member or an `octaveOverride_` int. The simpler approach: store `pendingOctaveOffset_` int in `PadComponent`, set on right-click `mouseDown`, used in `mouseUp` to build the release chord.

---

## Structural Changes Required

| File | Change |
|------|--------|
| `engine/Chord.h` | Add `int octaveOffset = 0;` and `std::string romanNumeral;` fields |
| `PersistentState.cpp` | Serialise `octaveOffset` and `romanNumeral` on progression chords in `toValueTree`/`fromValueTree` |
| `ui/PadComponent.h` | Add `int pendingOctaveOffset_ = 0;` private member |
| `ui/PadComponent.cpp` | Replace right-click MIDI export with octave-preview in `mouseDown`; apply offset in `mouseUp` and `mouseDrag` |
| `ui/ProgressionStrip.h` | No header changes needed (unless adding helper methods) |
| `ui/ProgressionStrip.cpp` | (1) Add right-click delete in `mouseDown`, (2) fix insertion line math if needed, (3) add Roman numeral second line in `paint()` |
| `ui/PluginEditor.cpp` | Update `onPressStart` lambda to use `c.midiNotes(4 + c.octaveOffset)` |

---

## Sources

### Primary (HIGH confidence)
- Direct codebase inspection: `ProgressionStrip.cpp`, `PadComponent.cpp`, `GridPanel.cpp`, `Chord.h`, `RomanNumeral.h/cpp`, `PersistentState.h/cpp`, `PluginEditor.cpp`
- JUCE API knowledge (MouseEvent modifiers, DragAndDropContainer/Target protocol) — corroborated by existing working code in the project

### Secondary (MEDIUM confidence)
- JUCE `isPopupMenu()` / modifier behaviour on Linux verified by existing working right-click in `PadComponent::mouseDown`

---

## Metadata

**Confidence breakdown:**
- STRIP-01 hit-testing bug: HIGH — code analysed, issue identified in `insertionIndexAtX` vs `getChordIndexAtPosition` inconsistency
- STRIP-02 insertion line math: HIGH — formula verified against slot geometry
- STRIP-03 right-click delete: HIGH — pattern identical to existing `PadComponent` right-click handler
- STRIP-04 octave offset design: HIGH — `Chord::midiNotes(int octave)` parameterised, `octaveOffset` field approach straightforward
- STRIP-05 Roman numeral in strip: HIGH — data available in `romanNumerals_` computed at morph time, storage design clear

**Research date:** 2026-02-20
**Valid until:** Stable — all findings from local source code
