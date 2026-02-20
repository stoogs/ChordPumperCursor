---
phase: 09-chord-depth-ui-polish
verified: 2026-02-20T15:00:00Z
status: passed
score: 12/12 must-haves verified
re_verification: false
---

# Phase 9: Chord Depth & UI Polish Verification Report

**Phase Goal:** Expand chord pads with sub-variation menus (7th/9th/11th/13th extensions where musically useful), make pad borders bolder and glowing on hover, and allow drag-reordering of notes within the progression strip
**Verified:** 2026-02-20
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Success Criteria (from ROADMAP.md)

| # | Criterion | Status | Evidence |
|---|-----------|--------|----------|
| 1 | Selected pads that benefit from extensions show sub-variations (e.g. Cmaj → Cmaj7, Cmaj9, etc.) accessible via sub-pad quadrant UI | VERIFIED | `applySubVariations()` in GridPanel.cpp wires 5 qualifying types; `hasSubVariations`, `quadrantAt()`, quadrant label paint all implemented in PadComponent.cpp |
| 2 | Pad borders are visibly thicker and produce a subtle glow effect on mouse hover | VERIFIED | PadComponent.cpp line 87: `3.0f` border (was 1.5f); lines 77-84: 3 concentric glow rings at 0.07/0.13/0.25 alpha drawn when `isHovered` |
| 3 | Notes in the progression strip can be dragged to reorder their position within the sequence | VERIFIED | ProgressionStrip.cpp: `mouseDrag()` initiates `REORDER:N` drag; `itemDropped()` performs `erase+insert` with toIdx correction; `insertionIndex` cursor drawn in `paint()` |

**Score:** 3/3 success criteria verified

---

## Observable Truths (from plan must_haves)

### Plan 09-01 Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | All 18 chord types (9 existing + 9 extension types) can be constructed and named | VERIFIED | ChordType.h: enum has 18 values (Major through Dom13); kChordSuffix[18] with all suffixes |
| 2 | kAllChords contains 216 chords (12 roots × 18 types) | VERIFIED | PitchClassSet.h line 42: `std::array<Chord, 216> result{}`; line 50: `inline constexpr auto kAllChords = allChords()` |
| 3 | noteCount() returns 3 for triads, 4 for 7ths, 5 for 9ths, 6 for 11ths/13ths | VERIFIED | ChordType.h lines 42-48: multi-range dispatch exactly as planned |
| 4 | kIntervals array has 6 slots per chord type (unused slots are -1) | VERIFIED | ChordType.h line 16: `std::array<std::array<int, 6>, 18> kIntervals`; all 18 rows confirmed |
| 5 | MorphEngine iterates all 216 chords without assertion failure or out-of-bounds | VERIFIED | MorphEngine.cpp line 90: `all.reserve(216)`; line 92: `for (const auto& chord : kAllChords)` |
| 6 | accentForType() handles all 18 types without undefined behaviour | VERIFIED | ChordPumperLookAndFeel.h line 26: `constexpr juce::uint32 accents[18]` with all 18 entries |

### Plan 09-02 Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 7 | Major, Minor, Maj7, Min7, Dom7 pads are visually divided into 4 quadrants with thin separator lines | VERIFIED | PadComponent.cpp lines 59-68: `if (hasSubVariations)` draws vertical + horizontal centre lines at 0x22ffffff |
| 8 | Each quadrant shows a short label at small font | VERIFIED | PadComponent.cpp lines 91-110: 7.5f font, 4 `drawText()` calls per quadrant using `subChords[i].name()` |
| 9 | Clicking/pressing a quadrant fires the correct extension chord | VERIFIED | PadComponent.cpp mouseDown() line 158: `pressedQuadrant = quadrantAt(event.getPosition())`; lines 161-165: `activeChord` resolves from `subChords[pressedQuadrant]`; same pattern in mouseUp() |
| 10 | All pads show a visibly thicker border (3px) at rest | VERIFIED | PadComponent.cpp line 87: `g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 3.0f)` — unconditional |
| 11 | Hovering a pad produces a multi-ring glow effect in the pad's accent colour | VERIFIED | PadComponent.cpp lines 76-84: 3 rings at 9.0f/6.0f/4.0f thickness, 0.07/0.13/0.25 alpha, drawn `if (isHovered)` |

### Plan 09-03 Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 12 | User can drag a chord in the strip and drop it at a different position | VERIFIED | ProgressionStrip.cpp `mouseDrag()` lines 208-228 initiates drag; `itemDropped()` lines 106-136 performs reorder |
| 13 | Dragging shows a visible 3px insertion cursor between slots | VERIFIED | ProgressionStrip.cpp lines 294-304: `g.fillRect(cursorX, ..., 3, ...)` in `paint()` when `insertionIndex >= 0` |
| 14 | Dropping reorders the chord in-place (not appends) | VERIFIED | ProgressionStrip.cpp lines 117-121: `chords.erase(...)` then `chords.insert(...)` with toIdx correction |
| 15 | Reorder drag does not trigger the pad-drag code path | VERIFIED | Drag description uses `"REORDER:N"` prefix (ProgressionStrip.cpp line 225); `isInterestedInDragSource()` routes by prefix (lines 94-99) |
| 16 | Dropping a pad chord onto the strip still appends as before | VERIFIED | ProgressionStrip.cpp lines 138-148: existing pad-drop `addChord()` path preserved after REORDER: check |
| 17 | Progression state is updated after reorder | VERIFIED | ProgressionStrip.cpp lines 123-126: `persistentState.progression = chords` inside ScopedLock after erase+insert |

