# Architecture Research

**Domain:** JUCE C++ Audio Plugin (MIDI)
**Researched:** 2026-02-19
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  DAW Host                                                           │
│                                                                     │
│  ┌──────────────────── AUDIO THREAD ──────────────────────────┐     │
│  │                                                            │     │
│  │   MIDI In ──► ChordGridProcessor::processBlock()           │     │
│  │                  │                                         │     │
│  │                  ├─► MidiRouter (map input to pad/pass)    │     │
│  │                  │       │                                 │     │
│  │                  │       ▼                                 │     │
│  │                  │   ChordVoicer (build chord + voice lead)│     │
│  │                  │       │                                 │     │
│  │                  │       ▼                                 │     │
│  │                  └─► MIDI Out buffer                       │     │
│  │                                                            │     │
│  │   Shared State (lock-free):                                │     │
│  │     • gridState[] ◄── atomic/snapshot from message thread  │     │
│  │     • lastPlayedChord ──► flag to message thread           │     │
│  │     • parameterValues ◄──► APVTS (atomic internally)       │     │
│  │                                                            │     │
│  └────────────────────────────────────────────────────────────┘     │
│                           ▲  lock-free  ▼                           │
│  ┌──────────────────── MESSAGE THREAD ────────────────────────┐     │
│  │                                                            │     │
│  │   ChordGridEditor                                          │     │
│  │     │                                                      │     │
│  │     ├─► GridPanel (8×4 pads, click → trigger chord)        │     │
│  │     │     └─► DragSource (per-pad MIDI drag-to-DAW)        │     │
│  │     │                                                      │     │
│  │     ├─► InfoPanel (current chord, scale, key display)      │     │
│  │     │                                                      │     │
│  │     └─► Timer (30 Hz poll → repaint grid highlights)       │     │
│  │                                                            │     │
│  │   MorphEngine (recalculate 32 chords after chord played)   │     │
│  │     └─► MusicTheoryEngine (intervals, scales, harmony)     │     │
│  │                                                            │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Thread | Communicates With |
|-----------|---------------|--------|-------------------|
| **ChordGridProcessor** | AudioProcessor subclass. Owns all state, handles processBlock, state save/restore | Audio | MidiRouter, ChordVoicer, Editor (via APVTS + lock-free) |
| **ChordGridEditor** | AudioProcessorEditor subclass. Owns GUI tree, Timer-based repaint | Message | Processor (reads state), GridPanel, InfoPanel |
| **MidiRouter** | Maps incoming MIDI to pad triggers or passes through | Audio | ChordVoicer |
| **ChordVoicer** | Looks up chord from grid, applies voice leading, writes MIDI out | Audio | VoiceLeading, grid state snapshot |
| **MusicTheoryEngine** | Pure logic: intervals, scales, chord construction, Roman numerals | Either (stateless) | MorphEngine, ChordVoicer |
| **MorphEngine** | Calculates next 32 chords for grid after a chord is played | Message | MusicTheoryEngine, grid state |
| **VoiceLeading** | Minimizes note movement between consecutive chords | Audio (fast path) | ChordVoicer |
| **GridPanel** | 8×4 interactive pad Component with chord labels + Roman numerals | Message | Editor, DragSource |
| **DragSource** | Writes MidiFile to temp, calls performExternalDragDropOfFiles | Message | GridPanel, MusicTheoryEngine |
| **InfoPanel** | Displays current key, scale, last played chord | Message | Editor |

### Data Flow

```
MIDI Note On (C3 = pad 0)
    │
    ▼
processBlock() ── audio thread ──────────────────────────────
    │
    ├─► MidiRouter: is this a mapped pad trigger?
    │     YES ──► look up gridState[padIndex]
    │               │
    │               ▼
    │             ChordVoicer: build chord notes from ChordDefinition
    │               │
    │               ├─► MusicTheoryEngine::buildChord(root, quality)
    │               │     returns: {60, 64, 67} (C major)
    │               │
    │               ├─► VoiceLeading::minimize(previousNotes, newNotes)
    │               │     returns: {60, 64, 67} (optimized voicing)
    │               │
    │               └─► write Note On messages to output MidiBuffer
    │
    │     NO ──► pass MIDI through unchanged
    │
    ├─► set lastPlayedPad atomically (for GUI highlight)
    ├─► set morphTriggerFlag atomically (for MorphEngine)
    │
    ▼
MIDI Out (chord notes)

────────── message thread (async) ───────────────────────────

Timer::timerCallback() @ 30 Hz
    │
    ├─► read lastPlayedPad ──► highlight active pad in GridPanel
    ├─► read morphTriggerFlag
    │     │
    │     ▼ (if set)
    │   MorphEngine::recalculate(playedChord, currentGrid, key, scale)
    │     │
    │     ├─► MusicTheoryEngine (diatonic chords, secondary dominants, ...)
    │     ├─► proximity scoring (voice leading distance from played chord)
    │     │
    │     ▼
    │   new ChordDefinition[32]
    │     │
    │     ▼
    │   publish to gridState via lock-free snapshot swap
    │
    └─► repaint() GridPanel with updated chord names + numerals
```

