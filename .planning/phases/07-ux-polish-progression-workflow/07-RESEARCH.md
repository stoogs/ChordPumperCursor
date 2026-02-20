# Phase 7: UX Polish & Progression Workflow - Research

**Researched:** 2026-02-20
**Domain:** JUCE intra-plugin DnD, click/drag disambiguation, FileChooser export, visual polish
**Confidence:** HIGH

## Summary

Phase 7 transforms the ChordPumper progression strip from a passive display into an intentional composition tool. The core behavioral change is: clicking a grid pad should **only preview/play** the chord (no auto-add), while **dragging** a pad into the progression strip adds it. The strip itself becomes interactive — clicking a chord in it plays MIDI, and an export button saves the progression as a `.mid` file.

The codebase is well-positioned for this phase. `ChordPumperEditor` already inherits `juce::DragAndDropContainer`, `PadComponent` already implements the 6px drag threshold, `MidiFileBuilder` already converts chords to MIDI files, and `ProgressionStrip` already manages a `std::vector<Chord>` with FIFO overflow. The main work is: (1) rewiring PadComponent to use `startDragging()` for intra-plugin DnD instead of `performExternalDragDropOfFiles()`, (2) making ProgressionStrip a `DragAndDropTarget`, (3) adding click-to-play on strip slots, (4) building a multi-chord MIDI export with `FileChooser::launchAsync`, and (5) visual polish across the UI.

**Primary recommendation:** Use JUCE's built-in `DragAndDropContainer::startDragging()` / `DragAndDropTarget` API for intra-plugin drag-and-drop with a `juce::var` description carrying the chord data. Keep external DnD (`performExternalDragDropOfFiles`) as a secondary path triggered by dragging outside the plugin window via `shouldDropFilesWhenDraggedExternally()`.

## Standard Stack

### Core (already linked)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 | Framework for all UI, MIDI, file I/O | Already in use; provides DnD, FileChooser, Graphics APIs |
| juce_gui_basics | 8.0.12 | DragAndDropContainer, DragAndDropTarget, FileChooser, ComponentAnimator | All needed APIs live here, already linked |
| juce_gui_extra | 8.0.12 | Additional UI utilities | Already linked |
| juce_audio_basics | 8.0.12 | MidiMessage, MidiFile, MidiMessageSequence | Already linked; used by MidiFileBuilder |

### Supporting (no new dependencies needed)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::ColourGradient | (juce_graphics) | Subtle pad/strip gradients | Visual polish of pads and strip backgrounds |
| juce::DropShadow | (juce_graphics) | Depth effects on components | Optional polish for strip or header |
| juce::ComponentAnimator | (juce_gui_basics) | Smooth fade/position animations | Drop feedback animation, hover transitions |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ComponentAnimator | juce_animation (Animator class) | Newer API with easing curves, but module not linked; would require CMakeLists change. Not worth it for simple feedback animations |
| Custom DnD with mouse tracking | JUCE DragAndDropContainer/Target | Custom is fragile; built-in handles hit testing, drag images, Z-order automatically |
| Modal FileChooser::browseForFileToSave | FileChooser::launchAsync | Modal loops not permitted in plugin context (JUCE_MODAL_LOOPS_PERMITTED not defined); must use async |

## Architecture Patterns

### Current Project Structure (relevant files)
```
src/
├── PluginProcessor.{h,cpp}       # Audio processor, MidiKeyboardState owner
├── PersistentState.{h,cpp}       # ValueTree serialization for grid/progression state
├── ui/
│   ├── PluginEditor.{h,cpp}      # Top-level editor, DragAndDropContainer
│   ├── GridPanel.{h,cpp}         # 8×4 grid of PadComponents, morph engine
│   ├── PadComponent.{h,cpp}      # Individual chord pad, click/drag handling
│   ├── ProgressionStrip.{h,cpp}  # Horizontal strip of up to 8 chords
│   └── ChordPumperLookAndFeel.h  # LookAndFeel_V4 subclass + PadColours
├── midi/
│   ├── MidiFileBuilder.{h,cpp}   # Single-chord MIDI file creation
│   └── ChromaticPalette.h        # Initial chord palette
└── engine/                        # ChordPumperEngine static lib
```

### Pattern 1: Intra-Plugin Drag and Drop (DragAndDropContainer + DragAndDropTarget)
**What:** PadComponent initiates drag via `startDragging()` on the parent `DragAndDropContainer` (ChordPumperEditor). ProgressionStrip implements `DragAndDropTarget` to receive drops.
**When to use:** For all pad-to-strip chord additions.
**Key mechanism:** The `sourceDescription` var carries chord identity. Use a `juce::DynamicObject` or a simple string encoding `"root:type"` (e.g. `"0:1"` for C minor).

