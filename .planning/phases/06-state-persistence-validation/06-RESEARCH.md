# Phase 6: State Persistence & Validation - Research

**Researched:** 2026-02-20
**Domain:** JUCE plugin state serialization, pluginval validation, real-time safety
**Confidence:** HIGH

## Summary

Phase 6 covers two distinct requirements: (1) reliable state persistence so the plugin survives DAW session reload (PLAT-06), and (2) passing pluginval validation at strictness level 5+ (PLAT-07). These are deeply interrelated — pluginval's state restoration tests at levels 2, 6, and 8 specifically verify that `getStateInformation`/`setStateInformation` round-trip correctly.

The current codebase has **empty stubs** for both `getStateInformation` and `setStateInformation` in `PluginProcessor.cpp`. The plugin has no `AudioProcessorValueTreeState` (APVTS) and no registered parameters. The state that needs serializing is entirely non-parameter data: the 32 pad chords (post-morph), the last-played chord (which determines the morph context), the progression strip contents, and the morph engine weights. This is a simpler scenario than parameter-heavy plugins — we use a raw `ValueTree` for serialization with `copyXmlToBinary`/`getXmlFromBinary` for the binary wrapper DAWs expect.

Pluginval v1.0.4 (latest, December 2024) tests 15+ scenarios across strictness levels 1-10. At level 5 (the recommended minimum for host compatibility), the plugin must pass: plugin info logging, program switching, editor creation (cold/warm), audio processing at multiple sample rates and block sizes, automation with sub-block parameter changes, editor automation with rapid parameter fuzzing, bus layout tests, state save/restore, VST3 validator (for VST3 format), and editor-while-processing concurrency. Since ChordPumper has zero automatable parameters currently, most parameter-related tests are trivially satisfied. The critical tests are **state save/restore** (level 2+), **state restoration with value verification** (level 6+), **audio processing** (level 3+), and **editor creation** (level 2+).

**Primary recommendation:** Implement ValueTree-based state serialization with version tagging first, then run pluginval at level 5 to identify issues, then audit processBlock for real-time safety violations.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PLAT-06 | Plugin state saves and restores correctly when DAW session is reloaded | ValueTree serialization pattern with `copyXmlToBinary`/`getXmlFromBinary`; version tagging for forward compatibility; encode grid state, morph context, progression strip, weights |
| PLAT-07 | Plugin passes pluginval validation at reasonable strictness level | Pluginval v1.0.4 test catalog documented below; level 5 target; state restoration, audio processing, editor creation, bus layout tests identified |
</phase_requirements>

## Standard Stack

### Core

| Library/Class | Version | Purpose | Why Standard |
|---------------|---------|---------|--------------|
| `juce::ValueTree` | JUCE 8.0.12 | Hierarchical state storage with XML/binary serialization | Built into JUCE, thread-safe copy semantics, supports typed properties and child nodes |
| `juce::AudioProcessor::copyXmlToBinary` | JUCE 8.0.12 | Serialize XML to binary blob for DAW state storage | Standard JUCE helper — handles the XML→MemoryBlock conversion that DAWs expect |
| `juce::AudioProcessor::getXmlFromBinary` | JUCE 8.0.12 | Deserialize binary blob back to XML for state restore | Companion to `copyXmlToBinary`, with null-safety on corrupt data |
| pluginval | v1.0.4 | Cross-platform plugin validation and testing | Industry standard — "Verified by pluginval" at level 5 is the de facto compatibility bar |

### Supporting

| Library/Class | Version | Purpose | When to Use |
|---------------|---------|---------|-------------|
| `juce::XmlElement` | JUCE 8.0.12 | Intermediate representation between ValueTree and binary | Created by `ValueTree::createXml()`, consumed by `copyXmlToBinary` |
| `juce::MemoryBlock` | JUCE 8.0.12 | Binary blob container for state data | Passed to/from `getStateInformation`/`setStateInformation` |
| Catch2 | v3.13.0 | Unit testing framework (already in project) | State serialization round-trip unit tests |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Raw ValueTree (no APVTS) | `AudioProcessorValueTreeState` (APVTS) | APVTS is designed for host-automatable parameters with UI bindings; ChordPumper's state is non-parameter data (chord grids, morph context). Raw ValueTree is simpler and more appropriate. If automatable parameters are added in v2 (e.g., GRID-07 adjustable weights), APVTS can be layered on top. |
| XML-based binary format (`copyXmlToBinary`) | `ValueTree::writeToStream` (raw binary) | Binary is faster/smaller but not human-debuggable. XML-based binary provides debuggability via `getXmlFromBinary` while still using a compact binary wrapper. Performance difference is negligible for our small state size. |
| pluginval pre-built binary | pluginval as CMake subdirectory | CMake subdirectory gives better stack traces when debugging failures, but adds build complexity. Start with pre-built binary, switch to CMake integration only if needed for debugging. |

