# Pitfalls Research

**Domain:** JUCE C++ Audio Plugin (MIDI, Linux)
**Researched:** 2026-02-19
**Confidence:** HIGH (core pitfalls), MEDIUM (CLAP/DnD specifics)

---

## Critical Pitfalls

Mistakes that cause rewrites, crashes, or shipped-product failures.

### Pitfall 1: Memory Allocation in the Audio Thread

**What goes wrong:** Calling `new`, `delete`, `malloc`, `std::vector::push_back`, `juce::String` construction, `DBG()`, or any allocating operation inside `processBlock()`. The OS memory allocator uses locks internally — any allocation on the audio thread can block for milliseconds, causing audible clicks, pops, and dropouts.

**Why it happens:** C++ makes allocation invisible. `std::string` concatenation, lambda captures, `juce::Array` resizing, and even `DBG()` (which builds a `juce::String`) all allocate. Developers coming from non-real-time backgrounds don't realize these are forbidden.

**Consequences:** Intermittent audio glitches that are hard to reproduce in testing but audible in production. DAWs may flag the plugin as unstable. pluginval's real-time safety checker will fail the plugin.

**Warning signs:**
- `DBG()` calls anywhere in `processBlock()` or functions it calls
- `std::vector` or `juce::Array` used without pre-allocation in audio path
- `juce::String` construction/formatting in audio path
- Any `new`/`make_unique`/`make_shared` in audio path

**Prevention:**
- Pre-allocate all buffers in `prepareToPlay()` — `MidiBuffer`, working arrays, etc.
- Use `juce::AbstractFifo` or lock-free queues for audio-to-UI communication
- Use `std::atomic<>` for simple parameter reads (load per-block, not per-sample in tight loops)
- Run pluginval at strictness level 10 in CI — it intercepts allocations on the audio thread
- Use Tracktion's allocator-interceptor or Address Sanitizer to detect allocations

**Phase to address:** Phase 1 (Core Architecture) — establish real-time-safe patterns from day one. Retrofitting is extremely painful.

**Confidence:** HIGH — JUCE forum #1 most common mistake, melatonin.dev, pluginval documentation all confirm.

---

### Pitfall 2: Locks/Mutexes in the Audio Thread

**What goes wrong:** Using `std::mutex`, `juce::CriticalSection`, `juce::SpinLock`, or any blocking synchronization primitive in `processBlock()`. The audio thread has a hard deadline (buffer_size / sample_rate seconds). A lock that blocks for even 1ms at 128 samples / 48kHz (2.67ms budget) causes a dropout.

**Why it happens:** Developers need to share state between the audio thread and the UI/message thread. The instinct is to use a mutex. This is correct in general programming but catastrophic in real-time audio.

**Consequences:** Priority inversion — the audio thread blocks waiting for a lower-priority UI thread holding the lock. Random, unreproducible audio dropouts. Worse under CPU load.

**Warning signs:**
- Any `lock()`, `lock_guard`, `scoped_lock`, `ScopedLock` in audio code path
- `SpinLock` in audio path (still not real-time safe — can spin indefinitely)
- Shared data structures accessed from both `processBlock` and UI code without lock-free patterns

**Prevention:**
- Use `std::atomic<>` for single values (parameters, flags)
- Use `juce::AbstractFifo` for streaming data between threads
- Use lock-free single-producer/single-consumer queues for complex messages
- For state that must be atomically swapped (e.g., chord voicing tables), use an atomic pointer swap pattern or `std::atomic<std::shared_ptr<>>` (C++20)
- `parameterChanged` / `parameterValueChanged` callbacks can fire on ANY thread including audio — treat them as audio-thread-restricted

**Phase to address:** Phase 1 (Core Architecture).