```cpp
// In PadComponent::mouseDrag — after 6px threshold
void PadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 6 || isDragInProgress)
        return;

    isDragInProgress = true;

    // Encode chord as var for intra-plugin DnD
    juce::String desc = juce::String(static_cast<int>(chord.root.semitone()))
                      + ":" + juce::String(static_cast<int>(chord.type));

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        container->startDragging(desc, this, juce::ScaledImage(), false);
    }
}
```

```cpp
// In ProgressionStrip — implement DragAndDropTarget
bool ProgressionStrip::isInterestedInDragSource(const SourceDetails& details)
{
    // Accept drags from PadComponent
    return dynamic_cast<PadComponent*>(details.sourceComponent.get()) != nullptr;
}

void ProgressionStrip::itemDropped(const SourceDetails& details)
{
    // Decode chord from description string
    auto desc = details.description.toString();
    // Parse "root:type" and call addChord(...)
}
```

### Pattern 2: Click-Play vs Drag-Add Disambiguation
**What:** mouseDown records press state. mouseUp checks drag distance — if < 6px, it's a click (play chord). If ≥ 6px, drag was initiated and mouseUp is a no-op.
**When to use:** PadComponent interaction model change from Phase 7.
**Critical behavioral change from current code:**
- **Current (Phase 6):** `mouseDown` → immediately calls `onClick(chord)` → GridPanel::padClicked → plays MIDI + adds to strip + morphs grid
- **Phase 7:** `mouseDown` → only sets pressed visual state (no onClick). `mouseUp` with distance < 6px → calls `onClick(chord)` → plays MIDI + morphs grid (no strip add). `mouseDrag` past 6px → starts intra-plugin DnD toward strip.

```cpp
// Phase 7 PadComponent::mouseDown
void PadComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu()) { /* existing right-click export */ return; }
    isPressed = true;
    isDragInProgress = false;
    repaint();
    // NOTE: Do NOT call onClick here anymore
}

// Phase 7 PadComponent::mouseUp
void PadComponent::mouseUp(const juce::MouseEvent& event)
{
    isPressed = false;
    repaint();

    if (!isDragInProgress && event.getDistanceFromDragStart() < 6)
    {
        // This was a click, not a drag — play the chord
        if (onClick)
            onClick(chord);
    }
    isDragInProgress = false;
}
```

### Pattern 3: ProgressionStrip Click-to-Play
**What:** ProgressionStrip detects clicks on individual chord slots and sends MIDI note-on/note-off via MidiKeyboardState.
**When to use:** Making strip chords playable.
**Architecture need:** ProgressionStrip currently has no access to `MidiKeyboardState`. Must be provided (either via constructor injection or a callback).

```cpp
// ProgressionStrip needs a play callback or MidiKeyboardState reference
// Recommended: callback approach (consistent with onChordPlayed pattern)
std::function<void(const Chord&)> onChordClicked;

void ProgressionStrip::mouseDown(const juce::MouseEvent& event)
{
    int index = getChordIndexAtPosition(event.getPosition());
    if (index >= 0 && index < static_cast<int>(chords.size()))
    {
        if (onChordClicked)
            onChordClicked(chords[static_cast<size_t>(index)]);
    }
}
```

### Pattern 4: Async FileChooser for MIDI Export
**What:** FileChooser::launchAsync with saveMode flags, member-lifetime FileChooser.
**When to use:** Save/export button on progression strip.

```cpp
// FileChooser MUST be stored as a member (destroyed = dialog cancelled)
std::unique_ptr<juce::FileChooser> fileChooser;

void ProgressionStrip::exportProgression()
{
    if (chords.empty()) return;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Progression as MIDI",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory)
            .getChildFile("ChordPumper-Progression.mid"),
        "*.mid",
        true,   // use native dialog
        false,  // don't treat packages as dirs
        this);  // parent component — important for embedded plugin windows

    int flags = juce::FileBrowserComponent::saveMode
              | juce::FileBrowserComponent::canSelectFiles
              | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File()) return; // cancelled

        // Build multi-chord MIDI and write
        MidiFileBuilder::exportProgression(chords, 4, file);
    });
}
```

