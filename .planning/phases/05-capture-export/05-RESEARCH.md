# Phase 5: Capture & Export - Research

**Researched:** 2026-02-19
**Domain:** MIDI file creation, drag-and-drop (DnD) to DAW, progression tracking UI
**Confidence:** MEDIUM

## Summary

Phase 5 has three distinct technical challenges: (1) creating Standard MIDI Files from chord data, (2) implementing drag-and-drop of those MIDI files to the DAW, and (3) building a progression strip UI that tracks the user's chord exploration sequence.

MIDI file creation is straightforward — JUCE 8 provides `MidiFile`, `MidiMessageSequence`, and `MidiMessage` classes that handle the SMF format natively. The chord data model (`Chord::midiNotes()`) already produces the MIDI note numbers needed. The main design decision is clip duration (one bar at 120 BPM is a sensible default).

Drag-and-drop is the highest-risk area. JUCE provides `DragAndDropContainer::performExternalDragDropOfFiles()` which uses X11's XDND protocol on Linux. However, **external DnD from embedded plugin windows on Linux X11 is a known, long-standing issue** in JUCE. The `externalDragInit()` implementation calls `XGrabPointer` on the plugin's native window handle, which can fail when the plugin window is reparented into the DAW's window hierarchy. Multiple JUCE forum threads confirm this is problematic. A feasibility spike is essential, with a file-export fallback (write `.mid` to a known directory, user drags from file manager) as the planned alternative.

The progression strip is a standard JUCE UI component — a horizontal row of up to 8 chord slots with a clear button. This is architecturally simple, requiring only a data model (`std::vector<Chord>` capped at 8) and a `Component` that renders the sequence.

**Primary recommendation:** Implement MIDI file creation first (lowest risk), then spike DnD feasibility in an isolated test, then build the progression strip. Have file-export fallback ready before attempting DnD integration.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CAPT-01 | User can drag an individual chord from any pad to the DAW as a MIDI clip | MIDI file creation via `MidiFile`/`MidiMessageSequence` + `performExternalDragDropOfFiles` or file-export fallback; see Architecture Patterns and Common Pitfalls |
| CAPT-02 | A progression strip displays the sequence of chords the user has triggered (up to 8 chords) | `ProgressionStrip` component with `std::vector<Chord>` model, horizontal layout; see Architecture Patterns §3 |
| CAPT-03 | User can clear the progression strip and start a new sequence | Clear button on progression strip; trivial once CAPT-02 model exists |
</phase_requirements>

## Standard Stack

### Core

| Library/Class | Version | Purpose | Why Standard |
|---------------|---------|---------|--------------|
| `juce::MidiFile` | JUCE 8.0.12 | Write Standard MIDI File format (.mid) | Built into JUCE, handles SMF Type 0/1 encoding, tick-based timestamps |
| `juce::MidiMessageSequence` | JUCE 8.0.12 | Build ordered sequence of MIDI events for a track | Companion to MidiFile, manages note-on/off pairing via `updateMatchedPairs()` |
| `juce::MidiMessage` | JUCE 8.0.12 | Create individual MIDI note-on/off events | Static factory methods `noteOn()`, `noteOff()` with channel, note, velocity |
| `juce::DragAndDropContainer` | JUCE 8.0.12 | Enable internal + external drag-and-drop | `performExternalDragDropOfFiles()` for OS-level file DnD; `shouldDropFilesWhenDraggedExternally()` callback pattern |
| `juce::File` | JUCE 8.0.12 | Temp file creation, file path management | `File::createTempFile(".mid")` for generating temp MIDI files |

### Supporting

