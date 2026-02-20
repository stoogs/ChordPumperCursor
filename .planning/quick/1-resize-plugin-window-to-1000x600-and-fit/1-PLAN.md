---
phase: quick-1
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - src/ui/PluginEditor.cpp
  - src/ui/PadComponent.cpp
autonomous: true
requirements: [RESIZE-01]
must_haves:
  truths:
    - "Plugin window opens at 1000x600"
    - "All 64 pads (8x8 grid) are visible without scrolling"
    - "Chord names and roman numerals are readable on smaller pads"
    - "Progression strip remains visible at the bottom"
  artifacts:
    - path: "src/ui/PluginEditor.cpp"
      provides: "Window size 1000x600"
      contains: "setSize(1000, 600)"
    - path: "src/ui/PadComponent.cpp"
      provides: "Scaled-down font sizes for smaller pads"
  key_links:
    - from: "src/ui/PluginEditor.cpp"
      to: "src/ui/GridPanel.cpp"
      via: "resized() setBounds passes smaller area to gridPanel"
      pattern: "gridPanel.setBounds"
---

<objective>
Resize the plugin window from 1000x1200 to 1000x600 and ensure the 8x8 pad grid fits within the smaller space.

Purpose: The current window is too tall -- users cannot see the bottom of the window. The grid uses juce::Grid with Fr(1) fractional tracks so pads auto-resize when the window shrinks. The main code change is the setSize call, plus reducing font sizes so text remains readable on the smaller pads (~58px tall instead of ~138px).

Output: Plugin opens at 1000x600 with all 64 pads and progression strip visible.
</objective>

<execution_context>
@/home/stoo/.claude/get-shit-done/workflows/execute-plan.md
@/home/stoo/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@src/ui/PluginEditor.cpp
@src/ui/GridPanel.cpp
@src/ui/PadComponent.cpp
</context>

<tasks>

<task type="auto">
  <name>Task 1: Resize window and scale pad fonts</name>
  <files>src/ui/PluginEditor.cpp, src/ui/PadComponent.cpp</files>
  <action>
In src/ui/PluginEditor.cpp:
- Line 34: Change `setSize(1000, 1200)` to `setSize(1000, 600)`
- No other changes needed -- the resized() layout logic uses removeFromTop(40) for title, removeFromBottom(50) for strip, and gives the rest to gridPanel. At 600px height with 10px reduce on each side, that leaves ~490px for the grid which is sufficient for 8 rows.

In src/ui/PadComponent.cpp, scale font sizes for the smaller pads (~58px tall vs previous ~138px):
- Line 55: Change font size from 14.0f to 12.0f (chord name, single-line mode)
- Line 65: Change font size from 14.0f to 11.0f (chord name, two-line mode with roman numeral)
- Line 70: Change font size from 11.0f to 9.0f (roman numeral text)

These are the only two files that need changes. GridPanel.cpp already uses Fr(1) fractional grid tracks so pads auto-resize to fill available space.
  </action>
  <verify>
Run `cmake --build build --config Debug 2>&1 | tail -5` to confirm it compiles without errors. Then grep for setSize to confirm the new dimensions: `grep -n "setSize" src/ui/PluginEditor.cpp`
  </verify>
  <done>Plugin compiles successfully. setSize is 1000x600. Font sizes are reduced to 12/11/9 for readable text on smaller pads.</done>
</task>

</tasks>

<verification>
- `grep "setSize(1000, 600)" src/ui/PluginEditor.cpp` returns a match
- `grep "14.0f" src/ui/PadComponent.cpp` returns NO matches (all reduced)
- Project builds without errors
</verification>

<success_criteria>
- Plugin window opens at 1000x600
- All 64 pads visible in the 8x8 grid without scrolling
- Chord names and roman numerals readable on the smaller pads
- Progression strip visible at bottom of window
</success_criteria>

<output>
After completion, create `.planning/quick/1-resize-plugin-window-to-1000x600-and-fit/1-SUMMARY.md`
</output>