### Pattern 5: External DnD Fallback via shouldDropFilesWhenDraggedExternally
**What:** When an intra-plugin drag moves outside the plugin window, convert it to a file-based external DnD.
**When to use:** Allow dragging chords to DAW timeline even with the new intra-plugin DnD model.

```cpp
// Override in ChordPumperEditor (which is the DragAndDropContainer)
bool ChordPumperEditor::shouldDropFilesWhenDraggedExternally(
    const juce::DragAndDropTarget::SourceDetails& details,
    juce::StringArray& files, bool& canMoveFiles)
{
    auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get());
    if (pad == nullptr) return false;

    auto midiFile = MidiFileBuilder::createMidiFile(pad->getChord(), 4);
    if (!midiFile.existsAsFile()) return false;

    files.add(midiFile.getFullPathName());
    canMoveFiles = false;
    return true;
}
```

### Anti-Patterns to Avoid
- **Mixing external and internal DnD in the same handler:** Use `startDragging()` for intra-plugin, `performExternalDragDropOfFiles()` only as legacy fallback. Don't call both.
- **Calling onClick in mouseDown for click-to-play:** Must wait for mouseUp to distinguish click from drag. Phase 6 behavior (mouseDown = play) must change.
- **Storing FileChooser as stack variable:** It gets destroyed immediately, cancelling the async dialog. Must be a member variable.
- **Using modal FileChooser in plugin context:** JUCE_MODAL_LOOPS_PERMITTED is not defined; browseForFileToSave() will not compile or will silently fail.
- **Holding CriticalSection during FileChooser callback:** The callback runs on the message thread but could interleave with state changes. Lock briefly, copy data, unlock, then write file.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Intra-plugin drag image + hit testing | Custom mouse tracking + bounds checking | `DragAndDropContainer::startDragging()` + `DragAndDropTarget` | JUCE handles transparent drag image, Z-order, multi-monitor, cleanup |
| File save dialog | Custom text input for path | `FileChooser::launchAsync` | Native OS dialog, security sandbox compliance, platform-correct UX |
| MIDI file format | Raw byte writing | `juce::MidiFile` + `juce::MidiMessageSequence` | MidiFileBuilder already wraps this correctly; just extend for multi-chord |
| Smooth animations | Raw Timer + linear interpolation | `ComponentAnimator` or `juce::Timer` with easing | ComponentAnimator handles interruption, completion callbacks, proxy rendering |
| Chord-type colour mapping | Inline switch everywhere | Static lookup table in PadColours or LookAndFeel | Single source of truth, easy to tune |

**Key insight:** The JUCE DnD system handles all the fiddly bits (drag image rendering, cursor changes, hit-test delegation to nested components, cleanup on destruction). Hand-rolling mouse tracking for drag-and-drop is a common JUCE anti-pattern that leads to Z-order bugs and leaked state.

## Common Pitfalls

### Pitfall 1: onClick Firing in mouseDown Breaks Click/Drag Disambiguation
**What goes wrong:** If onClick fires in mouseDown (current behavior), every drag attempt also plays the chord and fires the callback before the user has committed to dragging.
**Why it happens:** Phase 6 design intentionally fires on mouseDown for responsiveness.
**How to avoid:** Move onClick call from mouseDown to mouseUp, gated by `event.getDistanceFromDragStart() < 6 && !isDragInProgress`.
**Warning signs:** Every drag operation also plays a chord sound.

### Pitfall 2: FileChooser Destroyed Before Async Callback
**What goes wrong:** Dialog appears then immediately disappears, or callback never fires.
**Why it happens:** FileChooser created as local variable in button callback; goes out of scope.
**How to avoid:** Store as `std::unique_ptr<juce::FileChooser>` member on the component that owns the export button.
**Warning signs:** Export button click does nothing, or dialog flashes briefly.

### Pitfall 3: startDragging() with Wrong Source Component
**What goes wrong:** `DragAndDropTarget::isInterestedInDragSource` receives wrong `sourceComponent`, breaking type checks.
**Why it happens:** Passing `this` when `this` is a child of the actual drag source component.
**How to avoid:** Always pass the PadComponent pointer as `sourceComponent` to `startDragging()`.
**Warning signs:** `dynamic_cast<PadComponent*>(details.sourceComponent.get())` returns nullptr in the drop target.