| Library/Class | Version | Purpose | When to Use |
|---------------|---------|---------|-------------|
| `juce::FileOutputStream` | JUCE 8.0.12 | Stream for writing MidiFile to disk | Created from `File::createOutputStream()`, passed to `MidiFile::writeTo()` |
| `juce::MemoryOutputStream` | JUCE 8.0.12 | In-memory stream for MidiFile data | Alternative to FileOutputStream if building MIDI data in memory first |
| `juce::Timer` | JUCE 8.0.12 | Delayed cleanup of temp files | Delete temp `.mid` files after DnD operation completes |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `performExternalDragDropOfFiles` (OS DnD) | File export to fixed directory + user drags from file manager | Works reliably on Linux but requires extra user step; planned fallback |
| `performExternalDragDropOfFiles` (OS DnD) | `shouldDropFilesWhenDraggedExternally` callback | More elegant (auto-triggers when internal drag leaves plugin window) but same underlying X11 issues |
| Temp file on disk | `MemoryOutputStream` only | DAW needs a real file path for DnD; memory-only won't work for external drop |

## Architecture Patterns

### Recommended Project Structure

```
src/
├── midi/
│   ├── ChromaticPalette.h/.cpp    # existing
│   └── MidiFileBuilder.h/.cpp     # NEW: creates .mid files from Chord data
├── ui/
│   ├── PadComponent.h/.cpp        # MODIFIED: add mouseDrag for DnD initiation
│   ├── GridPanel.h/.cpp           # MODIFIED: progression tracking, DragAndDropContainer
│   ├── ProgressionStrip.h/.cpp    # NEW: horizontal chord sequence display
│   └── PluginEditor.h/.cpp        # MODIFIED: layout includes ProgressionStrip
└── PluginProcessor.h/.cpp         # minimal changes
```

### Pattern 1: MidiFileBuilder — Create MIDI File from Chord

**What:** A utility class that takes a `Chord` (or `VoicedChord`) and produces a `.mid` file on disk.
**When to use:** Every time a user initiates a drag or export of a chord.

```cpp
// Source: JUCE MidiFile API (juce_MidiFile.h, juce_MidiMessageSequence.h)
class MidiFileBuilder {
public:
    static juce::File createMidiFile(const Chord& chord, int octave,
                                      float velocity = 0.8f);
private:
    static constexpr int kTicksPerQuarterNote = 480;
    static constexpr int kDefaultChannel = 1;
};

// Implementation pattern:
juce::File MidiFileBuilder::createMidiFile(const Chord& chord, int octave,
                                            float velocity) {
    juce::MidiMessageSequence seq;
    auto notes = chord.midiNotes(octave);
    const double noteOnTime = 0.0;
    const double noteOffTime = kTicksPerQuarterNote * 4.0; // 1 bar in 4/4

    for (auto note : notes) {
        seq.addEvent(juce::MidiMessage::noteOn(kDefaultChannel, note, velocity),
                     noteOnTime);
        seq.addEvent(juce::MidiMessage::noteOff(kDefaultChannel, note),
                     noteOffTime);
    }
    seq.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(kTicksPerQuarterNote);
    midiFile.addTrack(seq);

    auto tempFile = juce::File::createTempFile(".mid");
    if (auto stream = tempFile.createOutputStream()) {
        midiFile.writeTo(*stream);
    }
    return tempFile;
}
```

### Pattern 2: Drag-to-DAW via performExternalDragDropOfFiles

**What:** Initiate OS-level file drag from a PadComponent's `mouseDrag` callback.
**When to use:** When user drags a pad (as opposed to clicking it).

```cpp
// Source: JUCE DragAndDropContainer (juce_DragAndDropContainer.h)
void PadComponent::mouseDrag(const juce::MouseEvent& event) {
    if (event.getDistanceFromDragStart() < 5)
        return; // click threshold

    auto midiFile = MidiFileBuilder::createMidiFile(chord, defaultOctave);

    if (midiFile.existsAsFile()) {
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            { midiFile.getFullPathName() },
            false, // canMoveFiles = false (copy only)
            this,
            [midiFile]() {
                midiFile.deleteFile(); // cleanup after DnD completes
            }
        );
    }
}
```

**Critical detail:** `performExternalDragDropOfFiles` MUST be called from within a `mouseDrag` or `mouseDown` handler (the implementation uses the current mouse drag state to find the ComponentPeer). Calling from elsewhere triggers a `jassertfalse`.