## Architecture Patterns

### Pattern 1: ValueTree State Schema

**What:** Define a structured ValueTree hierarchy that captures all plugin state.
**When to use:** In `getStateInformation` and `setStateInformation`.

The state tree for ChordPumper:

```
ChordPumperState (version="1")
├── Grid
│   ├── Pad (index="0", root="0", accidental="0", type="0", roman="")
│   ├── Pad (index="1", root="0", accidental="0", type="1", roman="iv")
│   └── ... (32 pads total)
├── MorphContext
│   ├── lastPlayedRoot="0"
│   ├── lastPlayedAccidental="0"
│   ├── lastPlayedType="0"
│   └── lastVoicing="60,64,67" (comma-separated MIDI notes)
├── Progression
│   ├── Chord (root="0", accidental="0", type="0")
│   └── ... (up to 8)
└── Weights
    ├── diatonic="0.40"
    ├── commonTones="0.25"
    └── voiceLeading="0.25"
```

**Example:**

```cpp
juce::ValueTree serializeChord(const Chord& chord, const juce::Identifier& type)
{
    juce::ValueTree node(type);
    node.setProperty("root", static_cast<int>(chord.root.letter), nullptr);
    node.setProperty("accidental", static_cast<int>(chord.root.accidental), nullptr);
    node.setProperty("type", static_cast<int>(chord.type), nullptr);
    return node;
}

Chord deserializeChord(const juce::ValueTree& node)
{
    Chord chord;
    chord.root.letter = static_cast<NoteLetter>(static_cast<int>(node["root"]));
    chord.root.accidental = static_cast<int8_t>(static_cast<int>(node["accidental"]));
    chord.type = static_cast<ChordType>(static_cast<int>(node["type"]));
    return chord;
}
```

### Pattern 2: Version-Tagged State with Forward Compatibility

**What:** Include a version number in the serialized state to enable future schema migration.
**When to use:** Always — even for v1. Prevents locked-in schema from day one.

```cpp
static constexpr int kCurrentStateVersion = 1;

void ChordPumperProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("ChordPumperState");
    state.setProperty("version", kCurrentStateVersion, nullptr);

    // ... serialize grid, morph context, progression, weights ...

    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void ChordPumperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr) return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid() || !state.hasType("ChordPumperState")) return;

    int version = state.getProperty("version", 0);
    if (version < 1) return;

    // ... deserialize with version-aware logic ...
    // Future: if (version >= 2) { read new fields }
}
```

### Pattern 3: State Ownership in Processor (Not Editor)

**What:** All serializable state lives in or is accessible from the Processor. The Editor reads it on creation and writes back on interaction.
**When to use:** Always — the Processor outlives the Editor (editors are created/destroyed as plugin windows open/close).

Currently, grid state (pad chords, morph context) lives in `GridPanel` and progression state lives in `ProgressionStrip` — both are Editor-owned components. For state persistence, the canonical state must move to (or be mirrored in) the Processor:

```cpp
class ChordPumperProcessor : public juce::AudioProcessor
{
public:
    // State that persists across editor lifecycles
    struct PersistentState {
        std::array<Chord, 32> gridChords;
        std::array<std::string, 32> romanNumerals;
        Chord lastPlayedChord;
        std::vector<int> lastVoicing;
        std::vector<Chord> progression;
        MorphWeights weights;
        bool hasMorphed = false; // distinguishes initial palette from morphed grid
    };

    PersistentState& getState() { return persistentState; }
    const PersistentState& getState() const { return persistentState; }

private:
    PersistentState persistentState;
};
```