### Pitfall 4: Drag Image Not Showing in Embedded Plugin Windows
**What goes wrong:** Drag image invisible or clipped to plugin bounds.
**Why it happens:** Plugin is embedded in DAW window; JUCE's drag image is a peer-level component that may get clipped.
**How to avoid:** Set `allowDraggingToOtherJuceWindows = false` (default) and accept that drag image stays within the plugin editor bounds. For visual feedback, use `itemDragEnter`/`itemDragExit` on the target to highlight the strip instead.
**Warning signs:** Dragging works (drop succeeds) but no visual drag ghost appears.

### Pitfall 5: Multi-Chord MIDI Export with Wrong Timing
**What goes wrong:** All chords play simultaneously in the exported MIDI file.
**Why it happens:** All note-on events placed at tick 0 (current MidiFileBuilder pattern for single chord).
**How to avoid:** For progression export, place each chord at successive bar boundaries: chord[i] starts at `i * kBarLengthTicks`.
**Warning signs:** DAW imports the MIDI file but shows all notes stacked at beat 1.

### Pitfall 6: ProgressionStrip Click Detected During Drag-Over
**What goes wrong:** Clicking/tapping on the strip during a drag-in-progress inadvertently plays a chord.
**Why it happens:** mouseDown on strip fires while DnD is active.
**How to avoid:** In ProgressionStrip::mouseDown, check `DragAndDropContainer::isDragAndDropActive()` on the parent container, or check if the target is currently receiving a drag (use a bool flag set in `itemDragEnter`/`itemDragExit`).
**Warning signs:** Dropping a chord on the strip also plays it.

### Pitfall 7: FileChooser on Linux in Embedded Plugin
**What goes wrong:** Native file dialog doesn't appear or appears behind plugin window.
**Why it happens:** Linux plugin embedding (XEMBED) can interfere with top-level window creation.
**How to avoid:** Pass `this` (the component) as `parentComponent` in FileChooser constructor. If native dialog fails, JUCE falls back to its built-in file browser. Also consider a right-click fallback export to `~/ChordPumper-Export/` (already established pattern in PadComponent).
**Warning signs:** Export button does nothing on Linux; works on standalone.

## Code Examples

### Multi-Chord Progression MIDI Export (new method for MidiFileBuilder)
```cpp
// Source: Extends existing MidiFileBuilder pattern
static juce::File exportProgression(const std::vector<Chord>& chords,
                                     int octave, const juce::File& file,
                                     float velocity = 0.8f)
{
    juce::MidiMessageSequence seq;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent(kTempoMicrosecondsPerBeat), 0.0);

    for (size_t i = 0; i < chords.size(); ++i)
    {
        double startTick = static_cast<double>(i * kBarLengthTicks);
        double endTick = startTick + static_cast<double>(kBarLengthTicks);

        for (int note : chords[i].midiNotes(octave))
        {
            seq.addEvent(juce::MidiMessage::noteOn(kChannel, note, velocity), startTick);
            seq.addEvent(juce::MidiMessage::noteOff(kChannel, note, 0.0f), endTick);
        }
    }
    seq.updateMatchedPairs();

    juce::MidiFile midi;
    midi.setTicksPerQuarterNote(kTicksPerQuarterNote);
    midi.addTrack(seq);

    file.deleteFile();
    auto stream = file.createOutputStream();
    if (stream == nullptr) return {};
    midi.writeTo(*stream);
    stream->flush();
    return file;
}
```

### Chord Encoding/Decoding for DnD Description Var
```cpp
// Encode: PadComponent → var
juce::String encodeChord(const Chord& c)
{
    return juce::String(c.root.semitone()) + ":" + juce::String(static_cast<int>(c.type));
}

// Decode: var → Chord (in ProgressionStrip::itemDropped)
Chord decodeChord(const juce::var& description)
{
    auto parts = juce::StringArray::fromTokens(description.toString(), ":", "");
    if (parts.size() != 2) return {};
    int semitone = parts[0].getIntValue();
    int type = parts[1].getIntValue();
    return Chord{ PitchClass::fromSemitone(semitone),
                  static_cast<ChordType>(type) };
}
```

### ProgressionStrip Slot Hit Detection
```cpp
// Source: Custom based on existing paint() slot layout
int ProgressionStrip::getChordIndexAtPosition(juce::Point<int> pos) const
{
    auto area = getLocalBounds();
    auto slotArea = area.removeFromLeft(area.getWidth() - 60);
    auto slotWidth = (slotArea.getWidth() - (kMaxChords - 1) * 4) / kMaxChords;

    if (!slotArea.contains(pos)) return -1;

    int x = pos.getX() - slotArea.getX();
    int index = x / (slotWidth + 4);

    if (index < 0 || index >= static_cast<int>(chords.size())) return -1;

    // Verify click is within slot bounds (not in the 4px gap)
    int slotLeft = index * (slotWidth + 4);
    if (pos.getX() - slotArea.getX() > slotLeft + slotWidth) return -1;

    return index;
}
```