### Pattern 3: Alternative — shouldDropFilesWhenDraggedExternally Callback

**What:** Override on the `DragAndDropContainer` to auto-export when an internal drag leaves the plugin window.
**When to use:** If `PluginEditor` inherits from `DragAndDropContainer`, internal drags that exit the window automatically trigger the callback.

```cpp
// PluginEditor inherits DragAndDropContainer
class ChordPumperEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer {
protected:
    bool shouldDropFilesWhenDraggedExternally(
        const juce::DragAndDropTarget::SourceDetails& details,
        juce::StringArray& files, bool& canMoveFiles) override
    {
        // Extract chord from drag description
        auto* pad = dynamic_cast<PadComponent*>(details.sourceComponent.get());
        if (!pad) return false;

        auto midiFile = MidiFileBuilder::createMidiFile(pad->getChord(), 4);
        files.add(midiFile.getFullPathName());
        canMoveFiles = false;
        pendingTempFiles.add(midiFile); // track for cleanup
        return true;
    }
};
```

### Pattern 4: ProgressionStrip Component

**What:** Horizontal strip showing up to 8 chord "slots" representing the user's exploration sequence.
**When to use:** Always visible below or above the grid panel.

```cpp
class ProgressionStrip : public juce::Component {
public:
    void addChord(const Chord& chord);
    void clear();
    const std::vector<Chord>& getChords() const;

private:
    std::vector<Chord> chords; // max 8
    static constexpr int kMaxChords = 8;

    void paint(juce::Graphics& g) override;
    void resized() override;
};
```

### Anti-Patterns to Avoid

- **Creating MidiFile on the audio thread:** MidiFile writing involves file I/O — always do it on the GUI thread (from mouse callbacks), never from `processBlock`.
- **Calling performExternalDragDropOfFiles outside a mouse handler:** The Linux implementation uses `getPeerForDragEvent()` which requires an active mouse drag. Calling asynchronously or from a timer will trigger `jassertfalse` and fail.
- **Not cleaning up temp files:** Every drag creates a `.mid` temp file. Without cleanup, `/tmp` fills up over time. Use the completion callback to delete.
- **Blocking on DnD completion:** `performExternalDragDropOfFiles` is asynchronous. Don't spin-wait or block the message thread waiting for it.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| MIDI file format encoding | Custom SMF byte writer | `juce::MidiFile::writeTo()` | SMF has variable-length delta times, track headers, meta events — JUCE handles all edge cases |
| Note-on/off pairing | Manual offset tracking | `MidiMessageSequence::updateMatchedPairs()` | Ensures correct pairing, handles edge cases with duplicate notes |
| OS drag-and-drop protocol | Raw X11 XDND messages | `DragAndDropContainer::performExternalDragDropOfFiles()` | XDND has versioning, selection ownership, atom negotiation — JUCE wraps all of it |
| Temp file lifecycle | Manual `/tmp` path generation | `juce::File::createTempFile(".mid")` | Handles unique naming, OS temp directory resolution |

**Key insight:** The entire MIDI file creation + DnD pipeline is covered by JUCE's built-in classes. The only custom code needed is the glue: converting `Chord` data to `MidiMessageSequence` events, and wiring the DnD initiation to mouse handlers.

## Common Pitfalls

### Pitfall 1: Linux X11 External DnD from Embedded Plugin Windows

**What goes wrong:** `performExternalDragDropOfFiles` fails silently or triggers `jassertfalse` when called from a JUCE plugin embedded in a DAW on Linux.
**Why it happens:** The implementation calls `XGrabPointer` on the plugin's native window handle. When the plugin window is reparented into the DAW's X11 window hierarchy (standard for VST3/CLAP on X11), the pointer grab may fail because:
1. The DAW already holds a pointer grab
2. The plugin's window may not be a top-level X11 window
3. XDND messages sent to the root window may not route correctly through reparented windows

The JUCE Linux DnD implementation (`juce_DragAndDrop_linux.cpp`) uses `X11DragState::externalDragInit()` which does `XGrabPointer` and `XSetSelectionOwner`. Both require the window to be visible and have appropriate event masks, which can be disrupted by reparenting.

