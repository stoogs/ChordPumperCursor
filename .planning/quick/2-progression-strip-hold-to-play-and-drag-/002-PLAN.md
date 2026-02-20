---
phase: quick-2
plan: 02
type: execute
wave: 1
depends_on: []
files_modified:
  - src/ui/ProgressionStrip.h
  - src/ui/ProgressionStrip.cpp
  - src/ui/PluginEditor.cpp
autonomous: true
requirements: [QUICK-2]

must_haves:
  truths:
    - "Clicking and holding a strip chord sustains MIDI notes for the full hold duration"
    - "Releasing a strip chord stops the MIDI notes immediately"
    - "Dragging a pad onto the strip shows an insertion line indicating where the chord will land"
    - "The insertion line updates as the drag cursor moves across the strip"
    - "The insertion line disappears when the drag exits or drops"
  artifacts:
    - path: "src/ui/ProgressionStrip.h"
      provides: "onPressStart and onPressEnd callbacks on ProgressionStrip"
      contains: "onPressStart"
    - path: "src/ui/ProgressionStrip.cpp"
      provides: "mouseDown fires onPressStart, mouseUp fires onPressEnd; itemDragMove sets insertionIndex for all drag types"
    - path: "src/ui/PluginEditor.cpp"
      provides: "Wiring of onPressStart/onPressEnd to noteOn/noteOff on keyboardState"
  key_links:
    - from: "src/ui/ProgressionStrip.cpp mouseDown"
      to: "PluginEditor onPressStart lambda"
      via: "onPressStart callback"
    - from: "src/ui/ProgressionStrip.cpp mouseUp"
      to: "PluginEditor onPressEnd lambda"
      via: "onPressEnd callback"
    - from: "src/ui/ProgressionStrip.cpp itemDragMove"
      to: "paint() insertion line"
      via: "insertionIndex member"
---

<objective>
Two UX fixes for the progression strip: hold-to-sustain note playback on strip chords, and a visual drop-target insertion line when dragging pads onto the strip.

Purpose: Strip chords should behave like grid pads (sustain while held). Drag-to-strip needs visual feedback so users know exactly where a chord will land.
Output: Modified ProgressionStrip with onPressStart/onPressEnd callbacks and full-drag insertion line tracking.
</objective>

<execution_context>
@/home/stoo/.claude/get-shit-done/workflows/execute-plan.md
@/home/stoo/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Hold-to-sustain on strip chords</name>
  <files>
    src/ui/ProgressionStrip.h
    src/ui/ProgressionStrip.cpp
    src/ui/PluginEditor.cpp
  </files>
  <action>
**ProgressionStrip.h** — add two callbacks alongside `onChordClicked`:
```cpp
std::function<void(const Chord&)> onPressStart;
std::function<void(const Chord&)> onPressEnd;
```

**ProgressionStrip.cpp — mouseDown** — replace the `onChordClicked` call with `onPressStart`. Keep the `isReceivingDrag` / DnD-active guards. The morphTo trigger must still happen somewhere: move the `onChordClicked` call to `mouseUp` (for the click-to-morph behaviour), or keep `onChordClicked` firing on mouseUp only. Concrete changes:

1. In `mouseDown`: after index check, call `onPressStart(chords[index])` (starts notes). Do NOT call `onChordClicked` here.
2. Add `mouseUp` override to ProgressionStrip. In `mouseUp`: if not drag-in-progress (distance < 10px from down position), call `onPressEnd(chords[index])` then `onChordClicked(chords[index])`. If drag occurred (user dragged away), call `onPressEnd` only (no morph trigger). Use `event.getDistanceFromDragStart() < 10` as the click guard, matching the existing 10px reorder threshold.
3. Declare `mouseUp` in ProgressionStrip.h.
4. Track the pressed index as a private member `int pressedIndex = -1` to pass the correct chord to mouseUp without re-computing position.

**PluginEditor.cpp — wiring** — replace the `onChordClicked` lambda with separate `onPressStart` and `onPressEnd` lambdas, plus an updated `onChordClicked`:

```cpp
// onPressStart: noteOn, no timer, no morph
progressionStrip.onPressStart = [this](const Chord& c) {
    auto& ks = processor.getKeyboardState();
    auto notes = c.midiNotes(4);
    for (auto n : notes) ks.noteOn(1, n, 0.8f);
    stripActiveNotes = std::vector<int>(notes.begin(), notes.end());
};

// onPressEnd: noteOff immediately
progressionStrip.onPressEnd = [this](const Chord&) {
    auto& ks = processor.getKeyboardState();
    for (auto n : stripActiveNotes) ks.noteOff(1, n, 0.0f);
    stripActiveNotes.clear();
};

// onChordClicked: morphTo only (notes already handled by press callbacks)
progressionStrip.onChordClicked = [this](const Chord& c) {
    gridPanel.morphTo(c);
};
```

Add `std::vector<int> stripActiveNotes;` as a private member in `PluginEditor.h`.

Note: The existing `onChordClicked` lambda in PluginEditor called noteOn + scheduled noteOff at 300ms AND called morphTo. Split this into the three callbacks above. Remove the old timer-based noteOff — onPressEnd handles it.
  </action>
  <verify>
    Build: `cmake --build /home/stoo/code/GSD/Plugins/ChordPumperCursor/build-release --config Release -j$(nproc)`
    Manual test: Hold a strip chord — notes sustain. Release — notes stop. Click-release quickly — morphs grid.
  </verify>
  <done>Holding a strip chord sustains MIDI notes until mouse release. Quick click still triggers morphTo on the grid.</done>
</task>

<task type="auto">
  <name>Task 2: Insertion line for pad-to-strip drops</name>
  <files>
    src/ui/ProgressionStrip.cpp
  </files>
  <action>
**ProgressionStrip.cpp — itemDragMove** — currently only sets `insertionIndex` for `REORDER:` drags. Extend to handle pad drops too:

```cpp
void ProgressionStrip::itemDragMove(const SourceDetails& details)
{
    auto localPos = getLocalPoint(details.sourceComponent.get(), details.localPosition);
    insertionIndex = insertionIndexAtX(localPos.getX());
    repaint();
}
```

Remove the `REORDER:` prefix check entirely — compute `insertionIndex` for all drag sources (both reorder and pad drops). The `insertionIndexAtX` function clamps correctly so pad drops get the same line as reorder drags.

No other changes needed — `paint()` already draws the insertion line when `insertionIndex >= 0`, and `itemDragExit` already resets `insertionIndex = -1`. `itemDropped` already resets it too (for REORDER; pad-drop path sets `isReceivingDrag = false` and calls repaint but does not reset `insertionIndex` — add `insertionIndex = -1` to the pad-drop branch of `itemDropped` before `repaint()`).
  </action>
  <verify>
    Build: `cmake --build /home/stoo/code/GSD/Plugins/ChordPumperCursor/build-release --config Release -j$(nproc)`
    Manual test: Drag a pad over the strip — vertical insertion line appears and tracks cursor. Drop — line disappears and chord lands at indicated position.
  </verify>
  <done>Dragging any source (pad or reorder) over the strip shows a live insertion line. Line clears on drop or exit.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <what-built>Hold-to-sustain on strip chords and visual drag insertion line for pad drops.</what-built>
  <how-to-verify>
    1. Open ChordPumper in your DAW or standalone.
    2. Add a few chords to the progression strip (click grid pads).
    3. Click and hold a strip chord — verify notes sustain for the full hold duration (not just 300ms).
    4. Release the mouse — verify notes cut off immediately.
    5. Drag a pad to the strip — verify a vertical blue insertion line appears as you move across slots.
    6. Move the cursor to different positions — verify the line jumps between gap positions.
    7. Drop the pad — verify the chord lands at the indicated position and the line disappears.
    8. Quick-click a strip chord (no hold) — verify it still morphs the grid.
  </how-to-verify>
  <resume-signal>Type "approved" or describe any issues</resume-signal>
</task>

</tasks>

<verification>
- Build succeeds with no errors or warnings
- Strip hold behaviour matches pad hold behaviour (sustain for duration)
- Insertion line visible during pad-to-strip drag
- Insertion line tracks cursor position accurately
- No regression: reorder drag still shows insertion line, drop still places chord correctly
</verification>

<success_criteria>
Strip chords sustain MIDI notes for as long as the mouse is held. Dragging any source (pad or strip reorder) over the strip shows a live vertical insertion line at the prospective drop position.
</success_criteria>

<output>
After completion, create `.planning/quick/2-progression-strip-hold-to-play-and-drag-/002-SUMMARY.md`
</output>