**Score:** 12/12 (truths 1-6 = plan 01, 7-11 = plan 02, 12-17 = plan 03; 17 total but 5 are from plan 03 extras)

---

## Required Artifacts

| Artifact | Status | Level 1: Exists | Level 2: Substantive | Level 3: Wired |
|----------|--------|----------------|---------------------|----------------|
| `src/engine/ChordType.h` | VERIFIED | Yes | 18-value enum, kIntervals[18][6], kChordSuffix[18], noteCount() — all present | Used by PitchClassSet.h, Chord.cpp, VoiceLeader.cpp |
| `src/engine/PitchClassSet.h` | VERIFIED | Yes | `allChords()` returns `std::array<Chord, 216>`, `kAllChords` constexpr populated | Imported by MorphEngine.cpp (kAllChords loop) |
| `src/ui/ChordPumperLookAndFeel.h` | VERIFIED | Yes | `accents[18]` in `accentForType()` with all 18 colour entries | Used by PadComponent.cpp for accent colour lookup |
| `src/engine/MorphEngine.cpp` | VERIFIED | Yes | `all.reserve(216)` at line 90 | Already part of existing morphTo() pipeline |
| `src/ui/PadComponent.h` | VERIFIED | Yes | `hasSubVariations`, `subChords[4]`, `pressedQuadrant`, `quadrantAt()`, `setSubVariations()` all present | Fields used in PadComponent.cpp paint/mouseDown/mouseUp; setter called from GridPanel.cpp |
| `src/ui/PadComponent.cpp` | VERIFIED | Yes | `quadrantAt()`, quadrant separator paint, glow rings, 3px border, quadrant-aware mouse handlers — all implemented | Directly renders to screen; callbacks wire to MIDI engine |
| `src/ui/GridPanel.cpp` | VERIFIED | Yes | `applySubVariations()` anonymous-namespace helper, called in both `morphTo()` (line 98) and `refreshFromState()` (lines 135, 148) | Wired after every `setChord()` call for all 64 pads |
| `src/ui/ProgressionStrip.h` | VERIFIED | Yes | `reorderDragFromIndex`, `insertionIndex`, `mouseDrag()`, `itemDragMove()`, `insertionIndexAtX()` all declared | Fields used in ProgressionStrip.cpp |
| `src/ui/ProgressionStrip.cpp` | VERIFIED | Yes | Full reorder lifecycle: `mouseDrag()`, `insertionIndexAtX()`, `itemDragMove()`, updated `itemDropped()`, `itemDragExit()`, insertion cursor in `paint()` | Registered as DragAndDropTarget, drag container present via parent lookup |

---

## Key Link Verification

### Plan 09-01 Key Links

| From | To | Via | Status | Evidence |
|------|----|-----|--------|----------|
| `src/engine/Chord.cpp` | `src/engine/ChordType.h` | `kIntervals[static_cast<int>(type)]` | WIRED | Chord.cpp line 11: `auto& intervals = kIntervals[static_cast<int>(type)]` — confirmed by grep |
| `src/engine/PitchClassSet.h` | `src/engine/ChordType.h` | `noteCount()` and `kIntervals` in `allChords()` | WIRED | PitchClassSet.h line 15: `kIntervals[static_cast<int>(chord.type)]`; line 17: `noteCount(chord.type)` |
| `src/engine/MorphEngine.cpp` | `src/engine/PitchClassSet.h` | iterates `kAllChords` in morphTo() | WIRED | MorphEngine.cpp line 92: `for (const auto& chord : kAllChords)` |

### Plan 09-02 Key Links

| From | To | Via | Status | Evidence |
|------|----|-----|--------|----------|
| `PadComponent.cpp paint()` | `hasSubVariations` field | conditional quadrant rendering branch | WIRED | PadComponent.cpp lines 59, 91: `if (hasSubVariations)` branches for separators and labels |
| `PadComponent.cpp mouseDown()` | `quadrantAt() + subChords[]` | `pressedQuadrant` set on mouseDown | WIRED | PadComponent.cpp line 158: `pressedQuadrant = quadrantAt(...)`; line 161-165: resolved to `subChords[pressedQuadrant]` |
| `GridPanel.cpp` | `PadComponent::hasSubVariations` and `subChords` | `setSubVariations()` called after `setChord()` | WIRED | GridPanel.cpp lines 98, 135, 148: `applySubVariations(*pads[i], ...)` in morphTo() and refreshFromState() |