## Recommended Project Structure

```
ChordGrid/
├── CMakeLists.txt                 # Top-level: project(), find JUCE, juce_add_plugin()
├── JUCE/                          # JUCE framework (git submodule)
├── lib/
│   └── clap-juce-extensions/      # CLAP support (git submodule)
│
├── Source/
│   ├── PluginProcessor.h/.cpp     # ChordGridProcessor : AudioProcessor
│   ├── PluginEditor.h/.cpp        # ChordGridEditor : AudioProcessorEditor
│   │
│   ├── Audio/                     # Audio-thread-safe components
│   │   ├── MidiRouter.h/.cpp      # Map MIDI input to pad triggers
│   │   └── ChordVoicer.h/.cpp     # Build + voice-lead chord output
│   │
│   ├── Engine/                    # Pure logic (no JUCE GUI deps, independently testable)
│   │   ├── MusicTheoryEngine.h/.cpp  # Intervals, scales, chord quality, Roman numerals
│   │   ├── MorphEngine.h/.cpp     # Grid recalculation algorithm
│   │   └── VoiceLeading.h/.cpp    # Minimal voice movement algorithm
│   │
│   ├── Model/                     # Data structures
│   │   ├── ChordDefinition.h      # Root, quality, bass note, inversion
│   │   ├── GridState.h            # Array of 32 ChordDefinitions + metadata
│   │   ├── ScaleDefinition.h      # Scale type + degree mappings
│   │   └── NoteSet.h              # Compact note representation (bitfield or vector)
│   │
│   ├── UI/                        # GUI components (message thread only)
│   │   ├── GridPanel.h/.cpp       # 8×4 pad grid component
│   │   ├── PadComponent.h/.cpp    # Single pad (label, color, click, drag)
│   │   ├── InfoPanel.h/.cpp       # Key/scale/chord info display
│   │   ├── DragSource.h/.cpp      # MIDI file creation + external drag
│   │   └── LookAndFeel.h/.cpp     # Custom styling
│   │
│   └── Util/                      # Shared utilities
│       ├── LockFreeState.h        # Snapshot-swapping state container
│       └── Constants.h            # Grid dimensions, MIDI mappings, etc.
│
├── Tests/                         # Unit tests (Catch2 or JUCE UnitTest)
│   ├── CMakeLists.txt
│   ├── MusicTheoryEngineTest.cpp
│   ├── MorphEngineTest.cpp
│   └── VoiceLeadingTest.cpp
│
└── Resources/                     # Binary assets
    └── (fonts, images if needed)
```

**Rationale for separation:**
- `Engine/` has zero JUCE GUI dependencies — just `<cstdint>`, `<array>`, `<vector>`, `<algorithm>`. This makes it unit-testable without a GUI event loop.
- `Audio/` components are designed for real-time safety: no allocations, no locks, no virtual dispatch in hot paths.
- `Model/` structs are plain data — copyable, serializable, used across thread boundaries.
- `UI/` components live entirely on the message thread and never touch the audio path directly.

## Architectural Patterns

### Pattern 1: AudioProcessor / AudioProcessorEditor Split

**What:** The processor handles all non-GUI logic and persists across editor lifetimes. The editor is created/destroyed as the user opens/closes the plugin window. The processor outlives the editor.

**Rules:**
- Editor reads from processor, never the reverse
- Processor must not hold a pointer to the editor (editor may not exist)
- Editor accesses processor via the reference passed to its constructor
- Multiple editors can theoretically exist for one processor