The Editor reads `getState()` on construction and updates it when the user interacts. `getStateInformation` serializes `persistentState`; `setStateInformation` deserializes it. When the Editor is recreated (e.g., plugin window reopened), it initializes from `getState()`.

### Anti-Patterns to Avoid

- **Storing state only in the Editor:** The Editor is transient — it's destroyed when the plugin window closes. State must live in or be mirrored to the Processor.
- **Using `AudioProcessorValueTreeState` for non-parameter data only:** APVTS is designed for DAW-automatable parameters. Using it solely for internal state adds unnecessary complexity without the benefits of parameter automation.
- **Allocating memory in `getStateInformation`:** While `getStateInformation` is typically called on the message thread, some hosts call it from other threads. The ValueTree approach is safe because `createXml()` and `copyXmlToBinary` do allocate, but they're called outside the audio thread. Never call `getStateInformation` patterns from `processBlock`.
- **Ignoring version field:** Without a version tag, any schema change in a future release will break state loading from older sessions. Always include it.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Binary serialization format | Custom byte packing/unpacking | `ValueTree::createXml()` + `copyXmlToBinary()` | Handles encoding, null safety, cross-platform byte order |
| State version migration | Ad-hoc version checks scattered through code | Single version integer in root ValueTree + centralized migration in `setStateInformation` | Clean migration path, single point of maintenance |
| Plugin validation | Manual test scripts for each DAW | pluginval headless mode | Standard tool, 15+ test categories, CI-compatible |
| Parameter thread safety | `std::mutex` in processBlock | `std::atomic<float>` for parameter values, ValueTree for state | Lock-free on audio thread |

**Key insight:** ChordPumper's state is relatively small (32 chords + a few scalars) — ValueTree XML serialization is more than fast enough, and the debugging benefits of human-readable state outweigh the marginal performance gain of custom binary formats.

## Common Pitfalls

### Pitfall 1: State Only Lives in Editor Components

**What goes wrong:** Grid state and progression state are lost when the plugin window closes (Editor destroyed), even though the DAW session is still open. Reopening the plugin window shows the initial chromatic palette instead of the morphed grid.
**Why it happens:** Currently, `GridPanel` owns the pad chords and `ProgressionStrip` owns the chord progression. These are Editor children. The Processor has no access to this data.
**How to avoid:** Move canonical state to `PersistentState` struct in the Processor. Editor reads from it on creation, writes to it on user interaction.
**Warning signs:** State lost when closing/reopening plugin window (without DAW reload).

### Pitfall 2: Empty getStateInformation Passes Pluginval Level 2 But Fails Level 6+

**What goes wrong:** At level 2, `PluginStateTest` only checks that `getStateInformation`/`setStateInformation` don't crash. At level 6, `PluginStateTestRestoration` verifies that parameter values are actually restored within tolerance (0.1). At level 8, it checks binary state equality.
**Why it happens:** The level 2 test is a crash test. Level 6 actually validates restoration correctness. Since ChordPumper currently has no automatable parameters, the parameter restoration tests pass vacuously — but the binary state equality test at level 8 requires `getStateInformation` to return consistent data.
**How to avoid:** Ensure `getStateInformation` returns deterministic output. Test that `get → set → get` produces identical binary data.
**Warning signs:** Pass at level 5, fail at level 8 with "Returned state differs from that set by host."

### Pitfall 3: processBlock Allocations Under Pluginval Level 9

**What goes wrong:** Pluginval's `AllocationsInRealTimeThreadTest` (level 9) intercepts `malloc` during `processBlock` and fails if any allocation occurs.
**Why it happens:** Common hidden allocations in JUCE audio code:
- `std::vector` resize/push_back in audio callback
- `juce::String` construction (allocates heap memory)
- `juce::MidiBuffer::addEvent` if buffer needs to grow
- `juce::Array` resizing
**How to avoid:** Audit `processBlock` and all functions it calls. Pre-allocate all buffers in `prepareToPlay`. Use `MidiBuffer::ensureSize()` in `prepareToPlay`. Current `processBlock` is simple (`buffer.clear()`, `midiMessages.clear()`, `keyboardState.processNextMidiBuffer`) — verify `processNextMidiBuffer` doesn't allocate.
**Warning signs:** "Allocations occurred in audio thread" at pluginval level 9.

### Pitfall 4: Thread Safety of State Access