### Visual Polish: Chord-Type Colour Differentiation
```cpp
// Source: Extension of existing PadColours namespace
namespace PadColours {
    // Existing
    inline constexpr juce::uint32 background = 0xff2a2a3a;
    inline constexpr juce::uint32 pressed    = 0xff6c8ebf;
    inline constexpr juce::uint32 hovered    = 0xff353545;

    // New: chord-type accent colours (used as border or subtle tint)
    inline constexpr juce::uint32 majorAccent     = 0xff4a9eff; // blue
    inline constexpr juce::uint32 minorAccent     = 0xff9b6dff; // purple
    inline constexpr juce::uint32 diminishedAccent = 0xffff6b6b; // red-ish
    inline constexpr juce::uint32 augmentedAccent  = 0xffffb347; // orange
    inline constexpr juce::uint32 dom7Accent       = 0xff5bc0de; // teal
    inline constexpr juce::uint32 maj7Accent       = 0xff6baed6; // light blue
    inline constexpr juce::uint32 min7Accent       = 0xffb39ddb; // light purple
    inline constexpr juce::uint32 dim7Accent       = 0xffef5350; // bright red
    inline constexpr juce::uint32 halfDim7Accent   = 0xffff8a65; // salmon

    inline juce::uint32 accentForType(ChordType type) {
        constexpr juce::uint32 accents[] = {
            majorAccent, minorAccent, diminishedAccent, augmentedAccent,
            maj7Accent, min7Accent, dom7Accent, dim7Accent, halfDim7Accent
        };
        return accents[static_cast<int>(type)];
    }
}
```

### Gradient Background for Pads
```cpp
// Subtle vertical gradient instead of flat fill
auto gradient = juce::ColourGradient::vertical(
    juce::Colour(PadColours::background).brighter(0.05f),
    juce::Colour(PadColours::background).darker(0.05f),
    bounds);
g.setGradientFill(gradient);
g.fillRoundedRectangle(bounds, cornerSize);

// Accent border based on chord type
g.setColour(juce::Colour(PadColours::accentForType(chord.type)).withAlpha(0.4f));
g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
```