```cpp
class ChordGridProcessor : public juce::AudioProcessor
{
public:
    ChordGridProcessor();
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Exposed for editor to read (thread-safe access)
    const GridState& getGridStateSnapshot() const;
    std::atomic<int> lastPlayedPad { -1 };

private:
    juce::AudioProcessorValueTreeState apvts;
    MidiRouter midiRouter;
    ChordVoicer chordVoicer;
    MorphEngine morphEngine;
    LockFreeState<GridState> gridState;
};
```

### Pattern 2: AudioProcessorValueTreeState (APVTS) for Parameters

**What:** APVTS wraps a ValueTree for thread-safe parameter management with automatic DAW integration (automation, save/restore, undo).

**When:** For every user-controllable parameter that the DAW should know about (key, scale, velocity, octave, etc.).

**Not for:** Complex internal state like the 32-chord grid. Use a separate ValueTree or custom lock-free mechanism for that.

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "key", "Key", juce::StringArray{"C","C#","D",...,"B"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "scale", "Scale", juce::StringArray{"Major","Minor",...}, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "velocity", "Velocity", 1, 127, 100));
    return { params.begin(), params.end() };
}
```

### Pattern 3: MIDI-Only Plugin (No Audio Buses)

**What:** Plugin processes MIDI only — no audio I/O. Constructed with empty BusesProperties.

```cpp
ChordGridProcessor()
    : AudioProcessor(BusesProperties())  // no audio buses
{
}
```

**CMake configuration:**

```cmake
juce_add_plugin(ChordGrid
    PLUGIN_MANUFACTURER_CODE Gsd0
    PLUGIN_CODE Cgrd
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    FORMATS VST3 Standalone
    PRODUCT_NAME "Chord Grid")
```

For CLAP, add clap-juce-extensions (JUCE 8 does not have native CLAP; JUCE 9 will):

```cmake
# After juce_add_plugin:
clap_juce_extensions_plugin(TARGET ChordGrid
    CLAP_ID "com.gsd.chord-grid"
    CLAP_FEATURES "note-effect" "utility")
```

### Pattern 4: Lock-Free Grid State Sharing

**What:** The MorphEngine (message thread) produces a new GridState. The audio thread consumes it. Use a triple-buffer or snapshot-swap pattern — never a mutex.

**Implementation strategy — atomic pointer swap:**

```cpp
template <typename T>
class LockFreeState
{
public:
    // Message thread: publish new state
    void publish(std::unique_ptr<T> newState)
    {
        auto* raw = newState.release();
        auto* old = ready.exchange(raw, std::memory_order_acq_rel);
        if (old) pending.store(old, std::memory_order_release);
    }

    // Audio thread: get latest state (non-blocking)
    const T* acquire()
    {
        auto* latest = ready.exchange(nullptr, std::memory_order_acq_rel);
        if (latest)
        {
            reclaim(current);
            current = latest;
        }
        return current;
    }

private:
    std::atomic<T*> ready { nullptr };
    std::atomic<T*> pending { nullptr };
    T* current { nullptr };

    void reclaim(T* old) { /* return to pool or let pending handle it */ }
};
```

For simpler per-value sharing (last played pad, trigger flags), `std::atomic<int>` suffices.

### Pattern 5: Timer-Based GUI Polling

**What:** The editor inherits from `juce::Timer` and polls processor state at ~30 Hz, avoiding audio-thread callbacks into GUI code.

```cpp
class ChordGridEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    ChordGridEditor(ChordGridProcessor& p)
        : AudioProcessorEditor(p), processor(p)
    {
        startTimerHz(30);
    }

