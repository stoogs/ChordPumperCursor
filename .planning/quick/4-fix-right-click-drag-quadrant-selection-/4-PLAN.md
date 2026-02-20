---
phase: quick-4
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - src/ui/PadComponent.cpp
autonomous: true
requirements: []

must_haves:
  truths:
    - "Right-click drag plays and drags the sub-chord under the cursor at press time, not the stale quadrant from the last left-click"
  artifacts:
    - path: "src/ui/PadComponent.cpp"
      provides: "Fixed mouseDown with pressedQuadrant set in right-click path"
      contains: "pressedQuadrant = quadrantAt(event.getPosition())"
  key_links:
    - from: "PadComponent::mouseDown (right-click path)"
      to: "pressedQuadrant"
      via: "quadrantAt(event.getPosition()) assignment before first read"
      pattern: "pressedQuadrant = quadrantAt"
---

<objective>
Fix right-click drag quadrant selection bug in PadComponent.

Purpose: When right-clicking a pad that has sub-variations, the code reads `pressedQuadrant` to select which sub-chord to preview/drag. But `pressedQuadrant` is only set in the left-click path, so right-clicks always use a stale value from the last left-click, playing the wrong sub-chord.

Output: One-line fix so right-click mouseDown sets `pressedQuadrant` before reading it.
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
  <name>Task 1: Set pressedQuadrant in right-click path before first read</name>
  <files>src/ui/PadComponent.cpp</files>
  <action>
In `PadComponent::mouseDown` (around line 141), at the very start of the `if (event.mods.isPopupMenu())` block, add:

    pressedQuadrant = quadrantAt(event.getPosition());

Insert it as the first line inside the block, before `bool shiftHeld = ...`. The block currently reads `pressedQuadrant` on line 146 without ever having set it for right-click events — this one-liner fixes the stale-quadrant bug.

Do NOT change any other logic. The left-click path already sets `pressedQuadrant` correctly and must remain unchanged.
  </action>
  <verify>
    grep -n "pressedQuadrant = quadrantAt" src/ui/PadComponent.cpp
    # Should appear twice: once in the right-click block (new), once in the left-click block (existing)
    cmake --build /home/stoo/code/GSD/Plugins/ChordPumperCursor/build-release --config Release -j$(nproc) 2>&1 | tail -5
  </verify>
  <done>Build succeeds. `pressedQuadrant = quadrantAt(event.getPosition())` appears as the first statement in the `isPopupMenu()` block. Right-click drag now picks the sub-chord under the cursor at press time.</done>
</task>

</tasks>

<verification>
After the build, manually right-click a pad with sub-variations active, drag from each quadrant in turn, and confirm the dragged chord matches the quadrant under the cursor — not whatever quadrant was last left-clicked.
</verification>

<success_criteria>
Build clean. Right-click drag in any quadrant carries the correct sub-chord for that quadrant regardless of prior left-click history.
</success_criteria>

<output>
After completion, create `.planning/quick/4-fix-right-click-drag-quadrant-selection-/4-SUMMARY.md`
</output>