**What goes wrong:** DAW calls `setStateInformation` from a background thread while the Editor is reading/writing state on the message thread, causing data races.
**Why it happens:** Per JUCE docs and forum discussions, `getStateInformation`/`setStateInformation` can be called from any thread depending on the host. Pluginval's `BackgroundThreadStateTest` (level 7) specifically tests this: it opens the editor on the message thread, then calls state save/restore from a background thread.
**How to avoid:** Use a `juce::CriticalSection` (or `SpinLock`) to protect `PersistentState` access in `getStateInformation`/`setStateInformation`, and use message-thread dispatching for Editor updates. Alternatively, use copy-on-read/copy-on-write: `getStateInformation` takes a snapshot under a brief lock, serializes the copy outside the lock.
**Warning signs:** Crash or data corruption when running pluginval level 7 `BackgroundThreadStateTest`.

### Pitfall 5: Forgetting to Restore Grid Visual State After setStateInformation

**What goes wrong:** State is restored in the Processor, but the Editor (if currently open) doesn't update to reflect the restored state. The grid continues showing the old chords.
**Why it happens:** `setStateInformation` updates `PersistentState` in the Processor, but nobody tells the open Editor to refresh.
**How to avoid:** Use a listener/callback pattern: Processor notifies Editor that state has changed. The Editor re-reads `PersistentState` and updates all components. `juce::AsyncUpdater` is a good fit — trigger from `setStateInformation`, handle on message thread.
**Warning signs:** After DAW undo or preset change, the grid doesn't update visually even though MIDI output reflects the restored state.

### Pitfall 6: CLAP State via clap-juce-extensions

**What goes wrong:** CLAP format doesn't save/restore state correctly even though VST3 works fine.
**Why it happens:** `clap-juce-extensions` maps CLAP's `state.save()`/`state.load()` to JUCE's `getStateInformation`/`setStateInformation`, so the same code path should work. However, CLAP state is streamed (not block-based), so serialization must be complete and self-contained.
**How to avoid:** The `copyXmlToBinary`/`getXmlFromBinary` pattern produces a self-contained `MemoryBlock` that works with both VST3 and CLAP state mechanisms. Test state save/restore in all three formats.
**Warning signs:** State works in VST3 but not CLAP, or vice versa.

## Code Examples

Verified patterns from JUCE official documentation and codebase:

### Complete getStateInformation / setStateInformation Pattern

