# Phase 3: Playable Grid - Research

**Researched:** 2026-02-19
**Domain:** JUCE 8 grid UI components, MIDI output pipeline, lock-free audio/GUI thread communication
**Confidence:** HIGH

## Summary

Phase 3 transforms the ChordPumper shell into a playable instrument. The user sees an 8×4 grid of 32 labeled chord pads, populated with a chromatic palette on first load, and clicking any pad sends the correct MIDI chord to downstream instruments in Bitwig. This phase spans three technical domains: UI layout (grid of custom pad components), MIDI output (note-on/note-off through `processBlock`), and thread-safe GUI→audio communication (the pad click on the message thread must produce MIDI events on the real-time audio thread).

JUCE provides all the building blocks. `juce::Grid` (CSS Grid-inspired layout engine) handles the 8×4 arrangement declaratively with `Fr` fractional tracks. For MIDI bridging, `juce::MidiKeyboardState` is the standard lock-free solution — its `noteOn()`/`noteOff()` methods can be called from the GUI thread, and `processNextMidiBuffer()` injects the corresponding MIDI events into the audio thread's `MidiBuffer` during `processBlock`. This is the same pattern used by JUCE's own `MidiKeyboardComponent` and avoids the need for a custom `AbstractFifo` or atomic queue. For note-off timing, a `juce::Timer` on the GUI thread fires after a configurable duration and calls `MidiKeyboardState::noteOff()`.

The chromatic palette (32 chords: 12 major, 12 minor, 8 fills) maps directly to the existing `Chord` engine from Phase 2. Each pad stores a `Chord` value and uses `Chord::name()` for its label and `Chord::midiNotes(octave)` for MIDI output. The `ChordPumperEngine` library already provides everything needed — no new engine code is required for this phase.

**Primary recommendation:** Use `MidiKeyboardState` as the lock-free GUI→audio MIDI bridge. Build `PadComponent` as a custom `juce::Component` with mouse handling (not `TextButton`). Use `juce::Grid` for layout. Use `juce::Timer` for note-off scheduling.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| GRID-01 | User can see an 8x4 grid of 32 chord pads on the plugin UI | `juce::Grid` with 8 `Fr(1)` columns × 4 `Fr(1)` rows. `PadComponent` as child `Component` with custom paint. `GridItem` wraps each pad. Layout in `resized()` via `grid.performLayout(getLocalBounds())`. |
| GRID-02 | Grid starts with a chromatic palette (all 12 major, all 12 minor, plus diminished/augmented/sus fills) before any selection | Static palette array of 32 `Chord` values using existing `PitchClass` and `ChordType` from engine. First 12: C–B major. Next 12: C–B minor. Last 8: Cdim, C#dim, Daug, Ebaug, Fdim, F#dim, Gaug, Abaug (or similar fill pattern). |
| MIDI-01 | Clicking a pad sends that chord's MIDI notes to the plugin's MIDI output (downstream instruments in DAW) | `MidiKeyboardState::noteOn()` called from GUI thread on pad `mouseDown`. `processNextMidiBuffer()` called in `processBlock` injects MIDI into output `MidiBuffer`. Bitwig routes MIDI downstream through device chain. |
| MIDI-02 | MIDI output includes proper note-on/note-off messages with configurable velocity | `MidiKeyboardState::noteOn(channel, note, velocity)` takes float velocity 0.0–1.0. Note-off via `MidiKeyboardState::noteOff()` triggered by `juce::Timer` after configurable duration (default ~300ms). Velocity stored as member variable, configurable for future UI control. |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 | Grid layout, Component, MidiKeyboardState, MidiBuffer, Timer | Already in project. Grid/Component/Timer are core JUCE modules. MidiKeyboardState is the standard GUI→audio MIDI bridge. |
| ChordPumperEngine | (local) | Chord, PitchClass, ChordType | Already built in Phase 2. Provides chord construction, naming, MIDI note generation. |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce::Grid | (in juce_gui_basics) | 8×4 pad layout | In `GridPanel::resized()` to position 32 pad components |
| juce::MidiKeyboardState | (in juce_audio_basics) | Lock-free GUI→audio MIDI bridge | Shared between Processor and Editor. GUI calls noteOn/noteOff, processBlock reads via processNextMidiBuffer. |
| juce::Timer | (in juce_events) | Note-off scheduling | GridPanel inherits Timer, fires after note duration to send noteOff for all chord tones. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| MidiKeyboardState | Custom AbstractFifo-based MIDI queue | AbstractFifo is lower-level — need to manage buffer, wrap-around, event serialization. MidiKeyboardState does all this internally and is battle-tested. Only consider AbstractFifo if MidiKeyboardState proves limiting (e.g., needs CC messages). |
| MidiKeyboardState | std::atomic flag + MidiBuffer swap | Fragile, requires careful memory ordering. MidiKeyboardState is designed for exactly this use case. |
| juce::Grid layout | Manual Rectangle::removeFromLeft/Top arithmetic | Grid is cleaner for uniform grids. Rectangle arithmetic works but gets verbose for 32 items. Grid provides gap control and fractional sizing. |
| Custom PadComponent | juce::TextButton | TextButton's LookAndFeel is designed for standard buttons, not DAW pads. Custom Component gives full control over painting (rounded rect, highlight states, label positioning). Phase 4 will add Roman numeral sublabels — custom painting is essential. |
| Timer-based note-off | mouseUp-triggered note-off | mouseUp works for hold-to-play but doesn't give consistent note durations. Timer gives predictable timing. Can switch to mouseUp in future for "hold" mode. |