**How to avoid:**
1. **Spike early:** Test `performExternalDragDropOfFiles` in Phase 5 Plan 2 before building features on top of it
2. **Test in Standalone first:** Standalone mode uses a real top-level window where DnD will work
3. **Have fallback ready:** File-export fallback (CAPT-01 alternative path)
4. **Test in both VST3 and CLAP:** Different plugin formats may handle reparenting differently

**Warning signs:** `jassertfalse` in debug builds at the `getPeerForDragEvent` call, or `XGrabPointer` returning `GrabNotViewable` / `AlreadyGrabbed`.

### Pitfall 2: MidiFile Timestamps in Ticks Not Seconds

**What goes wrong:** MIDI file events play at wrong times or all at once when imported into DAW.
**Why it happens:** `MidiMessageSequence::addEvent()` timestamps are in the units set by `MidiFile::setTicksPerQuarterNote()`. Forgetting to set TPQN, or mixing tick units with second units, produces garbled timing.
**How to avoid:** Always call `setTicksPerQuarterNote(480)` before `addTrack()`. Use tick arithmetic: 480 ticks = 1 quarter note = 1 beat. One bar of 4/4 = 1920 ticks.
**Warning signs:** All chord notes import as a tiny sliver instead of a bar-length clip.

### Pitfall 3: Missing Note-Off Messages

**What goes wrong:** Chord notes sustain indefinitely when imported into DAW.
**Why it happens:** Forgetting to add note-off events to the MidiMessageSequence, or adding them at timestamp 0 (same as note-on).
**How to avoid:** Always add explicit note-off for every note-on at the desired end time. Call `seq.updateMatchedPairs()` after adding all events.
**Warning signs:** Stuck notes in DAW when playing back the imported MIDI clip.

### Pitfall 4: Temp File Cleanup Race Condition

**What goes wrong:** Temp `.mid` file is deleted before the DAW finishes reading it.
**Why it happens:** The DnD completion callback fires when the drag operation ends (mouse released), but the DAW may still be reading the file at that moment.
**How to avoid:** Use a small delay (e.g., `juce::Timer::callAfterDelay(2000, ...)`) before deleting temp files in the completion callback. Alternatively, accumulate temp files and clean them all up on plugin close.
**Warning signs:** Bitwig shows empty MIDI clip or error on import.

### Pitfall 5: mouseDrag vs mouseDown Click/Drag Disambiguation

**What goes wrong:** Clicking a pad to play a chord accidentally triggers a drag operation, or dragging fails because it conflicts with the click handler.
**Why it happens:** `mouseDown` fires before `mouseDrag`. Without a distance threshold, every click starts a drag.
**How to avoid:** Use `event.getDistanceFromDragStart() < threshold` in `mouseDrag` to distinguish clicks from drags. Only initiate DnD after a minimum drag distance (5-8 pixels). The `mouseDown` handler fires the chord, `mouseDrag` (past threshold) initiates DnD.
**Warning signs:** Chords play when trying to drag, or drags start on every click.

## Code Examples

Verified patterns from JUCE 8.0.12 source and official documentation:

### Creating a One-Bar MIDI Chord Clip

```cpp
// Source: juce_MidiFile.h, juce_MidiMessageSequence.h, juce_MidiMessage.h
juce::File createChordMidiFile(const std::vector<int>& midiNotes,
                                float velocity) {
    constexpr int tpqn = 480;
    constexpr int channel = 1;
    constexpr double barLength = tpqn * 4.0; // 4/4 time

    juce::MidiMessageSequence seq;

    for (auto note : midiNotes) {
        auto noteOn = juce::MidiMessage::noteOn(channel, note, velocity);
        noteOn.setTimeStamp(0.0);
        seq.addEvent(noteOn);

        auto noteOff = juce::MidiMessage::noteOff(channel, note);
        noteOff.setTimeStamp(barLength);
        seq.addEvent(noteOff);
    }

    seq.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(tpqn);
    midiFile.addTrack(seq);

    auto tempFile = juce::File::createTempFile(".mid");
    auto stream = tempFile.createOutputStream();
    if (stream != nullptr) {
        midiFile.writeTo(*stream);
        stream.reset(); // flush and close before DnD reads it
    }
    return tempFile;
}
```