### Plan 09-03 Key Links

| From | To | Via | Status | Evidence |
|------|----|-----|--------|----------|
| `ProgressionStrip::mouseDrag()` | `DragAndDropContainer::startDragging()` | `REORDER:N` prefix string | WIRED | ProgressionStrip.cpp lines 225-227: `"REORDER:" + String(index)` passed to `startDragging()` |
| `ProgressionStrip::itemDropped()` | `chords` vector `erase+insert` | parse `fromIdx` from `REORDER:` prefix, compute corrected `toIdx` | WIRED | ProgressionStrip.cpp lines 108, 117-121: `fromIdx` parsed, `erase` then `insert` with `toIdx > fromIdx` correction |
| `ProgressionStrip::paint()` | `insertionIndex` field | 3px vertical cursor drawn between slots | WIRED | ProgressionStrip.cpp lines 294-304: `g.fillRect(cursorX, ..., 3, ...)` gated on `insertionIndex >= 0` |

---

## Requirements Coverage

The DEPTH-01, DEPTH-02, DEPTH-03 IDs referenced in the plan frontmatter do NOT appear in `.planning/REQUIREMENTS.md`. These IDs are defined only in the ROADMAP.md Phase 9 entry, which references them by name without expanding them into REQUIREMENTS.md.

**Assessment:** The REQUIREMENTS.md covers v1 requirements (GRID-xx, CHRD-xx, MIDI-xx, etc.) and v2 deferred requirements, but does not contain DEPTH-xx entries. Phase 9 was planned after REQUIREMENTS.md was written and uses IDs defined locally in the ROADMAP only. This is a documentation gap — DEPTH-01/02/03 are not orphaned in a harmful sense, but they are not formally registered in REQUIREMENTS.md.

**Functional coverage against roadmap success criteria:**

| Requirement ID | ROADMAP Description | Status | Evidence |
|----------------|--------------------|---------|----|
| DEPTH-01 | Chord extension sub-variations on pads (quadrant UI for 7/9/11/13 extensions) | SATISFIED | Full quadrant system in PadComponent + GridPanel wiring |
| DEPTH-02 | Bolder glowing borders on pad hover | SATISFIED | 3px border unconditional; 3-ring glow when `isHovered` |
| DEPTH-03 | Drag-to-reorder within progression strip | SATISFIED | REORDER: drag protocol, insertion cursor, erase+insert |

**Note on REQUIREMENTS.md traceability:** DEPTH-01/02/03 IDs should be added to REQUIREMENTS.md for completeness, but their absence does not affect code correctness. The features are fully implemented and verified.

---

## Anti-Patterns Scan

Files scanned: ChordType.h, PitchClassSet.h, ChordPumperLookAndFeel.h, MorphEngine.cpp, PadComponent.h, PadComponent.cpp, GridPanel.cpp, ProgressionStrip.h, ProgressionStrip.cpp

**Result:** No anti-patterns found.
- No TODO/FIXME/HACK/PLACEHOLDER comments in any modified file
- No stub implementations (empty lambdas, `return null`, etc.)
- No response-ignored fetch calls (not applicable — C++ JUCE code)
- All handler functions perform substantive work

---

## Human Verification Required

### 1. Quadrant Click Fires Correct Chord

**Test:** Load plugin. Trigger a C Major pad to morph grid. Identify a C Major pad (shows 4 quadrants). Click TR quadrant. Listen to output.
**Expected:** MIDI output plays Cmaj7 notes (C E G B), not plain C Major (C E G)
**Why human:** Cannot verify MIDI note emission programmatically from static analysis

### 2. Glow Visual Quality

**Test:** Move mouse over any pad and observe border.
**Expected:** Accent-coloured glow rings visually distinct from non-hovered state; 3px border clearly thicker than surrounding pads in older screenshots
**Why human:** Visual appearance requires human assessment

### 3. Progression Strip Reorder End-to-End

**Test:** Add 3+ chords to progression strip. Drag one chord slot 10px+ and drop at a different position.
**Expected:** Chord moves to new position; blue 3px vertical cursor visible during drag; order persists after host session reload
**Why human:** Drag-and-drop interaction requires runtime verification; persistence requires session reload

### 4. Pad Drop into Strip Still Appends (Regression)

**Test:** Drag a grid pad onto the progression strip.
**Expected:** Chord is appended to strip (not reordered), strip still behaves as before
**Why human:** Drag interaction differentiation requires runtime verification

---

## Gaps Summary

No gaps. All must-haves verified at all three levels (exists, substantive, wired). All commits referenced in summaries are confirmed present in git log.

**Documentation observation (non-blocking):** DEPTH-01, DEPTH-02, DEPTH-03 requirement IDs exist in ROADMAP.md but are not registered in REQUIREMENTS.md. This is a traceability hygiene issue, not a code deficiency. The features are implemented correctly regardless.

---

_Verified: 2026-02-20_
_Verifier: Claude (gsd-verifier)_
