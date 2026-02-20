---
phase: quick-3
plan: "003"
type: execute
wave: 1
depends_on: []
files_modified:
  - src/ui/ProgressionStrip.h
  - src/ui/ProgressionStrip.cpp
autonomous: true
requirements: [QUICK-03]

must_haves:
  truths:
    - "Dragging a pad chord over an existing strip slot highlights it and overwrites on drop"
    - "Dragging between slots or past the end shows an insertion line and inserts on drop"
    - "Reorder drags respect overwrite/insert logic the same way"
    - "Insertion line is bright white, 3-4px thick, clearly visible"
    - "Overwrite highlight is a bright white/yellow border on the target slot"
  artifacts:
    - path: "src/ui/ProgressionStrip.h"
      provides: "overwriteIndex member declaration"
      contains: "overwriteIndex"
    - path: "src/ui/ProgressionStrip.cpp"
      provides: "overwrite vs insert logic in itemDragMove, itemDropped, and paint"
      contains: "overwriteIndex"
  key_links:
    - from: "itemDragMove"
      to: "overwriteIndex / insertionIndex"
      via: "hit-test: cursor within slot boundary sets overwriteIndex, gap sets insertionIndex"
      pattern: "overwriteIndex"
    - from: "itemDropped"
      to: "chords vector"
      via: "overwriteIndex >= 0 means replace chords[overwriteIndex], else insert at insertionIndex"
      pattern: "overwriteIndex"
    - from: "paint"
      to: "slot border"
      via: "bright white/yellow drawRoundedRectangle when overwriteIndex == i"
      pattern: "overwriteIndex"
---

<objective>
Improve progression strip drag-and-drop UX with slot-aware overwrite and stronger visual feedback.

Purpose: Dragging currently always inserts. Users need to overwrite existing slots by dropping directly on them, and the insertion indicator is too subtle to notice.
Output: ProgressionStrip.h/.cpp with overwrite logic, bright insertion line, and overwrite highlight.
</objective>

<execution_context>
@/home/stoo/.claude/get-shit-done/workflows/execute-plan.md
@/home/stoo/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/STATE.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Add overwriteIndex and slot hit-test logic</name>
  <files>src/ui/ProgressionStrip.h
src/ui/ProgressionStrip.cpp</files>
  <action>
**In ProgressionStrip.h**, add `int overwriteIndex = -1;` alongside `insertionIndex` in the private section.

**In ProgressionStrip.cpp**, update `itemDragMove` to determine whether the cursor is directly over an existing slot (overwrite) or in a gap / past the end (insert):

Replace the single-line `itemDragMove` body with:

```cpp
void ProgressionStrip::itemDragMove(const SourceDetails& details)
{
    auto localPos = getLocalPoint(details.sourceComponent.get(), details.localPosition);
    int xPos = localPos.getX();

    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 120);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int relX = xPos - slotArea.getX();
    int cellWidth = slotWidth + 4;

    // Determine which cell (0..kMaxChords-1) the cursor is in
    int cell = relX / cellWidth;
    int posInCell = relX % cellWidth;
    bool inSlotRegion = (posInCell < slotWidth) && (relX >= 0) && (relX < slotArea.getWidth());
    bool isExistingSlot = inSlotRegion && (cell >= 0) && (cell < static_cast<int>(chords.size()));

    // For REORDER drags, don't overwrite the slot being dragged
    juce::String desc = details.description.toString();
    int reorderFrom = -1;
    if (desc.startsWith("REORDER:"))
        reorderFrom = desc.fromFirstOccurrenceOf(":", false, false).getIntValue();

    if (isExistingSlot && cell != reorderFrom)
    {
        overwriteIndex = cell;
        insertionIndex = -1;
    }
    else
    {
        overwriteIndex = -1;
        insertionIndex = insertionIndexAtX(xPos);
    }

    repaint();
}
```

Also reset `overwriteIndex = -1` alongside every existing `insertionIndex = -1` reset in:
- `itemDragExit`
- `itemDropped` (at the end of both branches)
</action>
  <verify>Code compiles: `cmake --build build-release --config Release -j$(nproc)` with no errors related to ProgressionStrip.</verify>
  <done>overwriteIndex is set when cursor is directly over an existing slot; insertionIndex is set when cursor is in a gap or past the end; both reset on drag exit and drop.</done>
</task>

<task type="auto">
  <name>Task 2: Wire overwrite into itemDropped and update paint</name>
  <files>src/ui/ProgressionStrip.cpp</files>
  <action>
**In `itemDropped`**, handle overwrite for both pad drops and reorder drops:

For the **pad drop branch** (after the `dynamic_cast<PadComponent*>` check), replace the `addChord(chord)` call with:

```cpp
if (overwriteIndex >= 0 && overwriteIndex < static_cast<int>(chords.size()))
{
    // Overwrite in place
    chords[static_cast<size_t>(overwriteIndex)] = chord;
    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression = chords;
    }
    updateClearButton();
    updateExportButton();
    if (onChordDropped)
        onChordDropped(chord);
}
else
{
    // Insert at insertionIndex if valid, otherwise append
    if (insertionIndex >= 0 && insertionIndex <= static_cast<int>(chords.size()))
    {
        if (chords.size() >= static_cast<size_t>(kMaxChords))
            chords.erase(chords.begin());
        int idx = juce::jlimit(0, static_cast<int>(chords.size()), insertionIndex);
        chords.insert(chords.begin() + idx, chord);
        {
            const juce::ScopedLock sl(stateLock);
            persistentState.progression = chords;
        }
        updateClearButton();
        updateExportButton();
        if (onChordDropped)
            onChordDropped(chord);
    }
    else
    {
        addChord(chord);
        if (onChordDropped)
            onChordDropped(chord);
    }
}
```

For the **REORDER branch**, change the drop target check: if `overwriteIndex >= 0` and it's not the source slot, swap the two chords instead of inserting:

```cpp
if (overwriteIndex >= 0 && overwriteIndex != fromIdx &&
    overwriteIndex < static_cast<int>(chords.size()))
{
    // Swap
    std::swap(chords[static_cast<size_t>(fromIdx)],
              chords[static_cast<size_t>(overwriteIndex)]);
    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression = chords;
    }
    updateClearButton();
    updateExportButton();
}
else if (toIdx >= 0 && toIdx <= static_cast<int>(chords.size()) &&
         toIdx != fromIdx && toIdx != fromIdx + 1)
{
    // Existing insert logic
    auto chord = chords[static_cast<size_t>(fromIdx)];
    chords.erase(chords.begin() + fromIdx);
    if (toIdx > fromIdx)
        toIdx--;
    chords.insert(chords.begin() + toIdx, chord);
    {
        const juce::ScopedLock sl(stateLock);
        persistentState.progression = chords;
    }
    updateClearButton();
    updateExportButton();
}
```
(Keep the existing `reorderDragFromIndex = -1; insertionIndex = -1; isReceivingDrag = false; repaint(); return;` cleanup at the end of the REORDER branch, and add `overwriteIndex = -1;` there too.)

**In `paint()`**, make two visual changes:

1. **Stronger insertion line** — replace the existing insertion line block:
```cpp
if (insertionIndex >= 0 && insertionIndex <= static_cast<int>(chords.size()))
{
    auto area2 = getLocalBounds();
    auto slotArea2 = area2.removeFromLeft(area2.getWidth() - 120);
    auto slotWidth2 = (slotArea2.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;
    int cellWidth2 = slotWidth2 + 4;
    int cursorX = slotArea2.getX() + insertionIndex * cellWidth2 - 2;
    g.setColour(juce::Colours::white);
    g.fillRect(cursorX, slotArea2.getY() + 2, 4, slotArea2.getHeight() - 4);
}
```

2. **Overwrite highlight** — inside the slot-drawing loop, after drawing the normal accent border, add an overwrite highlight when `i == overwriteIndex`:
```cpp
if (i == overwriteIndex)
{
    g.setColour(juce::Colour(0xffffff00).withAlpha(0.9f)); // bright yellow
    g.drawRoundedRectangle(slotF.reduced(0.5f), 4.0f, 2.5f);
}
```
Place this immediately after the existing `g.drawRoundedRectangle(slotF, 4.0f, 1.5f)` accent border line for filled slots.

Build and deploy: `cmake --build build-release --config Release -j$(nproc)`
</action>
  <verify>
1. Build succeeds with no errors.
2. In the plugin: drag a pad chord directly onto an existing strip slot — it overwrites (slot chord changes, count stays same).
3. Drag a pad chord between two slots — thin insertion line appears as a thick bright white vertical bar; chord inserts at that gap.
4. Drag a strip slot onto another strip slot — the two chords swap.
5. Overwrite target slot shows a bright yellow border during hover.
</verify>
  <done>Pad drops overwrite or insert correctly based on cursor position. Reorder drags swap or reorder correctly. Insertion line is bright white 4px. Overwrite slot shows bright yellow border on hover.</done>
</task>

</tasks>

<verification>
After build:
- Load plugin in DAW or standalone
- Populate the strip with 3-4 chords
- Drag a pad chord directly over slot 2 — slot 2 changes to the new chord, total count unchanged
- Drag a pad chord into the gap between slot 1 and slot 2 — bright white vertical line appears, chord inserts between them
- Drag slot 1 onto slot 3 — they swap
- The insertion line is visibly thick white (not the old thin blue)
- The overwrite slot has a yellow border during hover
</verification>

<success_criteria>
- Overwrite mode: dropping on existing slot replaces it, strip length unchanged
- Insert mode: dropping in a gap or past the end inserts, strip grows (capped at 8)
- Reorder: drop on another slot = swap; drop in gap = move to that position
- Visual: 4px white insertion line; bright yellow overwrite border
- No regressions: hold-to-play, click-to-morph, export still work
</success_criteria>

<output>
After completion, create `.planning/quick/3-strip-slot-overwrite-drag-and-stronger-d/003-SUMMARY.md`
</output>