```cpp
// Source: JUCE tutorial_audio_processor_value_tree_state.html
void ChordPumperProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("ChordPumperState");
    state.setProperty("version", kCurrentStateVersion, nullptr);

    // Serialize grid
    juce::ValueTree gridNode("Grid");
    for (int i = 0; i < 32; ++i)
    {
        auto padNode = serializeChord(persistentState.gridChords[i], "Pad");
        padNode.setProperty("index", i, nullptr);
        padNode.setProperty("roman", juce::String(persistentState.romanNumerals[i]), nullptr);
        gridNode.addChild(padNode, -1, nullptr);
    }
    state.addChild(gridNode, -1, nullptr);

    // Serialize morph context
    if (persistentState.hasMorphed)
    {
        auto morphNode = serializeChord(persistentState.lastPlayedChord, "MorphContext");
        juce::String voicingStr;
        for (size_t i = 0; i < persistentState.lastVoicing.size(); ++i)
        {
            if (i > 0) voicingStr += ",";
            voicingStr += juce::String(persistentState.lastVoicing[i]);
        }
        morphNode.setProperty("voicing", voicingStr, nullptr);
        state.addChild(morphNode, -1, nullptr);
    }

    // Serialize progression
    juce::ValueTree progNode("Progression");
    for (const auto& chord : persistentState.progression)
        progNode.addChild(serializeChord(chord, "Chord"), -1, nullptr);
    state.addChild(progNode, -1, nullptr);

    // Serialize weights
    juce::ValueTree weightsNode("Weights");
    weightsNode.setProperty("diatonic", persistentState.weights.diatonic, nullptr);
    weightsNode.setProperty("commonTones", persistentState.weights.commonTones, nullptr);
    weightsNode.setProperty("voiceLeading", persistentState.weights.voiceLeading, nullptr);
    state.addChild(weightsNode, -1, nullptr);

    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void ChordPumperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr) return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid() || !state.hasType("ChordPumperState")) return;

    int version = state.getProperty("version", 0);
    if (version < 1) return;

    // Deserialize grid
    auto gridNode = state.getChildWithName("Grid");
    if (gridNode.isValid())
    {
        for (int i = 0; i < gridNode.getNumChildren() && i < 32; ++i)
        {
            auto padNode = gridNode.getChild(i);
            int index = padNode.getProperty("index", i);
            if (index >= 0 && index < 32)
            {
                persistentState.gridChords[index] = deserializeChord(padNode);
                persistentState.romanNumerals[index] =
                    padNode.getProperty("roman", "").toString().toStdString();
            }
        }
    }

    // Deserialize morph context
    auto morphNode = state.getChildWithName("MorphContext");
    if (morphNode.isValid())
    {
        persistentState.lastPlayedChord = deserializeChord(morphNode);
        persistentState.hasMorphed = true;
        juce::String voicingStr = morphNode.getProperty("voicing", "");
        persistentState.lastVoicing.clear();
        for (auto& token : juce::StringArray::fromTokens(voicingStr, ",", ""))
            persistentState.lastVoicing.push_back(token.getIntValue());
    }

    // Deserialize progression
    auto progNode = state.getChildWithName("Progression");
    if (progNode.isValid())
    {
        persistentState.progression.clear();
        for (int i = 0; i < progNode.getNumChildren(); ++i)
            persistentState.progression.push_back(deserializeChord(progNode.getChild(i)));
    }

    // Deserialize weights
    auto weightsNode = state.getChildWithName("Weights");
    if (weightsNode.isValid())
    {
        persistentState.weights.diatonic = weightsNode.getProperty("diatonic", 0.40f);
        persistentState.weights.commonTones = weightsNode.getProperty("commonTones", 0.25f);
        persistentState.weights.voiceLeading = weightsNode.getProperty("voiceLeading", 0.25f);
    }

    // Notify any open editor
    sendChangeMessage();  // or use AsyncUpdater
}
```

### Pluginval Headless Invocation

```bash
# Download pluginval v1.0.4 for Linux
wget https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip
unzip pluginval_Linux.zip

# Run at strictness level 5 (recommended minimum)
./pluginval --strictness-level 5 --validate /path/to/ChordPumper.vst3

# Run at level 5 for CLAP
./pluginval --strictness-level 5 --validate ~/.clap/ChordPumper.clap

# Verbose output for debugging
./pluginval --strictness-level 5 --verbose --validate /path/to/ChordPumper.vst3
```

### State Round-Trip Unit Test

```cpp
TEST_CASE("State serialization round-trips correctly", "[state]")
{
    ChordPumperProcessor processor;

    // Set up known state
    auto& state = processor.getState();
    state.gridChords[0] = {pitches::C, ChordType::Major};
    state.gridChords[1] = {pitches::D, ChordType::Minor};
    state.hasMorphed = true;
    state.lastPlayedChord = {pitches::C, ChordType::Major};
    state.lastVoicing = {60, 64, 67};
    state.progression = {{pitches::C, ChordType::Major}, {pitches::F, ChordType::Major}};

    // Serialize
    juce::MemoryBlock block;
    processor.getStateInformation(block);

    // Create fresh processor, restore
    ChordPumperProcessor processor2;
    processor2.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    // Verify
    auto& restored = processor2.getState();
    REQUIRE(restored.gridChords[0].root == pitches::C);
    REQUIRE(restored.gridChords[0].type == ChordType::Major);
    REQUIRE(restored.gridChords[1].type == ChordType::Minor);
    REQUIRE(restored.lastPlayedChord.root == pitches::C);
    REQUIRE(restored.lastVoicing == std::vector<int>{60, 64, 67});
    REQUIRE(restored.progression.size() == 2);

    // Binary equality (pluginval level 8 test)
    juce::MemoryBlock block2;
    processor2.getStateInformation(block2);
    REQUIRE(block == block2);
}
```

## Pluginval Test Catalog

Complete catalog of pluginval tests by strictness level (from source code analysis):