private:
    void timerCallback() override
    {
        int pad = processor.lastPlayedPad.load(std::memory_order_acquire);
        gridPanel.setActivePad(pad);
        // Check if morph completed, update grid display, etc.
    }

    ChordGridProcessor& processor;
    GridPanel gridPanel;
};
```

### Pattern 6: Drag-to-DAW via Temp MIDI File

**What:** Write a standard MIDI file to a temp location, then use JUCE's external drag API to hand it to the DAW.

```cpp
void DragSource::startMidiDrag(const ChordDefinition& chord,
                               const juce::MouseEvent& event)
{
    juce::MidiFile midiFile;
    juce::MidiMessageSequence track;

    auto notes = MusicTheoryEngine::buildChord(chord.root, chord.quality);
    for (auto note : notes)
    {
        track.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)100), 0.0);
        track.addEvent(juce::MidiMessage::noteOff(1, note), 480.0);
    }
    midiFile.addTrack(track);
    midiFile.setTicksPerQuarterNote(480);

    auto tempFile = juce::File::getSpecialLocation(
        juce::File::tempDirectory).getChildFile("chord-drag.mid");
    juce::FileOutputStream stream(tempFile);
    if (stream.openedOk())
    {
        midiFile.writeTo(stream);
        stream.flush();
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            { tempFile.getFullPathName() }, false, this, nullptr);
    }
}
```

## Data Flow

### MIDI Input → Processing → Output

1. DAW calls `processBlock(audioBuffer, midiBuffer)` on the audio thread
2. `midiBuffer` contains incoming MIDI events with sample-accurate timestamps
3. `MidiRouter` iterates events:
   - Note On in mapped range → resolve pad index → `ChordVoicer::trigger(padIndex)`
   - Note Off in mapped range → `ChordVoicer::release(padIndex)`
   - Other MIDI → pass through or discard (configurable)
4. `ChordVoicer` reads `gridState[padIndex]` to get the `ChordDefinition`
5. `MusicTheoryEngine::buildChord()` returns raw pitch set
6. `VoiceLeading::minimize()` adjusts octave placement relative to previous chord
7. Output MIDI messages written to `midiBuffer` (replacing or appending)
8. After `processBlock` returns, DAW sends `midiBuffer` downstream

### UI ↔ Processor Communication

| Direction | Data | Mechanism | Safety |
|-----------|------|-----------|--------|
| **Processor → Editor** | Last played pad index | `std::atomic<int>` | Audio writes, GUI reads |
| **Processor → Editor** | Grid state (32 chords) | `LockFreeState<GridState>` snapshot | Message thread publishes, audio thread acquires |
| **Processor → Editor** | Parameter values (key, scale) | APVTS listeners | Thread-safe by design |
| **Editor → Processor** | Pad click (trigger chord) | Lock-free SPSC queue | GUI pushes, audio pops |
| **Editor → Processor** | Parameter changes | APVTS | Thread-safe by design |
| **Editor → Processor** | Grid state after morph | `LockFreeState<GridState>` publish | Message thread publishes, audio thread acquires |

### State Management

**Automatable parameters** (saved by DAW, visible in automation lanes):
- Key (C through B)
- Scale (Major, Minor, Dorian, Mixolydian, etc.)
- Output velocity
- Output octave offset
- MIDI input channel
- MIDI output channel

**Internal state** (saved in plugin state block via ValueTree):
- Current grid contents (32 ChordDefinitions)
- Morph algorithm settings (theory weight vs proximity weight)
- Last played chord (for morph seed)

**Serialization flow:**

```
getStateInformation():
    APVTS.state → copyState()
    gridState → serialize to child ValueTree
    combine → write as binary to MemoryBlock

setStateInformation():
    read binary → parse ValueTree
    APVTS.replaceState(parametersSubtree)
    gridState ← deserialize from child ValueTree