### Initiating External File Drag from mouseDrag

```cpp
// Source: juce_DragAndDropContainer.h, juce_Windowing_linux.cpp
void PadComponent::mouseDrag(const juce::MouseEvent& event) {
    if (event.getDistanceFromDragStart() < 6)
        return;

    if (isDragInProgress)
        return;

    auto midiFile = createChordMidiFile(chord.midiNotes(4), 0.8f);
    if (!midiFile.existsAsFile())
        return;

    isDragInProgress = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        { midiFile.getFullPathName() },
        false,
        this,
        [this, midiFile]() {
            isDragInProgress = false;
            // Delayed cleanup to ensure DAW has read the file
            juce::Timer::callAfterDelay(2000, [midiFile]() {
                midiFile.deleteFile();
            });
        }
    );
}
```

### Progression Strip Data Model

```cpp
class ProgressionModel {
public:
    void addChord(const Chord& chord) {
        if (chords.size() >= kMaxChords)
            chords.erase(chords.begin()); // FIFO: drop oldest
        chords.push_back(chord);
    }

    void clear() { chords.clear(); }

    const std::vector<Chord>& getChords() const { return chords; }
    bool isFull() const { return chords.size() >= kMaxChords; }

    static constexpr size_t kMaxChords = 8;

private:
    std::vector<Chord> chords;
};
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `DragAndDropContainer::startDragging` with Image drag (internal only) | `performExternalDragDropOfFiles` for OS-level file DnD | JUCE 5+ | Enables drag-to-DAW workflow |
| Manual SMF byte writing | `MidiFile::writeTo()` | JUCE 3+ | No need to hand-roll SMF format |
| `juce::Font(float)` constructor | `juce::Font(juce::FontOptions(float))` | JUCE 8 | Deprecated float constructor; use FontOptions |

**Deprecated/outdated:**
- `startDragging()` with `Image` parameter: Use `ScaledImage` overload instead (the Image version is `[[deprecated]]`)
- Wayland: JUCE 8.0.12 still uses X11 for Linux plugin windows. Wayland DnD is not relevant yet for embedded plugins.

## Open Questions

1. **Does `performExternalDragDropOfFiles` work from Bitwig-embedded plugin windows on X11?**
   - What we know: JUCE forum threads report Linux external DnD as broken for plugins. The implementation uses `XGrabPointer` which can fail on reparented windows. Standalone mode should work.
   - What's unclear: Whether Bitwig's specific embedding approach (CLAP/VST3) allows the pointer grab to succeed. Bitwig uses Java/AWT for its UI which has its own X11 windows.
   - Recommendation: Plan 05-02 must be a feasibility spike. Test with a minimal `mouseDrag` → `performExternalDragDropOfFiles` before building features on top. Budget time for fallback implementation.

2. **What clip duration should the exported MIDI use?**
   - What we know: One bar (4/4 at any tempo) is standard for chord pad plugins. 480 TPQN is the de facto standard resolution.
   - What's unclear: Whether the user expects tempo-synced duration or a fixed duration. Since ChordPumper doesn't sync to host tempo yet, fixed is simpler.
   - Recommendation: Default to 1 bar (1920 ticks at 480 TPQN). The tempo meta-event in the MIDI file determines playback BPM — include a 120 BPM tempo event for predictable import behavior.

3. **Should progression strip chords use voice-led voicings or root-position?**
   - What we know: When the user clicks pads, `optimalVoicing()` produces voice-led MIDI notes. The progression strip could store either the abstract `Chord` or the `VoicedChord`.
   - What's unclear: Whether export from the progression strip (future CAPT-04, v2) should preserve the exact voicings played or normalize to root position.
   - Recommendation: Store `VoicedChord` (chord + midiNotes) in the progression model. For CAPT-01 (individual pad drag), use `Chord::midiNotes(octave)` (root position). The voiced data is available for future progression export.

4. **Temp file lifecycle management strategy**
   - What we know: Each drag creates a temp `.mid` file. The completion callback signals drag end.
   - What's unclear: How long Bitwig takes to copy/read the dropped file. Whether the callback fires before or after the DAW completes its read.
   - Recommendation: Use delayed cleanup (2-second timer after callback). Also clean up any stale temp files on plugin startup/shutdown.

## Sources

### Primary (HIGH confidence)
- `juce_MidiFile.h` (JUCE 8.0.12 source, local) — MidiFile API: `writeTo()`, `setTicksPerQuarterNote()`, `addTrack()`
- `juce_MidiMessageSequence.h` (JUCE 8.0.12 source, local) — `addEvent()`, `updateMatchedPairs()`
- `juce_MidiMessage.h` (JUCE 8.0.12 source, local) — `noteOn()`, `noteOff()` static factories
- `juce_DragAndDropContainer.h` (JUCE 8.0.12 source, local) — `performExternalDragDropOfFiles()`, `shouldDropFilesWhenDraggedExternally()`
- `juce_DragAndDrop_linux.cpp` (JUCE 8.0.12 source, local) — `X11DragState`, XDND protocol implementation, `externalDragInit()`
- `juce_Windowing_linux.cpp` (JUCE 8.0.12 source, local) — `performExternalDragDropOfFiles` Linux impl, `getPeerForDragEvent()`
- `juce_File.h` (JUCE 8.0.12 source, local) — `createTempFile()`, `SpecialLocationType::tempDirectory`
- JUCE Tutorial: Create MIDI data (https://juce.com/tutorials/tutorial_midi_message/) — MidiMessage creation patterns
- Bitwig Userguide: Inserting arranger clips (https://www.bitwig.com/userguide/bws44-504/inserting_and_working_with_arranger_clips/) — Drag from file manager works on Linux

### Secondary (MEDIUM confidence)
- JUCE Forum: Implementing drag & drop from JUCE to DAW (https://forum.juce.com/t/implementing-drag-drop-from-juce-to-daw/55905) — Confirms DnD-to-DAW is a common JUCE use case
- JUCE Forum: External Drag & Drop does not work on Linux (https://forum.juce.com/t/external-drag-drop-does-not-work-on-linux/25819) — Confirms Linux DnD issues
- JUCE Forum: Bug + Patch: DnD for plugin windows on Linux (https://forum.juce.com/t/bug-patch-implements-drag-and-drop-for-plugin-windows-on-linux/35203/4) — Confirms reparented window issues
- JUCE docs: TemporaryFile class (https://docs.juce.com/master/classjuce_1_1TemporaryFile.html) — Atomic temp file pattern

### Tertiary (LOW confidence)
- Community reports that Scaler/Cthulhu-style plugins use temp file + `performExternalDragDropOfFiles` pattern — unverified implementation details, but the general pattern is widely adopted

## Metadata

**Confidence breakdown:**
- MIDI file creation: **HIGH** — API verified from JUCE 8.0.12 source code in local repo. `MidiFile::writeTo()`, `MidiMessageSequence::addEvent()`, tick-based timestamps are well-documented.
- Drag-and-drop (general pattern): **HIGH** — `performExternalDragDropOfFiles` API verified from source. The callback pattern and usage constraints are clear from the implementation.
- Drag-and-drop (Linux/embedded feasibility): **LOW** — Multiple sources confirm this is problematic, but no definitive "works/doesn't work in Bitwig on X11 with JUCE 8" confirmation. This MUST be spiked.
- Progression strip UI: **HIGH** — Standard JUCE component pattern, no exotic APIs needed.
- Fallback approach: **MEDIUM** — File export is trivial; user workflow (drag from file manager to Bitwig) is confirmed by Bitwig docs.

**Research date:** 2026-02-19
**Valid until:** 2026-03-19 (30 days — JUCE and X11 DnD landscape are stable)