| Level | Test Name | Category | What It Tests |
|-------|-----------|----------|---------------|
| 1 | Plugin info | Basic | Logs name, latency, tail length — crash test |
| 2 | Plugin programs | Basic | Switches programs randomly — crash test |
| 2 | Editor | Basic | Creates editor twice (cold/warm) — expects non-null |
| 2 | Automatable Parameters | Basic | Logs parameter info for all automatable params |
| 2 | Plugin state | State | `getStateInformation` → randomize params → `setStateInformation` — crash test |
| 3 | Audio processing | Audio | 10 blocks at each sample rate/block size combo; NaN/Inf/subnormal checks |
| 3 | Automation | Audio | Sub-block processing with random parameter changes between blocks |
| 4 | Open editor whilst processing | Concurrency | processBlock on async thread while creating editor on message thread |
| 4 | Basic bus | Bus | Lists supported layouts, enables/disables buses, restores default layout |
| 5 | Editor Automation | GUI | With editor open, rapid random parameter value changes (100 iterations) |
| 5 | auval / VST3 validator | Format | Runs Steinberg's vstvalidator (VST3) or Apple's auval (AU) |
| 6 | Plugin state restoration | State | Per-parameter: save → randomize → restore → verify value within 0.1 tolerance. At level 8+: binary state equality check |
| 6 | Editor stress | GUI | Opens editor with released processing; opens with 0 sample rate / block size |
| 6 | Fuzz parameters | Params | Sets 5 random values per parameter, calls text/value conversion functions |
| 6 | Non-releasing audio processing | Audio | Calls `prepareToPlay` at new sample rate WITHOUT calling `releaseResources` first |
| 7 | Background thread state | Concurrency | Opens editor on message thread, saves/restores state from background thread |
| 7 | Parameter thread safety | Concurrency | `setValueNotifyingHost` on message thread while `setValue` + `processBlock` on background thread, 500 iterations |
| 7 | Parameters | Params | Same as automatable parameters test but at higher strictness |
| 8 | Larger than prepared block size | Audio | processBlock with 2x the prepared block size (CLAP/Standalone only, skipped for VST3) |
| 9 | Allocations during process | Realtime | Intercepts `malloc` during `processBlock`, fails on any allocation |

### ChordPumper-Specific Risk Assessment

| Test | Risk Level | Notes |
|------|-----------|-------|
| Plugin state (L2) | **LOW** — once implemented | Just needs to not crash |
| Plugin state restoration (L6) | **LOW** | ChordPumper has 0 automatable parameters — vacuously satisfied |
| Audio processing (L3) | **LOW** | Current processBlock is trivial: `clear + processNextMidiBuffer` |
| Editor (L2) | **LOW** | Editor already works |
| Editor whilst processing (L4) | **MEDIUM** | Need to verify no races between processBlock and editor creation |
| Automation (L3) | **LOW** | No automatable parameters to randomize |
| Background thread state (L7) | **MEDIUM** | State access must be thread-safe |
| Allocations during process (L9) | **LOW-MEDIUM** | Need to verify `processNextMidiBuffer` doesn't allocate |
| VST3 validator (L5) | **MEDIUM** | Linux VST3 validator has known issues |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Custom binary serialization | ValueTree + `copyXmlToBinary` | JUCE 4+ | Standardized, debuggable state format |
| Manual XML string building | `ValueTree::createXml()` | JUCE 4+ | Type-safe, no manual string escaping |
| No validation tool | pluginval at strictness 5+ | 2018+ (pluginval 0.x) | Industry standard pre-release quality gate |
| Malloc interception via custom allocator | pluginval `ScopedAllocationDisabler` + `AllocatorInterceptor` | pluginval 1.0+ | Automated real-time safety verification |
| APVTS required for all state | Raw ValueTree for non-parameter state, APVTS for parameters | JUCE 5.4+ | Cleaner separation of concerns |

**Deprecated/outdated:**
- `AudioProcessorValueTreeState::createAndAddParameter()` — deprecated since JUCE 5.4. Use constructor with `ParameterLayout` instead.
- `juce::Font(float)` constructor — deprecated in JUCE 8. Use `juce::Font(juce::FontOptions(float))` (already using correct pattern).

## Open Questions