```

## Thread Safety

### Thread Boundary Rules

| Rule | Rationale |
|------|-----------|
| **Never allocate memory on the audio thread** | `malloc`/`new` may take a lock internally |
| **Never lock a mutex on the audio thread** | Unbounded blocking violates real-time guarantee |
| **Never do I/O on the audio thread** | File/network ops have unbounded latency |
| **Never call virtual functions in hot paths** | Vtable lookup + potential cache miss; use templates or direct calls |
| **Never log on the audio thread** | Logging typically involves allocation + I/O |
| **All GUI operations on the message thread** | JUCE Components are not thread-safe |

### Safe Communication Mechanisms

| Mechanism | Direction | Use Case |
|-----------|-----------|----------|
| `std::atomic<T>` | Bidirectional | Single values: flags, indices, simple parameters |
| `juce::AbstractFifo` + ring buffer | SPSC | Streams of events (pad triggers from GUI → audio) |
| Lock-free snapshot swap | Message → Audio | Complex state (GridState with 32 chord definitions) |
| `juce::AsyncUpdater` | Audio → Message | Trigger GUI callback from audio (use sparingly — may touch system message queue) |
| `juce::Timer` polling | Audio → Message (indirect) | GUI reads atomic/shared state at 30 Hz |
| APVTS | Bidirectional | DAW-visible parameters (uses atomics internally) |

### MorphEngine Thread Placement

The MorphEngine is computationally expensive (scoring 32 candidates against theory + proximity). It runs on the **message thread**, triggered when the Timer detects a new chord was played. The result is published to the audio thread via `LockFreeState<GridState>`.

If morph computation exceeds ~16ms, consider offloading to a background thread with the result published the same way. Do **not** run it on the audio thread.

### Audio Thread Budget

At 48 kHz sample rate with 512-sample buffer: **~10.67ms** per processBlock call. The audio thread work (MidiRouter scan + chord lookup + voice leading) should complete in **< 1ms**. This is achievable because:
- MidiRouter: linear scan of typically 1-3 MIDI events
- Chord lookup: array index into GridState (O(1))
- VoiceLeading: sort + octave shift of 3-6 notes (O(n log n), n ≤ 6)

## Anti-Patterns

### Anti-Pattern 1: Mutex Between Audio and GUI

**What:** Using `std::mutex` or `juce::CriticalSection` to protect shared state accessed by both threads.
**Why bad:** The audio thread can block waiting for the GUI thread, causing audio dropouts. Priority inversion means the audio thread (high priority) waits for the GUI thread (low priority).
**Instead:** Lock-free atomics, SPSC queues, or snapshot-swap patterns as described above.

### Anti-Pattern 2: Processor Holding Editor Pointer

**What:** Storing a raw pointer to the editor in the processor and calling editor methods from processBlock.
**Why bad:** The editor's lifetime is unpredictable — the user can close the plugin window at any time. Dangling pointer access causes crashes.
**Instead:** Processor exposes state; editor polls. One-way data flow: editor reads processor, not vice versa.

### Anti-Pattern 3: Allocating in processBlock

**What:** Creating `std::vector`, `juce::String`, `juce::Array`, or any heap-allocated object inside processBlock.
**Why bad:** Heap allocation may block on a system-wide lock, breaking real-time guarantees.
**Instead:** Pre-allocate all buffers in `prepareToPlay()`. Use fixed-size arrays (`std::array`) for chord note sets. Pool reusable objects.

### Anti-Pattern 4: Complex Music Theory in Audio Thread

**What:** Running scale lookups, chord quality analysis, or morph calculations inside processBlock.
**Why bad:** Unpredictable computation time risks exceeding the audio buffer deadline.
**Instead:** Pre-compute everything on the message thread. The audio thread only reads pre-computed `ChordDefinition` structs from the grid state snapshot. Chord → note conversion should be a simple lookup table, not a computation.

### Anti-Pattern 5: Calling repaint() from Audio Thread

**What:** Directly calling `Component::repaint()` from processBlock to update the GUI when a note plays.
**Why bad:** `repaint()` posts to the message queue, which may involve system calls that block.
**Instead:** Set an `std::atomic` flag in processBlock. Timer on the message thread checks the flag and calls `repaint()`.

## Integration Points

### DAW Integration (VST3)

- **Parameter automation:** APVTS parameters are exposed to the DAW via standard VST3 parameter interface
- **State save/restore:** `getStateInformation()` / `setStateInformation()` called by DAW for preset and session save
- **MIDI routing:** DAW routes MIDI to plugin input; plugin output goes to downstream instruments or tracks
- **Transport:** `getPlayHead()->getPosition()` available in processBlock for tempo/position sync (useful if morph algorithm considers musical context)

### DAW Integration (CLAP via clap-juce-extensions)

- Same AudioProcessor code, different wrapper binary
- `clap-juce-extensions` hooks into JUCE's CMake build to produce `.clap` binaries
- Feature parity with VST3 for MIDI effect plugins
- When JUCE 9 ships native CLAP support, migration is straightforward: remove clap-juce-extensions, add CLAP to FORMATS list

### Standalone Mode

- JUCE Standalone wrapper provides its own audio device manager and MIDI device selection
- Useful for development and testing without a DAW
- Same AudioProcessor code runs unchanged

## Build Order (Dependency Graph)

```
Phase 1: Skeleton
   CMake + PluginProcessor + PluginEditor (empty) + builds as VST3/Standalone

Phase 2: Music Theory Engine (no JUCE GUI deps)
   MusicTheoryEngine + ChordDefinition + ScaleDefinition + NoteSet
   ├── Unit tests for intervals, chord building, Roman numerals
   └── Can be developed and tested entirely without a running plugin

Phase 3: MIDI Processing
   MidiRouter + ChordVoicer (basic, no voice leading yet)
   ├── Depends on: Phase 1 (processBlock) + Phase 2 (chord building)
   └── Testable: load plugin, send MIDI, verify output