## Architecture Patterns

### Recommended Project Structure

```
src/
├── engine/                    # (Phase 2 — unchanged)
│   ├── NoteLetter.h
│   ├── PitchClass.h / .cpp
│   ├── ChordType.h
│   └── Chord.h / .cpp
├── ui/
│   ├── PluginEditor.h / .cpp  # Updated: owns GridPanel, wires to Processor
│   ├── ChordPumperLookAndFeel.h  # Updated: pad colors
│   ├── GridPanel.h / .cpp     # NEW: 8×4 grid container, owns 32 PadComponents
│   └── PadComponent.h / .cpp  # NEW: single chord pad with painting + mouse
├── midi/
│   └── ChromaticPalette.h     # NEW: static function returning 32 Chord palette
└── PluginProcessor.h / .cpp   # Updated: owns MidiKeyboardState, processBlock outputs MIDI
```

### Pattern 1: MidiKeyboardState as Lock-Free MIDI Bridge

**What:** Use JUCE's built-in `MidiKeyboardState` to safely pass MIDI events from the GUI thread to the audio thread without locks or custom queues.

**When to use:** Any time a GUI action (pad click) needs to produce MIDI output in `processBlock`.

**How it works:**
1. `MidiKeyboardState` is owned by the Processor (lives for the plugin's lifetime)
2. Editor receives a reference to it
3. GUI thread calls `keyboardState.noteOn(channel, noteNumber, velocity)` — internally queues the event
4. Audio thread calls `keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true)` — dequeues events into the MidiBuffer
5. The `injectIndirectEvents = true` parameter causes GUI-triggered notes to appear in the output buffer

**Example:**
```cpp
// In PluginProcessor.h
class ChordPumperProcessor : public juce::AudioProcessor
{
public:
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

private:
    juce::MidiKeyboardState keyboardState;
};

// In PluginProcessor.cpp — processBlock
void ChordPumperProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
}
```

**Why MidiKeyboardState and not a custom queue:**
- It's the official JUCE pattern for this exact use case
- Internally uses a lock-free queue (`CriticalSection`-guarded pending list that's drained on the audio thread)
- Handles note tracking (knows which notes are currently on)
- `allNotesOff()` provides panic functionality
- Used by MidiKeyboardComponent, JUCE's own synth tutorials, and hundreds of production plugins

### Pattern 2: Custom PadComponent with Mouse Handling

**What:** A lightweight `juce::Component` subclass representing a single chord pad with custom painting and mouse event handling.

**When to use:** Each of the 32 grid cells.

**Example:**
```cpp
class PadComponent : public juce::Component
{
public:
    using ClickCallback = std::function<void(const Chord&)>;

    void setChord(const Chord& chord);
    void setClickCallback(ClickCallback callback);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    Chord chord;
    ClickCallback onClick;
    bool isPressed = false;
    bool isHovered = false;
};
```

**Paint states:**
- **Normal:** Dark background (from LookAndFeel scheme), chord name centered
- **Hovered:** Slightly lighter background
- **Pressed:** Accent color background (the `defaultFill` blue from the colour scheme)

**Why custom Component instead of TextButton:**
- Full control over visual appearance (rounded rect pads, multi-line labels for Phase 4 Roman numerals)
- No need for Button's toggle/radio/auto-repeat infrastructure
- Simpler state model (pressed/hovered/normal)
- Easier to extend for Phase 4 (sub-labels, color coding by chord type)

### Pattern 3: juce::Grid for 8×4 Layout

**What:** Declarative CSS Grid-style layout for positioning 32 pad components uniformly.

**When to use:** In `GridPanel::resized()` to lay out all pads.

**Example:**
```cpp
void GridPanel::resized()
{
    juce::Grid grid;
    grid.setGap(juce::Grid::Px(4));

    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    // 8 equal columns
    for (int i = 0; i < 8; ++i)
        grid.templateColumns.add(Track(Fr(1)));

    // 4 equal rows
    for (int i = 0; i < 4; ++i)
        grid.templateRows.add(Track(Fr(1)));

    for (auto& pad : pads)
        grid.items.add(juce::GridItem(pad));

    grid.performLayout(getLocalBounds());
}
```

**Layout order:** Items fill left-to-right, top-to-bottom (AutoFlow::row default). Pad 0 = top-left, Pad 7 = top-right, Pad 8 = second row left, etc.

### Pattern 4: Timer-Based Note-Off Scheduling

**What:** Use `juce::Timer` to send note-off events after a fixed duration, giving consistent chord playback length.

**When to use:** After a pad click triggers note-on events.

**Example:**
```cpp
class GridPanel : public juce::Component, private juce::Timer
{
    void padClicked(const Chord& chord)
    {
        releaseCurrentChord();

        auto notes = chord.midiNotes(defaultOctave);
        for (int note : notes)
            keyboardState.noteOn(midiChannel, note, velocity);

        activeNotes = notes;
        startTimer(noteDurationMs);  // default 300ms
    }

    void timerCallback() override
    {
        releaseCurrentChord();
        stopTimer();
    }

    void releaseCurrentChord()
    {
        for (int note : activeNotes)
            keyboardState.noteOff(midiChannel, note, 0.0f);
        activeNotes.clear();
    }
};
```

**Duration choice:** 300ms is a good default for chord preview. Long enough to hear the chord clearly, short enough to feel responsive. Configurable via member variable for future UI control.

### Pattern 5: Chromatic Palette Population

**What:** A static function that returns the initial 32-chord palette for the grid.

**When to use:** Grid initialization (first load, before any user interaction).

**Palette layout (32 chords in 8×4 grid):**

```
Row 0: C    C#   D    Eb   E    F    F#   G     (Major)
Row 1: Ab   A    Bb   B    Cm   C#m  Dm   Ebm   (Major cont. + Minor start)
Row 2: Em   Fm   F#m  Gm   Abm  Am   Bbm  Bm    (Minor cont.)
Row 3: Cdim Ddim Edim F#dim Caug Ebaug Gaug Bbaug (Fills: dim + aug)
```

**Example:**
```cpp
namespace chordpumper {

inline std::array<Chord, 32> chromaticPalette()
{
    using namespace pitches;
    using CT = ChordType;
    return {{
        // Row 0: Major chords C through G
        {C, CT::Major}, {Cs, CT::Major}, {D, CT::Major}, {Eb, CT::Major},
        {E, CT::Major}, {F, CT::Major},  {Fs, CT::Major}, {G, CT::Major},
        // Row 1: Major Ab-B, then Minor C-Eb
        {Ab, CT::Major}, {A, CT::Major}, {Bb, CT::Major}, {B, CT::Major},
        {C, CT::Minor},  {Cs, CT::Minor}, {D, CT::Minor}, {Eb, CT::Minor},
        // Row 2: Minor E through B
        {E, CT::Minor},  {F, CT::Minor},  {Fs, CT::Minor}, {G, CT::Minor},
        {Ab, CT::Minor}, {A, CT::Minor},  {Bb, CT::Minor}, {B, CT::Minor},
        // Row 3: Fills — diminished + augmented
        {C, CT::Diminished}, {D, CT::Diminished}, {E, CT::Diminished}, {Fs, CT::Diminished},
        {C, CT::Augmented},  {Eb, CT::Augmented}, {G, CT::Augmented},  {Bb, CT::Augmented},
    }};
}

} // namespace chordpumper
```

**Diminished fill rationale:** Only 3 unique diminished chords exist (C/Eb/F#/A are enharmonically equivalent, etc.). Show 4 representative roots: C, D, E, F#. This covers all 12 pitch classes across 4 diminished groups.

**Augmented fill rationale:** Only 4 unique augmented chords exist (C/E/Ab are equivalent, etc.). Show 4 representative roots: C, Eb, G, Bb. This covers all 12 pitch classes across 4 augmented groups.

### Anti-Patterns to Avoid

- **Don't use locks (mutex/CriticalSection) between GUI and audio threads:** Locks cause priority inversion and audio dropouts. MidiKeyboardState's internal mechanism is lock-free for the audio thread.
- **Don't create MidiMessage objects on the audio thread:** MidiKeyboardState handles this internally. The audio thread only calls `processNextMidiBuffer()`.
- **Don't allocate memory on the audio thread:** `std::vector`, `std::string`, `new` — all forbidden in `processBlock`. The MIDI bridge pattern avoids this entirely.
- **Don't put the Timer on the audio thread:** `juce::Timer` runs on the message thread. `juce::HighResolutionTimer` runs on its own thread. Neither should be called from the audio thread. Timer callbacks are for GUI-initiated actions (note-off scheduling).
- **Don't store chords as MIDI note vectors:** Preserve `Chord{root, type}` identity. MIDI notes are derived on demand. Phase 4's morph engine needs chord identity, not raw note lists.
- **Don't use TextButton for pads:** TextButton's painting is designed for standard UI buttons. Custom `Component::paint()` gives full control for the pad aesthetic and future multi-line labels.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| GUI→audio MIDI bridge | Custom AbstractFifo + MidiMessage queue | `juce::MidiKeyboardState` | Built-in, battle-tested, handles note tracking, panic (allNotesOff), and event injection. Designed for exactly this use case. |
| Grid layout | Manual Rectangle subdivision math | `juce::Grid` with `Fr(1)` tracks | Declarative, handles gaps, fractional sizing, CSS Grid semantics. Grid is purpose-built for uniform layouts. |
| MIDI message creation | Raw byte construction | `juce::MidiMessage::noteOn()` / `noteOff()` static factories | Type-safe, handles channel encoding, velocity scaling. Raw bytes invite off-by-one and channel encoding bugs. |
| Note-off timing | Manual sample counting in processBlock | `juce::Timer` on message thread calling `MidiKeyboardState::noteOff()` | Timer handles scheduling without polluting audio thread logic. processBlock stays minimal. |
| Chord data for pads | New chord representation | Existing `Chord` struct from `ChordPumperEngine` | Already has root, type, name(), midiNotes(). No new types needed. |

**Key insight:** Phase 3 is primarily a wiring exercise — connecting existing engine types (Chord) to existing JUCE infrastructure (Grid, MidiKeyboardState, Timer, Component). The only new code is the thin UI layer (PadComponent, GridPanel) and the chromatic palette definition.

## Common Pitfalls

### Pitfall 1: Calling processNextMidiBuffer with injectIndirectEvents = false
**What goes wrong:** Pad clicks produce no MIDI output.
**Why it happens:** `processNextMidiBuffer(buffer, start, numSamples, false)` — the `false` parameter means GUI-triggered noteOn/noteOff events are NOT injected into the buffer.
**How to avoid:** Always pass `true` for `injectIndirectEvents` when calling `processNextMidiBuffer` in `processBlock`.
**Warning signs:** Pad highlights on click but no sound from downstream instrument.

### Pitfall 2: Note-Off Never Sent (Stuck Notes)
**What goes wrong:** Chord notes sustain indefinitely after pad click.
**Why it happens:** Timer not started after noteOn, or timer callback doesn't call noteOff for all chord tones, or activeNotes list not properly maintained.
**How to avoid:** Always track which MIDI notes are currently active. Release ALL active notes before triggering new ones. Implement `releaseCurrentChord()` as a separate method called both by the timer and before any new chord trigger. Add `allNotesOff()` as a panic mechanism in the destructor.
**Warning signs:** Notes keep playing after clicking different pads rapidly.

### Pitfall 3: MIDI Channel Mismatch
**What goes wrong:** Bitwig doesn't route the MIDI to downstream instruments.
**Why it happens:** Using MIDI channel 0 (invalid — JUCE channels are 1-16) or a channel that doesn't match the downstream instrument's receive channel.
**How to avoid:** Use MIDI channel 1 as default. JUCE's `MidiKeyboardState::noteOn()` expects channel 1-16. Bitwig's default instrument channel is 1.
**Warning signs:** MIDI activity shows in plugin but downstream instrument doesn't respond.

### Pitfall 4: LookAndFeel Not Applied to Child Components
**What goes wrong:** Pads use default JUCE styling instead of the dark ChordPumper theme.
**Why it happens:** `setLookAndFeel()` on the Editor doesn't automatically propagate to dynamically created child components if they're added after the LookAndFeel is set.
**How to avoid:** Set LookAndFeel on the Editor BEFORE adding child components. Or use `findColour()` in PadComponent's paint (which traverses the component hierarchy to find the active LookAndFeel). Or explicitly set colours on pad components.
**Warning signs:** Pads have white/grey default styling while the Editor background is dark.

### Pitfall 5: Grid Layout Not Updated on Resize
**What goes wrong:** Pads don't reposition when the plugin window is resized.
**Why it happens:** `Grid::performLayout()` only runs once in the constructor instead of in `resized()`.
**How to avoid:** Always perform grid layout in `resized()`, which JUCE calls whenever the component bounds change.
**Warning signs:** Pads overlap or clip when DAW resizes the plugin window.

### Pitfall 6: Audio Thread Allocation via processBlock MIDI Generation
**What goes wrong:** Audio dropouts during MIDI generation.
**Why it happens:** Creating `std::vector<int>` from `Chord::midiNotes()` in processBlock allocates heap memory.
**How to avoid:** Never call `Chord::midiNotes()` in processBlock. The GUI thread calls `MidiKeyboardState::noteOn()` for each note — the audio thread never needs to know about Chord objects or allocate vectors. The separation is: GUI thread resolves chords to individual MIDI notes; audio thread just injects pre-queued MIDI events.
**Warning signs:** Occasional audio glitches when clicking pads rapidly.

### Pitfall 7: Octave Choice Puts Notes Outside Audible/Useful Range
**What goes wrong:** Chords sound too high, too low, or span an awkward range.
**Why it happens:** Using octave 3 for root puts some seventh chords below MIDI note 48 (C3) which can sound muddy, or octave 5 puts augmented chords above MIDI note 80.
**How to avoid:** Default octave 4 (root at MIDI 60 = middle C). This places all v1 chord types (up to +11 semitones for major 7th) within the comfortable range of MIDI 60-71. Verify with `Chord::midiNotes(4)` — the highest note in any chord is 72 (C5 for B major 7th = 71 + 11 = 82... actually B4=71, 71+11=82). Actually: MIDI note for B4 = 11 + (4+1)*12 = 71. B maj7 = {71, 75, 78, 82}. Top note 82 = Bb5 — comfortable range.
**Warning signs:** User complaints about chords sounding too low or too high.

## Code Examples

### processBlock with MidiKeyboardState Integration

```cpp
// Source: JUCE MidiKeyboardState docs (official) + JUCE synth tutorial pattern
void ChordPumperProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);
}
```

### PadComponent Paint Method

```cpp
void PadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    float cornerSize = 6.0f;

    // Background based on state
    if (isPressed)
        g.setColour(findColour(juce::TextButton::buttonOnColourId));
    else if (isHovered)
        g.setColour(findColour(juce::ResizableWindow::backgroundColourId).brighter(0.15f));
    else
        g.setColour(findColour(juce::ResizableWindow::backgroundColourId).brighter(0.05f));

    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(findColour(juce::GroupComponent::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Chord name label
    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText(juce::String(chord.name()), getLocalBounds(), juce::Justification::centred);
}
```

### PadComponent Mouse Handling

```cpp
void PadComponent::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    repaint();
    if (onClick)
        onClick(chord);
}

void PadComponent::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    repaint();
}

void PadComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void PadComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    isPressed = false;
    repaint();
}
```

### GridPanel with Timer Note-Off

```cpp
class GridPanel : public juce::Component, private juce::Timer
{
public:
    GridPanel(juce::MidiKeyboardState& state)
        : keyboardState(state)
    {
        for (int i = 0; i < 32; ++i)
        {
            auto* pad = pads.add(new PadComponent());
            pad->setClickCallback([this](const Chord& chord) { padClicked(chord); });
            addAndMakeVisible(pad);
        }
        populatePalette(chromaticPalette());
    }

    ~GridPanel() override
    {
        releaseCurrentChord();
    }

    void resized() override
    {
        juce::Grid grid;
        grid.setGap(juce::Grid::Px(4));

        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;

        for (int i = 0; i < 8; ++i)
            grid.templateColumns.add(Track(Fr(1)));
        for (int i = 0; i < 4; ++i)
            grid.templateRows.add(Track(Fr(1)));

        for (auto* pad : pads)
            grid.items.add(juce::GridItem(*pad));

        grid.performLayout(getLocalBounds());
    }

private:
    void padClicked(const Chord& chord)
    {
        releaseCurrentChord();
        auto notes = chord.midiNotes(defaultOctave);
        for (int note : notes)
            keyboardState.noteOn(midiChannel, note, velocity);
        activeNotes.assign(notes.begin(), notes.end());
        startTimer(noteDurationMs);
    }

    void timerCallback() override
    {
        releaseCurrentChord();
        stopTimer();
    }

    void releaseCurrentChord()
    {
        for (int note : activeNotes)
            keyboardState.noteOff(midiChannel, note, 0.0f);
        activeNotes.clear();
    }

    void populatePalette(const std::array<Chord, 32>& palette)
    {
        for (int i = 0; i < 32; ++i)
            pads[i]->setChord(palette[static_cast<size_t>(i)]);
    }

    juce::MidiKeyboardState& keyboardState;
    juce::OwnedArray<PadComponent> pads;
    std::vector<int> activeNotes;

    static constexpr int midiChannel = 1;
    static constexpr int defaultOctave = 4;
    static constexpr int noteDurationMs = 300;
    float velocity = 0.8f;
};
```

### Complete MidiMessage Factory Usage

```cpp
// Source: JUCE MidiMessage class reference (official)

// Note-on with float velocity (0.0–1.0)
auto noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);   // channel 1, C4, velocity 0.8

// Note-on with uint8 velocity (0–127)
auto noteOn2 = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);

// Note-off with float velocity
auto noteOff = juce::MidiMessage::noteOff(1, 60, 0.0f);

// Note-off (no velocity — sends velocity 0)
auto noteOff2 = juce::MidiMessage::noteOff(1, 60);

// Adding to MidiBuffer at sample position 0
midiBuffer.addEvent(noteOn, 0);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual Rectangle subdivision for grid layouts | `juce::Grid` (CSS Grid layout) | JUCE 5.3 (2018) | Declarative, gap-aware, fractional sizing. Use Grid for uniform grids. |
| Custom lock-free FIFO for GUI→audio MIDI | `MidiKeyboardState` as standard bridge | Always available in JUCE | MidiKeyboardState has always been the intended pattern. Custom FIFOs only needed for non-MIDI data. |
| `juce::Font(float)` constructor | `juce::Font(juce::FontOptions(float))` | JUCE 8 (2024) | Float constructor deprecated. Use FontOptions. Already adopted in Phase 1 editor. |
| Raw MidiMessage byte construction | Static factory methods (noteOn, noteOff, etc.) | Always preferred | Type-safe, validates parameters, readable. |
| Blocking `sleep()`/`usleep()` for note-off timing | `juce::Timer` on message thread | Always preferred in JUCE | Non-blocking, runs on message thread, ~10-20ms precision is acceptable for chord preview. |

**Deprecated/outdated:**
- **`juce::Font(float)` constructor:** Deprecated in JUCE 8. Use `juce::Font(juce::FontOptions(float))`.
- **`MidiBuffer::Iterator`:** The old iterator class was removed. Use range-for loops: `for (const auto metadata : midiBuffer)`.
- **`juce::Atomic<T>`:** Wrapper around `std::atomic`. Use `std::atomic` directly — JUCE's wrapper adds no value over the standard.

## Open Questions

1. **Note-off timing: Timer vs. mouseUp**
   - What we know: Timer gives fixed-duration chords (good for consistent preview). mouseUp gives hold-to-play (good for performance feel).
   - What's unclear: Which feels better for chord exploration. Users may expect different behaviors.
   - Recommendation: Start with Timer (300ms fixed duration). It's simpler, avoids stuck-note edge cases with mouse-leave, and gives consistent behavior. Can add mouseUp mode later if requested.

2. **Chromatic palette fill chord selection**
   - What we know: 12 major + 12 minor = 24. Need 8 more chords to fill 32 pads. Options: diminished triads, augmented triads, sus2/sus4 (sus not in current ChordType enum), or 7th chords.
   - What's unclear: Whether the user prefers diminished/augmented fills or would rather have 7th chords.
   - Recommendation: Use 4 diminished + 4 augmented triads as fills. They're already in the ChordType enum and represent the remaining triad types. The 4 unique diminished groups and 4 unique augmented groups cover all 12 pitch classes. This gives a complete "all triads" palette. Seventh chords will appear when the morph engine suggests them in Phase 4.

3. **Default velocity value**
   - What we know: MIDI velocity 0.0 = noteOn treated as noteOff by many synths. 1.0 = maximum. 
   - What's unclear: Optimal default for exploration (too loud is fatiguing, too soft is hard to hear).
   - Recommendation: Default 0.8f (≈102/127). Standard "mf" dynamic. Configurable via member variable for future Phase 5+ velocity UI.

4. **MIDI output in Bitwig with IS_SYNTH plugin**
   - What we know: Phase 1 verified the plugin loads. The IS_SYNTH + MIDI output routing in Bitwig should work (instrument MIDI output feeds downstream devices in the same chain).
   - What's unclear: Whether Bitwig actually routes MIDI output from an IS_SYNTH plugin downstream. Most IS_SYNTH plugins produce audio, not MIDI. This needs testing.
   - Recommendation: The first plan (03-01 or 03-03) should include a verification step: load ChordPumper → add a synth downstream in Bitwig → verify MIDI flows. If it doesn't work, fallback is to place ChordPumper as a Note FX device before an instrument.

## Sources

### Primary (HIGH confidence)
- JUCE Grid class reference: https://docs.juce.com/master/classGrid.html — Grid, GridItem, Fr, Px, TrackInfo, performLayout API
- JUCE MidiKeyboardState class reference: https://docs.juce.com/master/classMidiKeyboardState.html — noteOn, noteOff, processNextMidiBuffer, injectIndirectEvents behavior
- JUCE MidiBuffer class reference: https://docs.juce.com/master/classMidiBuffer.html — addEvent, clear, iterator API
- JUCE MidiMessage class reference: https://docs.juce.com/master/classMidiMessage.html — static noteOn/noteOff factories, channel/velocity parameters
- JUCE Timer class reference: https://docs.juce.com/master/classjuce_1_1Timer.html — startTimer, stopTimer, timerCallback on message thread
- JUCE Plugin Examples tutorial: https://docs.juce.com/master/tutorial_plugin_examples.html — Arpeggiator processBlock pattern with MidiBuffer::addEvent
- JUCE FlexBox and Grid tutorial: https://docs.juce.com/master/tutorial_flex_box_grid.html — Grid layout usage patterns
- Existing codebase: PluginProcessor.h/cpp, PluginEditor.h/cpp, Chord.h/cpp, PitchClass.h, ChordType.h — verified Phase 1+2 artifacts

### Secondary (MEDIUM confidence)
- JUCE AbstractFifo reference: https://docs.juce.com/master/classAbstractFifo.html — lock-free FIFO for custom queues (not needed but researched as alternative)
- JUCE Synth tutorial: https://docs.juce.com/master/tutorial_synth_using_midi_input.html — MidiKeyboardState + Synthesiser integration pattern
- bruce.audio memory ordering article: https://www.bruce.audio/post/2025/02/24/memory_ordering/ — lock-free audio synchronization context
- Bitwig routing documentation: https://www.bitwig.com/userguide/latest/routing — device chain MIDI routing behavior

### Tertiary (LOW confidence)
- Bitwig IS_SYNTH MIDI output routing: Community understanding that Bitwig routes instrument MIDI output downstream, but not explicitly documented for the IS_SYNTH + MIDI-only case. Needs Phase 3 verification.

## Metadata

**Confidence breakdown:**
- Grid UI (GRID-01): HIGH — juce::Grid is well-documented, CSS Grid semantics are standard, PadComponent is straightforward JUCE Component pattern
- Chromatic palette (GRID-02): HIGH — Uses existing engine types. Palette is a static array of Chord values. Only open question is fill chord selection (minor design decision, not technical risk).
- MIDI output (MIDI-01): HIGH — MidiKeyboardState is the standard JUCE pattern, processNextMidiBuffer is the official API, arpeggiator tutorial confirms the MidiBuffer::addEvent pattern
- MIDI note-on/off (MIDI-02): HIGH — MidiMessage::noteOn/noteOff factories are well-documented, Timer-based note-off is standard pattern
- Lock-free communication: HIGH — MidiKeyboardState handles this internally, no custom lock-free code needed
- Bitwig MIDI routing: MEDIUM — IS_SYNTH plugins typically produce audio, not MIDI output. MIDI downstream routing needs verification in Phase 3.

**Research date:** 2026-02-19
**Valid until:** ~2026-06-19 (JUCE 8.0.12 is stable; MidiKeyboardState, Grid, Timer APIs are mature and unlikely to change)