1. **Should we add automatable parameters for morph weights?**
   - What we know: `MorphWeights` has three float values (diatonic, commonTones, voiceLeading) that could be DAW-automatable. v2 requirement GRID-07 envisions user-adjustable weights.
   - What's unclear: Whether to add APVTS now for weights or keep them as internal state for v1.
   - Recommendation: Keep as non-parameter internal state for v1. The weights are not user-adjustable yet (GRID-07 is v2). Adding APVTS adds complexity and parameter reporting to DAWs that users can't yet meaningfully control. Add APVTS when GRID-07 is implemented.

2. **What strictness level should be the target?**
   - What we know: Level 5 is the "Verified by pluginval" minimum. Level 9 adds allocation checking. Level 10 adds RTC (real-time context) checking.
   - What's unclear: Whether level 5 is sufficient or if higher is practical.
   - Recommendation: Target level 5 as the required pass. Run level 7 to catch thread safety issues. Treat levels 8-10 as stretch goals — they catch real issues but some may require significant refactoring.

3. **Thread safety strategy for PersistentState**
   - What we know: `getStateInformation`/`setStateInformation` can be called from any thread. Editor runs on message thread. processBlock runs on audio thread.
   - What's unclear: Whether a lock-based or lock-free approach is better for the state struct.
   - Recommendation: Use `juce::CriticalSection` with brief lock durations for `getStateInformation`/`setStateInformation`. These methods are NOT called from the audio thread (they're called from message thread or host background threads), so a lock is safe here. The audio thread (`processBlock`) doesn't access `PersistentState` directly — it only uses `keyboardState`.

4. **pluginval and CLAP format support**
   - What we know: pluginval v1.0.4 added CLAP support. The CLAP state extension maps to JUCE's `getStateInformation`/`setStateInformation` via clap-juce-extensions.
   - What's unclear: Whether pluginval reliably loads CLAP plugins on Linux. Some users report issues.
   - Recommendation: Test VST3 with pluginval first (most mature), then CLAP. If CLAP loading fails in pluginval, test CLAP state manually in Bitwig.

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 source code (local) — `juce_AudioProcessor.h`: `getStateInformation`, `setStateInformation`, `copyXmlToBinary`, `getXmlFromBinary`
- JUCE 8.0.12 source code (local) — `juce_ValueTree.h`: `createXml()`, `fromXml()`, `setProperty()`, `getProperty()`
- JUCE official tutorial: "Saving and loading your plug-in state" — https://docs.juce.com/master/tutorial_audio_processor_value_tree_state.html — APVTS pattern, `copyXmlToBinary`/`getXmlFromBinary` usage
- pluginval v1.0.4 source code — `Source/tests/BasicTests.cpp`, `EditorTests.cpp`, `BusTests.cpp`, `ExtremeTests.cpp`, `ParameterFuzzTests.cpp` — complete test catalog with strictness levels
- pluginval README — https://github.com/Tracktion/pluginval — headless invocation, strictness levels 1-10 explained

### Secondary (MEDIUM confidence)
- CLAP state extension spec — https://github.com/free-audio/clap/blob/main/include/clap/ext/state.h — save/load stream API, mark_dirty callback
- JUCE forum: "Are getStateInformation() and setStateInformation() always called on the audio thread?" — https://forum.juce.com/t/53467 — confirms threading ambiguity
- timur.audio: "Using locks in real-time audio processing, safely" — https://timur.audio/using-locks-in-real-time-audio-processing-safely — forbidden operations on audio thread
- pluginval v1.0.4 release notes — https://github.com/tracktion/pluginval/releases — CLAP support, Linux minimum Ubuntu 22.04

### Tertiary (LOW confidence)
- Community reports of Linux VST3 validator issues in pluginval — needs validation during implementation

## Metadata

**Confidence breakdown:**
- State serialization pattern: **HIGH** — ValueTree + `copyXmlToBinary` pattern directly from JUCE official tutorial and source code
- Pluginval test catalog: **HIGH** — extracted from actual pluginval source code (BasicTests.cpp, etc.)
- Real-time safety: **HIGH** — well-documented forbidden operations; current processBlock is trivial
- Thread safety of state access: **MEDIUM** — threading model confirmed by JUCE forum but specific host behavior varies
- CLAP state compatibility: **MEDIUM** — clap-juce-extensions maps to same JUCE APIs, but Linux-specific CLAP testing in pluginval may have issues

**Research date:** 2026-02-20
**Valid until:** 2026-03-20 (30 days — JUCE 8 and pluginval are stable)