Phase 4: Grid UI (static)
   GridPanel + PadComponent + LookAndFeel
   ├── Depends on: Phase 1 (editor shell) + Phase 2 (ChordDefinition for labels)
   └── Static display: hardcoded 32 chords, click triggers MIDI

Phase 5: Lock-Free State + Wiring
   LockFreeState + Timer polling + APVTS parameters
   ├── Depends on: Phase 3 + Phase 4
   └── Audio thread and GUI thread communicate properly

Phase 6: Morph Engine
   MorphEngine (theory + proximity scoring)
   ├── Depends on: Phase 2 (theory) + Phase 5 (state sharing)
   └── Grid updates dynamically after each chord played

Phase 7: Voice Leading
   VoiceLeading algorithm integrated into ChordVoicer
   ├── Depends on: Phase 3 (ChordVoicer) + Phase 2 (note sets)
   └── Smooth transitions between consecutive chords

Phase 8: Drag-to-DAW
   DragSource + MidiFile writing
   ├── Depends on: Phase 4 (GridPanel) + Phase 2 (chord building)
   └── Independent feature, can be built in parallel with Phase 6/7

Phase 9: State Persistence
   getStateInformation / setStateInformation with full grid + parameters
   ├── Depends on: Phase 5 (APVTS) + Phase 6 (grid state)
   └── Session recall and preset support

Phase 10: CLAP Support
   clap-juce-extensions integration
   ├── Depends on: Phase 1 (CMake)
   └── Largely a build config task, can be done early or late
```

**Critical path:** Phases 1 → 2 → 3 → 5 → 6 (skeleton → theory → MIDI → wiring → morph)

**Parallelizable:** Phase 4 (Grid UI) can start as soon as Phase 2 is done. Phase 8 (Drag) can start after Phase 4. Phase 10 (CLAP) can be done anytime after Phase 1.

## Sources

- JUCE AudioProcessor class reference — https://docs.juce.com/master/classAudioProcessor.html (HIGH confidence)
- JUCE AudioProcessorEditor class reference — https://docs.juce.com/master/classAudioProcessorEditor.html (HIGH confidence)
- JUCE AudioProcessorValueTreeState docs — https://docs.juce.com/master/classAudioProcessorValueTreeState.html (HIGH confidence)
- JUCE tutorial: Create a basic Audio/MIDI plugin, Part 2 — https://docs.juce.com/master/tutorial_code_basic_plugin.html (HIGH confidence)
- JUCE tutorial: Plugin examples (Arpeggiator) — https://docs.juce.com/master/tutorial_plugin_examples.html (HIGH confidence)
- JUCE AbstractFifo class reference — https://docs.juce.com/master/juce__AbstractFifo_8h.html (HIGH confidence)
- JUCE AsyncUpdater class reference — https://docs.juce.com/master/classjuce_1_1AsyncUpdater.html (HIGH confidence)
- JUCE Timer class reference — https://docs.juce.com/master/classTimer.html (HIGH confidence)
- JUCE MidiFile class reference — https://docs.juce.com/master/classMidiFile.html (HIGH confidence)
- JUCE CMake AudioPlugin example — https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt (HIGH confidence)
- JUCE tutorial: Saving and loading plugin state — https://docs.juce.com/master/tutorial_audio_processor_value_tree_state.html (HIGH confidence)
- clap-juce-extensions — https://github.com/free-audio/clap-juce-extensions (HIGH confidence)
- JUCE Roadmap Q3 2025 (CLAP in JUCE 9) — https://juce.com/blog/juce-roadmap-update-q3-2025/ (HIGH confidence)
- Lock-free audio patterns — https://timur.audio/using-locks-in-real-time-audio-processing-safely (MEDIUM confidence)
- Memory ordering for real-time audio — https://www.bruce.audio/post/2025/02/24/memory_ordering/ (MEDIUM confidence)
- JUCE forum: drag & drop MIDI to DAW — https://forum.juce.com/t/implementing-drag-drop-from-juce-to-daw/55905 (MEDIUM confidence)
- JUCE CMake template (anthonyalfimov) — https://github.com/anthonyalfimov/JUCE-CMake-Plugin-Template (MEDIUM confidence)
- Scruffy Cat Studios CMake template — https://scruffycatstudios.com/posts/juce-cmake-template/ (MEDIUM confidence)