### Drop Target Visual Feedback on ProgressionStrip
```cpp
void ProgressionStrip::itemDragEnter(const SourceDetails&)
{
    isReceivingDrag = true;
    repaint();
}

void ProgressionStrip::itemDragExit(const SourceDetails&)
{
    isReceivingDrag = false;
    repaint();
}

// In paint(), when isReceivingDrag:
if (isReceivingDrag)
{
    // Highlight next empty slot or the strip border
    g.setColour(juce::Colour(0xff6c8ebf).withAlpha(0.3f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| ComponentAnimator | juce_animation Animator | JUCE 8.0 | New easing/builder API, but ComponentAnimator still works and is in juce_gui_basics (already linked). No need to add juce_animation for this phase. |
| Modal FileChooser | FileChooser::launchAsync | JUCE 6.x | Plugin context prohibits modal loops; launchAsync is required. The project correctly does NOT define JUCE_MODAL_LOOPS_PERMITTED. |
| performExternalDragDropOfFiles for all DnD | startDragging for intra-plugin + shouldDropFilesWhenDraggedExternally for external | Always in JUCE | The current PadComponent uses only external DnD. Phase 7 switches to internal DnD as primary. |

**Deprecated/outdated:**
- `ComponentAnimator`: Superseded by `juce::Animator` in `juce_animation` module, but still functional. Using it avoids adding a new module dependency.
- `FileChooser::browseForFileToSave()`: Modal variant — not usable in plugin context without `JUCE_MODAL_LOOPS_PERMITTED`.

## Open Questions

1. **PitchClass::fromSemitone() existence**
   - What we know: `PitchClass` has `.semitone()` and `.midiNote(octave)` methods.
   - What's unclear: Whether a static `PitchClass::fromSemitone(int)` factory exists for decoding the DnD description. If not, it needs to be added or the DnD encoding should use a different approach.
   - Recommendation: Check `PitchClass.h` during planning. If missing, add a trivial factory or use the index-into-chromatic-pitch-array approach.

2. **External DnD coexistence with intra-plugin DnD**
   - What we know: `shouldDropFilesWhenDraggedExternally()` is called when a `startDragging()` drag leaves the JUCE window. Bitwig on Linux X11 has partial DnD support (recognized but drop doesn't finalize — known limitation from Phase 5).
   - What's unclear: Whether `shouldDropFilesWhenDraggedExternally` fires reliably in Bitwig's embedded X11 window.
   - Recommendation: Implement it as best-effort. Keep the right-click fallback export to `~/ChordPumper-Export/` as the reliable escape hatch.

3. **Strip click-to-play: Voice leading state**
   - What we know: GridPanel maintains `activeNotes` for voice leading via `optimalVoicing()`. The strip doesn't have this context.
   - What's unclear: Whether strip click should use voice leading from the last-played note set, or just use basic `chord.midiNotes(octave)`.
   - Recommendation: Keep strip playback simple — use `chord.midiNotes(defaultOctave)` directly without voice leading. The strip is for quick preview, not performance. This avoids needing shared voicing state.

4. **Export button placement and strip layout**
   - What we know: Current strip has 8 slots + a "Clear" button (56px wide on right).
   - What's unclear: Where the Export button should go — next to Clear? Replace Clear? Additional row?
   - Recommendation: Add Export button next to Clear (both on the right side). Reduce slot area slightly to accommodate. Example: `[8 slots] [Export] [Clear]`.

## Sources

### Primary (HIGH confidence)
- `libs/JUCE/modules/juce_gui_basics/mouse/juce_DragAndDropContainer.h` — startDragging API, shouldDropFilesWhenDraggedExternally, findParentDragContainerFor
- `libs/JUCE/modules/juce_gui_basics/mouse/juce_DragAndDropTarget.h` — isInterestedInDragSource, itemDropped, itemDragEnter/Exit
- `libs/JUCE/modules/juce_gui_basics/filebrowser/juce_FileChooser.h` — launchAsync, getResult, constructor with parentComponent
- `libs/JUCE/modules/juce_gui_basics/filebrowser/juce_FileBrowserComponent.h` — FileChooserFlags enum (saveMode=2, canSelectFiles=4, warnAboutOverwriting=128)
- `libs/JUCE/modules/juce_graphics/colour/juce_ColourGradient.h` — ColourGradient::vertical
- `libs/JUCE/modules/juce_graphics/effects/juce_DropShadowEffect.h` — DropShadow struct
- `libs/JUCE/modules/juce_gui_basics/layout/juce_ComponentAnimator.h` — animateComponent, fadeIn/fadeOut
- Codebase: `src/ui/PadComponent.cpp` — current mouseDown/mouseDrag/mouseUp, 6px threshold, external DnD
- Codebase: `src/ui/ProgressionStrip.cpp` — current paint/slot layout, addChord, clear
- Codebase: `src/ui/PluginEditor.cpp` — DragAndDropContainer inheritance, onChordPlayed wiring
- Codebase: `src/midi/MidiFileBuilder.cpp` — createMidiFile, exportToDirectory, buildSequence
- Codebase: `src/ui/ChordPumperLookAndFeel.h` — PadColours namespace, ColourScheme

### Secondary (MEDIUM confidence)
- JUCE official docs: FileChooser class reference (https://docs.juce.com/master/classjuce_1_1FileChooser.html) — launchAsync pattern
- JUCE official docs: DragAndDropContainer reference (https://docs.juce.com/master/classjuce_1_1DragAndDropContainer.html) — startDragging parameters
- JUCE forum: Drag and Drop example (https://forum.juce.com/t/drag-and-drop-example/206) — community patterns
- JUCE tutorial: LookAndFeel customization (https://juce.com/tutorials/tutorial_look_and_feel_customisation/) — ColourScheme approach

### Tertiary (LOW confidence)
- Linux XEMBED + FileChooser interaction: Community reports suggest `launchAsync` works with native Linux dialogs but can have Z-order issues in embedded plugin windows. Passing `parentComponent` to FileChooser constructor helps. Fallback file browser works if native fails.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs verified by reading JUCE 8.0.12 header files directly from the codebase
- Architecture: HIGH — patterns derived from existing codebase architecture and verified JUCE API contracts
- Pitfalls: HIGH for DnD/FileChooser pitfalls (verified from API docs), MEDIUM for Linux-specific embedding issues (community reports, not reproduced)
- Visual polish: MEDIUM — colour scheme approach is verified, specific accent colours are aesthetic choices that may need tuning

**Research date:** 2026-02-20
**Valid until:** 2026-04-20 (JUCE 8.x API is stable; patterns unlikely to change)