**Confidence:** HIGH — universally documented across JUCE forum, ADC talks (Dave Rowland's lock-free queue presentations), melatonin.dev.

---

### Pitfall 3: External Drag-and-Drop Broken/Unreliable on Linux

**What goes wrong:** `DragAndDropContainer::performExternalDragDropOfFiles()` does not work reliably on Linux. JUCE forum reports confirm it fails on many Linux desktop environments, with only XFCE reported as working. The implementation depends on X11 drag-and-drop protocol support that varies across DEs and window managers.

**Why it happens:** JUCE's Linux drag-and-drop implementation uses X11 DnD protocol (XDND). Plugin windows are embedded in the host's window hierarchy via X11 embedding (XEmbed), and XDND from an embedded window is fundamentally problematic. The plugin doesn't own the top-level window, so initiating an external drag may not work as the host intercepts or ignores the drag events.

**Consequences:** The MIDI-to-DAW drag-and-drop feature — a core differentiator — may not work on Linux at all using JUCE's built-in mechanism. This could require a completely different approach.

**Warning signs:**
- Testing only in standalone mode (where DnD may work) but not in plugin context
- Testing only in one DAW/DE combination
- No Wayland testing (XDND doesn't exist on Wayland)

**Prevention:**
- Research alternative approaches early: CLAP's native drag-and-drop extensions, host-specific APIs, or clipboard-based workflows as fallback
- Test in actual DAW plugin context (Bitwig, REAPER, Ardour) from the earliest prototype
- Consider a "copy MIDI to clipboard" fallback that always works
- Consider writing a temp `.mid` file and letting the user drag from a file manager as an alternative workflow
- Investigate whether Bitwig's CLAP implementation supports CLAP drag-and-drop extensions natively

**Phase to address:** Phase 2 or 3 (MIDI Features) — but research feasibility in Phase 1 prototype. If it doesn't work, the feature design must change.

**Confidence:** MEDIUM — Forum reports confirm Linux DnD issues exist. Exact workarounds for plugin-embedded contexts need validation.

---

### Pitfall 4: parameterChanged Fires on the Audio Thread

**What goes wrong:** `AudioProcessorParameter::Listener::parameterValueChanged()` and `AudioProcessorValueTreeState::Listener::parameterChanged()` can be called from ANY thread, including the audio thread. Developers treat these callbacks as UI-safe, calling `repaint()`, allocating memory, or doing heavy computation inside them.

**Why it happens:** JUCE's documentation doesn't prominently warn about this. The callback name suggests it's a notification, and developers assume notifications are on the message thread. But DAWs can (and do) call `setValueNotifyingHost()` from the audio thread during automation playback.

**Consequences:** Memory allocation on audio thread (from `repaint()` or string operations in the callback). UI jank. Potential deadlock if callback tries to acquire a lock also held by the audio thread.

**Warning signs:**
- `repaint()` called directly inside `parameterChanged`
- String formatting or logging inside parameter callbacks
- Complex state recalculation inside parameter callbacks

**Prevention:**
- In parameter callbacks, only set an `std::atomic<bool>` flag
- Use a `juce::Timer` (or `juce::VBlankAttachment` on JUCE 7.0.6+) polling at 30-60Hz to check the flag and trigger UI updates
- Never call `repaint()` from a parameter callback
- Pattern (from melatonin.dev):
  ```cpp
  void parameterValueChanged(int, float) override {
      recalculate.store(true);
  }
  void timerCallback() override {
      if (recalculate.exchange(false))
          repaint();
  }
  ```

**Phase to address:** Phase 1 (Core Architecture) — parameter handling pattern must be established before any UI work.

**Confidence:** HIGH — melatonin.dev "big list of tips", JUCE forum threads, multiple experienced developers confirm.

---

### Pitfall 5: Plugin State Versioning Not Planned From Day One

**What goes wrong:** `getStateInformation()` / `setStateInformation()` serialize the current parameter set without any version marker. When parameters are added, removed, or renamed in later versions, old DAW sessions crash or load with wrong values. Users lose their work.

**Why it happens:** V1 developers don't think about V2. The default `AudioProcessorValueTreeState::state.copyXmlToBinary()` pattern works perfectly for the current version but has no migration path.

**Consequences:** Users who saved sessions with V1 open them with V2 and get wrong parameter values, crashes, or silent failures. This is the #1 cause of user complaints for plugin updates.

**Warning signs:**
- No version number in serialized state
- Parameters identified by index rather than string ID
- No unit tests for loading old state formats
- `setStateInformation` doesn't validate input data

**Prevention:**
- Include a version integer in serialized state from V1
- Use stable string parameter IDs (not indices) — `AudioProcessorValueTreeState` does this by default
- Write a `migrateState(int fromVersion)` function even if it's empty in V1
- Use XML serialization (human-readable, debuggable) rather than raw binary for state — or use binary with a version header
- Write tests that load hardcoded V1 state blobs and verify correct parameter restoration
- Never remove a parameter ID — deprecate and hide instead

**Phase to address:** Phase 1 (Core Architecture) — state format decisions are permanent once users save sessions.

**Confidence:** HIGH — JUCE forum threads on state issues, melatonin.dev tips, plugin developer experience reports.

---

### Pitfall 6: Static/Singleton State Shared Between Plugin Instances

**What goes wrong:** Using `static` variables or singletons to store plugin state. When a user loads two instances of the plugin in the same DAW, they share the static state, causing one instance's actions to corrupt the other's.

**Why it happens:** C++ developers use singletons as a convenience pattern. In standalone apps it's fine. In plugins, multiple instances coexist in the same process (or sometimes different processes — DAW-dependent).

**Consequences:** Two plugin instances interfere with each other. Bugs that only appear when multiple instances are loaded. Extremely hard to debug.

**Warning signs:**
- `static` member variables in processor or editor classes
- Singleton patterns (`getInstance()`) anywhere in plugin code
- `thread_local` used for data that should be per-instance (audio effects on same channel may share a thread)

**Prevention:**
- All state lives in the `AudioProcessor` instance. Pass references down to components.
- No singletons. No `static` mutable state. No `thread_local` for per-instance data.
- Test with two instances of the plugin in the same DAW session.

**Phase to address:** Phase 1 (Core Architecture).

**Confidence:** HIGH — melatonin.dev "big list of tips" explicitly covers this, confirmed by Dave Rowland (Tracktion).

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Skip pluginval in CI | Faster builds | Ship plugins that crash in DAWs | Never for release builds |
| `DBG()` in processBlock | Quick debugging | Audio glitches in Debug builds; false sense of working code | Only during initial development, remove immediately |
| Raw pointers for component ownership | Less typing | Memory leaks flagged by JUCE leak detector, crash on plugin re-open | Never — use `std::unique_ptr` or JUCE `OwnedArray` |
| Skip state versioning | Ship faster V1 | V2 breaks all saved sessions | Never — add version header from day one |
| Monolithic `processBlock` | All logic in one place | Untestable, unmaintainable | Only for proof-of-concept |
| Hardcoded UI sizes | Quick layout | Breaks on HiDPI, different host window sizes | Only for early prototyping |
| `std::mutex` for thread communication | Obvious correctness | Audio dropouts under load | Never in audio path — acceptable for non-real-time background tasks |
| Copy-paste chord voicing tables | Fast to prototype | Hard to maintain, music theory errors | Only for initial spike, refactor before Phase 2 |

---

## Integration Gotchas

### CLAP via clap-juce-extensions

| Issue | Detail | Mitigation |
|-------|--------|------------|
| **AAX validator breakage** | Inheriting from `clap_juce_extensions::clap_properties` has been reported to break the AAX validator (GitHub issue #166). | Not relevant for Linux-only/no-AAX, but be aware if expanding format support later. |
| **OpenGL rendering shows red** | CLAP plugins using OpenGL render as red rectangles until the window is resized (GitHub issue #159). | Avoid OpenGL for UI rendering, or implement a forced resize on plugin open. |
| **Resize/aspect ratio issues** | `setFixedAspectRatio()` doesn't work correctly on some platforms (issue #153). Resize API interacts improperly with aspect ratio (issue #56). | Test resize behavior in each target DAW. Don't rely on fixed aspect ratio working. |
| **Bypass state ignored** | CLAP bypass and deactivated states are not properly forwarded through clap-juce-extensions (issue #15, open since 2022). | Implement bypass handling in `processBlock` rather than relying on host bypass. |
| **Linux host behavior differences** | Issue #63 documents Linux-specific host behavior changes in CLAP. | Test in Bitwig (CLAP co-creator, best support), REAPER, and Ardour. |
| **CMake policy warning CMP0177** | Recent CMake versions emit warnings about policy CMP0177 (issue #162). | Set `cmake_policy(SET CMP0177 NEW)` or pin CMake version. |
| **JUCE 9 will have native CLAP** | Official CLAP support announced for JUCE 9. clap-juce-extensions is a bridge. | Design abstractions so migration to native CLAP is straightforward. |

### Bitwig on Linux

| Issue | Detail | Mitigation |
|-------|--------|------------|
| **Bitwig co-created CLAP** | Bitwig has the best CLAP support of any DAW. Prefer CLAP format for Bitwig. | Ship CLAP as primary format, VST3 as fallback. |
| **Vulkan graphics backend** | Bitwig 5.3.12 fixed Linux Vulkan graphics crashes. Older Bitwig versions may have issues. | Test on latest Bitwig. Don't do anything exotic with window rendering. |
| **Plugin window embedding** | VST3 plugin windows in DAW hosts use X11 embedding (XEmbed). Bitwig's CLAP implementation may handle window embedding differently. | Test plugin window lifecycle (open, close, re-open, resize) in Bitwig specifically. |
| **MIDI drag-and-drop to Bitwig** | Bitwig accepts MIDI file drops to the arrangement, but the drag must originate from a valid X11 DnD source. Plugin-embedded windows may not qualify. | Test early. Have a fallback (clipboard, file export). |

### Linux Audio Stack

| Issue | Detail | Mitigation |
|-------|--------|------------|
| **GLIBC version linking** | Building on a newer distro and distributing to an older one fails if the binary links against a newer GLIBC. | Build on the oldest supported distro (or use a Docker container with older GLIBC). Alternatively, target a specific GLIBC version. |
| **Wayland vs X11** | JUCE's Linux GUI is X11-only. Wayland users run DAWs under XWayland. JUCE has known Wayland issues (CalloutBox text not editable, focus problems). | Target X11. Document that XWayland is required. Test under both X11-native and XWayland sessions. |
| **X11 `isShowing()` bug (JUCE 8.0.2)** | A regression in `juce_Windowing_linux.cpp` inverts `isShowing()` logic — returns true when hidden, false when showing. Causes crashes and focus issues. | Check JUCE version. If using 8.0.2, verify this is fixed or apply patch. Monitor JUCE GitHub issues. |
| **XEMBED_MAPPED flag** | A stricter check for this flag broke compatibility with some hosts. Plugin windows may appear blank. | Test in all target hosts after each JUCE update. |
| **PipeWire / JACK / ALSA** | Linux audio stacks vary. PipeWire (with JACK compatibility) is now standard on most distros. Plugins don't interact with audio servers directly (the DAW does), but standalone builds need to select the right backend. | For plugin builds, this is the DAW's problem. For standalone testing, support JACK. |

---

## Performance Traps

### Audio Thread Performance

| Trap | Description | Detection | Fix |
|------|-------------|-----------|-----|
| **Atomic loads in tight inner loops** | Calling `param->load()` per-sample in a tight loop forces memory barriers on every iteration. With 20+ parameters at 512 samples, this adds up. | Profiling shows unexpectedly high CPU in processBlock. | Load atomic parameters once per block into local variables, then use locals in the loop. |
| **Unnecessary MidiBuffer iteration** | Iterating the entire MidiBuffer multiple times (once for note-on, once for note-off, etc.) wastes cycles. | processBlock takes longer than expected with MIDI input. | Single-pass iteration with event dispatch. |
| **Voice leading recalculation per sample** | Recalculating chord voicings for every MIDI event rather than caching. | CPU spikes on chord changes. | Cache voicing results. Only recalculate when input chord or parameters actually change. Use `std::atomic<bool>` dirty flag. |
| **Unbounded MIDI output** | Generating more MIDI events than the output buffer can hold, or generating events with invalid timestamps. | DAW crashes, MIDI data corruption. | Validate output buffer capacity. Ensure all event timestamps are within `0..numSamples-1`. |

### UI Thread Performance

| Trap | Description | Detection | Fix |
|------|-------------|-----------|-----|
| **Allocating in `paint()`** | Creating `juce::Path`, `juce::Image`, `juce::ColourGradient` objects inside `paint()` calls. | Laggy UI, dropped frames on complex UIs. | Make paths/images member variables. Allocate once, reuse in paint. |
| **Cascading repaints from transparency** | Transparent child components trigger parent repaints, which trigger sibling repaints. Grid of 48+ chord buttons can cascade. | `JUCE_ENABLE_REPAINT_DEBUGGING=1` shows excessive repaint regions. | Use `setOpaque(true)` where possible. Minimize transparent overlays. Repaint only changed cells, not the whole grid. |
| **Timer-driven repaints too frequent** | `startTimerHz(60)` calling `repaint()` on the entire grid every frame even when nothing changed. | Constant CPU usage even when plugin is idle. | Only repaint if state has actually changed (check dirty flags). Use `VBlankAttachment` instead of Timer for frame-accurate updates. |
| **LookAndFeel destruction order** | LookAndFeel destroyed before components using it. Members are destroyed in reverse declaration order. | Crash on plugin editor close, JUCE assertion. | Declare LookAndFeel members BEFORE components that use them in the class definition. |

---

## UX Pitfalls

### Plugin UI

| Pitfall | Description | Prevention |
|---------|-------------|------------|
| **No build version indicator** | Can't tell if DAW loaded the latest build or a cached old one. | Add `__DATE__ " " __TIME__` label in Debug builds. |
| **HiDPI not handled** | UI looks tiny on HiDPI Linux displays or blurry when scaled. | Use JUCE's logical pixel system. Test on 1x and 2x displays. Use `Desktop::getInstance().getDisplays()` to verify. |
| **Editor re-open crashes** | Closing and re-opening the plugin editor triggers assertion failures because listeners weren't removed in destructors. | Every `addListener()` must have a matching `removeListener()` in the destructor. pluginval tests this. |
| **Resizing breaks layout** | Host resizes plugin window and components overlap or disappear. | Use `juce::Component::resized()` with relative positioning. Test at multiple window sizes. |

### DAW Integration

| Pitfall | Description | Prevention |
|---------|-------------|------------|
| **Plugin not found after build** | DAW doesn't scan the build output directory. Developer thinks plugin is broken. | Set `COPY_PLUGIN_AFTER_BUILD TRUE` in CMake. Install to `~/.vst3/` and `~/.clap/`. |
| **State not restored on session load** | `setStateInformation` not called, or called with unexpected data. Behavior varies by DAW and plugin format. | Test state save/restore in every target DAW. Don't assume consistent behavior across hosts. |
| **MIDI channel handling** | Plugin processes all MIDI channels or ignores channel info. DAW sends on specific channels. | Respect MIDI channel filtering. Allow user to configure input channel. |
| **Sample rate changes not handled** | DAW changes sample rate mid-session. `prepareToPlay` called again but plugin doesn't reset internal state. | Implement full re-initialization in `prepareToPlay()`. Don't cache sample rate in constructors. |

---

## "Looks Done But Isn't" Checklist

- [ ] **pluginval strictness 10 passes** — Tests allocation on audio thread, state save/restore, editor open/close cycles, random parameters
- [ ] **Two instances simultaneously** — Load two instances in same DAW session, verify no shared state corruption
- [ ] **State round-trip** — Save session, close DAW, reopen, verify all parameters and UI state restored exactly
- [ ] **Editor re-open cycle** — Open editor, close editor, reopen 10x. No leaks, no assertions, no crashes
- [ ] **Buffer size changes** — Change buffer size in DAW preferences mid-session. Plugin continues working
- [ ] **Sample rate changes** — Switch between 44.1/48/96kHz. No crashes, no wrong timing
- [ ] **Automation playback** — Automate parameters in DAW. Verify smooth transitions, no clicks
- [ ] **MIDI flood test** — Send rapid MIDI input (all notes at once, rapid arpeggios). No crashes, no stuck notes
- [ ] **CPU idle test** — Plugin loaded but not processing. CPU usage should be near-zero (no busy-wait timers)
- [ ] **Linux DnD in plugin context** — Test drag-and-drop FROM the plugin window TO the DAW when the plugin is hosted (not standalone)
- [ ] **CLAP and VST3 both tested** — Don't assume one format works because the other does
- [ ] **Tested in Bitwig, REAPER, and Ardour** — Each host has different embedding and lifecycle behavior

---

## Recovery Strategies

### "Audio thread is allocating/locking"
1. Run pluginval with `--strictness-level 10` to identify the exact call site
2. Add Address Sanitizer (`-fsanitize=address`) or Tracktion's real-time thread checker
3. Move the allocation to `prepareToPlay()` or use pre-allocated containers
4. Replace locks with `std::atomic` or `juce::AbstractFifo`

### "Plugin crashes on editor re-open"
1. Check JUCE leak detector output — press Continue in debugger to see ALL leaked types
2. Search for every `addListener` / `addChangeListener` — ensure matching `removeListener` in destructor
3. Check member declaration order — LookAndFeels before components, processor references before components that use them
4. Use `juce::Component::SafePointer` if components reference each other

### "State doesn't restore correctly"
1. Add logging to `getStateInformation` and `setStateInformation` to dump the serialized data
2. Verify parameter IDs match between save and load
3. Check for XML type coercion — `ValueTree::toXml()` converts all var types to strings
4. Test with binary serialization (`copyXmlToBinary` / `getXmlFromBinary`) — verify the round-trip

### "Drag-and-drop doesn't work in plugin"
1. Confirm it works in standalone (isolates JUCE DnD from host embedding issues)
2. Test with `performExternalDragDropOfFiles()` writing a `.mid` temp file
3. If X11 DnD fails from embedded window, implement clipboard-based fallback
4. Research CLAP drag-and-drop extensions as alternative path (Bitwig-specific)

### "CLAP plugin not recognized"
1. Verify `.clap` bundle is in `~/.clap/`
2. Run `clap-info` tool against the binary to check metadata
3. Run `clap-validator` against the binary
4. Check clap-juce-extensions version compatibility with your JUCE version

### "Linux build works on dev machine but not target"
1. Check GLIBC version: `ldd --version` on target vs build machine
2. Check linked libraries: `ldd your_plugin.so` — any "not found"?
3. Install missing dependencies (see JUCE Linux Dependencies docs)
4. Consider building in a Docker container with older base image for wider compatibility

---

## Pitfall-to-Phase Mapping

| Phase | Pitfall | Priority | Notes |
|-------|---------|----------|-------|
| **Phase 1: Core Architecture** | Audio thread real-time safety | CRITICAL | Establish lock-free patterns from day one |
| **Phase 1: Core Architecture** | Parameter callback threading | CRITICAL | Set up Timer/VBlank polling pattern immediately |
| **Phase 1: Core Architecture** | State versioning | CRITICAL | Add version header to state format before any saves |
| **Phase 1: Core Architecture** | No singletons/statics | CRITICAL | Architectural decision, hard to fix later |
| **Phase 1: Core Architecture** | CMake + CLAP setup | HIGH | Get clap-juce-extensions building correctly from start |
| **Phase 2: MIDI Processing** | MidiBuffer timing | HIGH | Sample-accurate event handling, note-on/off pairing |
| **Phase 2: MIDI Processing** | Voice leading performance | MEDIUM | Cache results, only recalculate on change |
| **Phase 2: MIDI Processing** | MIDI output validation | HIGH | Bounds-check timestamps, don't overflow buffer |
| **Phase 3: UI/Grid** | Paint allocation avoidance | HIGH | Member-variable paths/images, not per-paint |
| **Phase 3: UI/Grid** | Cascading repaints | MEDIUM | setOpaque, targeted repaints, dirty flags |
| **Phase 3: UI/Grid** | Editor lifecycle (open/close) | HIGH | Listener cleanup in destructors |
| **Phase 3: UI/Grid** | LookAndFeel destruction order | MEDIUM | Declare before components |
| **Phase 4: DnD/Export** | Linux DnD from plugin context | CRITICAL | May not work — research alternatives in Phase 1 |
| **Phase 4: DnD/Export** | Bitwig-specific MIDI drop | HIGH | Test early, have fallback ready |
| **Integration/CI** | pluginval in CI | HIGH | Set up after Phase 1, run continuously |
| **Integration/CI** | GLIBC compatibility | MEDIUM | Build environment decision early |
| **Integration/CI** | Multi-DAW testing | HIGH | Bitwig + REAPER + Ardour from Phase 2 onward |

---

## Sources

### HIGH Confidence (Official docs, authoritative)
- JUCE official docs: AudioProcessorValueTreeState tutorial — https://docs.juce.com/master/tutorial_audio_processor_value_tree_state.html
- JUCE CMake API docs — https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE Linux Dependencies — https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md
- JUCE BREAKING_CHANGES.md — https://github.com/juce-framework/JUCE/blob/master/BREAKING_CHANGES.md
- JUCE DragAndDropContainer docs — https://docs.juce.com/master/classDragAndDropContainer.html
- JUCE AbstractFifo docs — https://docs.juce.com/master/juce__AbstractFifo_8h.html
- VST3 Plugin Locations (Steinberg) — https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html
- clap-juce-extensions GitHub issues — https://github.com/free-audio/clap-juce-extensions/issues
- Bitwig CLAP announcement — https://www.bitwig.com/stories/201/

### MEDIUM Confidence (Verified community sources)
- melatonin.dev "Big List of JUCE Tips and Tricks" — https://melatonin.dev/blog/big-list-of-juce-tips-and-tricks
- melatonin.dev "pluginval is a plugin dev's best friend" — https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend
- melatonin.dev "Dealing with UI jank in JUCE" — https://melatonin.dev/blog/dealing-with-jank-in-juce
- JUCE Forum: #1 most common programming mistake — https://forum.juce.com/t/1-most-common-programming-mistake-that-we-see-on-the-forum/26013
- JUCE Forum: External DnD on Linux — https://forum.juce.com/t/external-drag-drop-does-not-work-on-linux/25819
- JUCE Forum: Wayland support — https://forum.juce.com/t/wayland-support/12591
- JUCE Forum: AudioProcessorValueTreeState thread safety — https://forum.juce.com/t/audioprocessorvaluetreestate-thread-safety/21811
- JUCE GitHub issue #1481: Wayland CalloutBox — https://github.com/juce-framework/JUCE/issues/1481
- JUCE GitHub issue #1430: isShowing() inversion — https://github.com/juce-framework/JUCE/issues/1430
- Pamplejuce CMake template — https://github.com/sudara/pamplejuce
- Dave Rowland (ADC 2025): Lock-free queues presentations — https://drowaudio.github.io/presentations/

### LOW Confidence (Needs validation)
- CLAP drag-and-drop extensions for Bitwig — needs verification of current support status
- XWayland plugin window embedding behavior — limited testing reports available
- JUCE 9 native CLAP timeline — announced but no release date confirmed
