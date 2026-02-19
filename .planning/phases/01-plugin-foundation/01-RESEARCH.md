# Phase 1: Plugin Foundation - Research

**Researched:** 2026-02-19
**Domain:** JUCE 8 CMake plugin scaffold + clap-juce-extensions (Linux x86_64)
**Confidence:** HIGH

## Summary

Phase 1 establishes the build infrastructure for ChordPumper: a JUCE 8 CMake project producing VST3, CLAP, and Standalone binaries on Linux. The core challenge is wiring together JUCE's `juce_add_plugin` CMake API with clap-juce-extensions for CLAP format support, and ensuring correct plugin categorization as an Instrument in both VST3 and CLAP formats.

JUCE 8.0.12's CMake API is well-documented and stable. The `juce_add_plugin()` function handles format-specific build targets. clap-juce-extensions (latest commit `02f91b7`, Jan 2026) provides CLAP support via a simple `clap_juce_extensions_plugin()` CMake call after the main plugin target. Both are mature — Surge XT, ChowDSP, and Dexed all use this exact stack.

The one area requiring careful attention is the **Instrument categorization**. The user decided "Instrument (not MIDI Effect)" but ChordPumper outputs MIDI rather than audio. In VST3, `IS_SYNTH TRUE` categorizes as "Instrument" and implies audio output. The plugin must declare stereo audio buses (outputting silence for now) to satisfy host expectations while routing MIDI downstream. For CLAP, the "instrument" feature tag technically means "processes notes and produces audio" — but this matches Bitwig's browser categorization, so it's the right choice for user-facing plugin placement.

**Primary recommendation:** Use `IS_SYNTH TRUE` with `NEEDS_MIDI_OUTPUT TRUE` and stereo audio output buses (silent). This places ChordPumper in the Instrument category in Bitwig for both VST3 and CLAP.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Plugin Identity
- Plugin name: **ChordPumper**
- Manufacturer: User's personal name/alias (use placeholder if not provided, easy to change later)
- DAW category: **Instrument** (not MIDI Effect)
- Unique plugin ID: Generate a standard JUCE plugin code

#### JUCE Licensing
- Use **AGPLv3** license (free tier) for development
- JUCE acquired as **git submodule** pinned to release tag 8.0.12
- Open source / proprietary decision deferred — set up build to work with either

#### Source Organization
- **Modular layout**: `src/engine/`, `src/ui/`, `src/midi/` subdirectories
- C++ namespace: `chordpumper`
- clap-juce-extensions as git submodule alongside JUCE

#### Initial Plugin Window
- Window size: **1000x600** (large — room for 8x4 grid + controls in future phases)
- Visual direction: **Dark theme** (standard for DAW plugins)
- Phase 1 shell content: **Plugin name + version** text centered in window

### Claude's Discretion
- Test directory structure (separate `tests/` or alongside source)
- Header style (.h/.cpp pairs vs header-only for pure logic)
- Specific dark theme colors
- CMake structure details (single vs multi-file)
- .gitignore contents

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PLAT-01 | Plugin builds and runs on Linux x86_64 (CachyOS) | Linux dependency list verified (pacman packages). GCC 15 C++20 confirmed working. CMake 4.2.3 + Ninja 1.13.2 available. |
| PLAT-02 | Plugin available as VST3 format, loadable in Bitwig Studio 5+ | `juce_add_plugin(FORMATS VST3 Standalone)` + `IS_SYNTH TRUE`. Bitwig scans `~/.vst3/`. COPY_PLUGIN_AFTER_BUILD copies there automatically. |
| PLAT-03 | Plugin available as CLAP format via clap-juce-extensions, loadable in Bitwig Studio 5+ | `clap_juce_extensions_plugin()` CMake function. Pin to commit `02f91b7`. Bitwig scans `~/.clap/`. Must manually copy CLAP binary. |
| PLAT-04 | Plugin available as standalone application | `FORMATS VST3 Standalone` in juce_add_plugin. Standalone launches natively on Linux with ALSA/JACK audio. |
| PLAT-05 | Build system uses CMake with juce_add_plugin | Full CMake API documented. Single CMakeLists.txt with CMakePresets.json for Debug/Release. Ninja generator. |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 | Plugin framework, GUI, audio/MIDI | Industry standard. `juce_add_plugin()` CMake API handles VST3/Standalone targets. Git submodule pinned to tag. |
| clap-juce-extensions | commit `02f91b7` | CLAP format support | Only way to build CLAP with JUCE until JUCE 9 (no ETA). Used by Surge XT, ChowDSP, Dexed. MIT licensed. |
| CMake | ≥3.22 (system: 4.2.3) | Build system | JUCE's native build system since v6. CachyOS ships 4.2.3. |
| Ninja | 1.13.2 | Build backend | 3-5x faster incremental builds than Make. CMake generates Ninja files with `-G Ninja`. |
| GCC | 15.2 | C++20 compiler | Ships with CachyOS. Full C++20 support. |

### Supporting (Phase 1 only)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| ccache | system | Compilation caching | Always — dramatically speeds up rebuilds. `CMAKE_CXX_COMPILER_LAUNCHER=ccache` |
| pluginval | 1.0.4 | VST3 validation | After build — validates plugin loads correctly. VST3 only (no CLAP support). |
| clap-validator | 0.3.2+ | CLAP validation | After build — validates CLAP plugin correctness. Install via AUR or cargo. |

## Architecture Patterns

### Recommended Project Structure

```
ChordPumper/
├── CMakeLists.txt              # Single root CMake file
├── CMakePresets.json            # Debug/Release build presets
├── .gitignore
├── .gitmodules
├── .clang-format
├── VERSION                     # Single source of truth for version
├── libs/
│   ├── JUCE/                   # git submodule @ tag 8.0.12
│   └── clap-juce-extensions/   # git submodule @ commit 02f91b7
├── src/
│   ├── PluginProcessor.h       # AudioProcessor subclass
│   ├── PluginProcessor.cpp
│   ├── ui/
│   │   ├── PluginEditor.h      # AudioProcessorEditor subclass
│   │   ├── PluginEditor.cpp
│   │   └── ChordPumperLookAndFeel.h  # Custom dark theme
│   ├── engine/                 # Empty in Phase 1 — chord/theory engine later
│   └── midi/                   # Empty in Phase 1 — MIDI output logic later
├── tests/
│   └── placeholder_test.cpp    # Catch2 smoke test (optional Phase 1)
└── resources/                  # Icons, assets (empty Phase 1)
```

**Rationale for structure:**
- `PluginProcessor.h/cpp` at `src/` root — it's the plugin's entry point, referenced by every subdirectory
- `src/ui/` — all visual components. PluginEditor lives here since it's UI-specific.
- `src/engine/` and `src/midi/` — created empty in Phase 1 to establish the layout. Populated in Phases 2-3.
- `tests/` separate from `src/` — keeps test dependencies (Catch2) out of plugin build. Standard CMake convention.
- `.h/.cpp` pairs for all classes — consistent pattern, good for compilation units and IDE navigation

### Pattern 1: juce_add_plugin Configuration for Instrument Category

**What:** Correct CMake flags to categorize ChordPumper as an Instrument with MIDI output.

**When to use:** The `juce_add_plugin()` call in CMakeLists.txt.

**Critical flag interactions:**

| Flag | Value | Effect |
|------|-------|--------|
| `IS_SYNTH` | `TRUE` | VST3 categories default to "Instrument Synth". Plugin appears in Instrument browser. |
| `IS_MIDI_EFFECT` | `FALSE` | Not a MIDI-only effect. Plugin has audio buses (even if outputting silence). |
| `NEEDS_MIDI_INPUT` | `TRUE` | Enables MIDI input bus. Needed for future MIDI keyboard triggering (Phase 3+). |
| `NEEDS_MIDI_OUTPUT` | `TRUE` | Enables MIDI output bus. Core requirement — chord MIDI goes downstream. |

**Example:**
```cmake
# Source: JUCE CMake API docs (official)
juce_add_plugin(ChordPumper
    COMPANY_NAME           "ChordPumperDev"
    PLUGIN_MANUFACTURER_CODE Cpdv
    PLUGIN_CODE            Cpmp
    IS_SYNTH               TRUE
    NEEDS_MIDI_INPUT       TRUE
    NEEDS_MIDI_OUTPUT      TRUE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS                VST3 Standalone
    PRODUCT_NAME           "ChordPumper"
    PLUGIN_NAME            "ChordPumper"
    DESCRIPTION            "Chord progression discovery instrument"
)
```

**Why `IS_SYNTH TRUE` and not `IS_MIDI_EFFECT TRUE`:**
- `IS_SYNTH TRUE` → VST3_CATEGORIES defaults to "Instrument Synth" → Bitwig shows it as an Instrument
- `IS_MIDI_EFFECT TRUE` → Plugin is treated as MIDI-only processor. Some hosts put MIDI effects in a different slot (before instruments). The user explicitly decided against this.
- Trade-off: With IS_SYNTH TRUE, the plugin must declare audio output buses. In Phase 1, `processBlock` outputs silence. This is normal — many MIDI-outputting instruments produce silence when auditioning is disabled.

### Pattern 2: clap-juce-extensions Integration

**What:** Adding CLAP format support via clap-juce-extensions.

**When to use:** After `juce_add_plugin()` in CMakeLists.txt.

**Example:**
```cmake
# Source: clap-juce-extensions README (official)
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

clap_juce_extensions_plugin(
    TARGET ChordPumper
    CLAP_ID "org.chordpumper.chordpumper"
    CLAP_FEATURES instrument utility
)
```

**CLAP feature tags explained:**
- `instrument` — "Plugin can process note events and then produce audio." Places plugin in Bitwig's Instrument browser. User's explicit choice.
- `utility` — Subcategory hint. ChordPumper is a utility/tool for chord exploration.

**CLAP_ID format:** Reverse-domain notation. Use a placeholder domain for now — easy to change before release. Must be globally unique.

**Note:** clap-juce-extensions does NOT use `FORMATS` — it creates its own `ChordPumper_CLAP` target automatically. The CLAP binary appears alongside VST3/Standalone in the build artifacts directory.

### Pattern 3: Plugin Codes and Manufacturer IDs

**What:** Four-character codes required by JUCE for plugin identification.

**PLUGIN_MANUFACTURER_CODE:**
- Exactly 4 characters
- At least one uppercase letter (AU compatibility)
- GarageBand requires: first letter uppercase, rest lowercase
- Must be unique to your company/person
- Example: `Cpdv` (ChordPumper Dev)

**PLUGIN_CODE:**
- Exactly 4 characters
- Exactly one uppercase letter (AU compatibility)
- Must be unique per plugin
- GarageBand requires: first letter uppercase, rest lowercase
- Example: `Cpmp` (ChordPumPer)
- JUCE default: random code regenerated each configure — always set explicitly

**Note:** These codes primarily matter for AU format (macOS). For VST3 on Linux, the codes still get embedded but Bitwig uses the VST3 UID for identification. For CLAP, the `CLAP_ID` string is used instead. Still, set stable codes from the start to avoid state-loading issues if formats expand later.

### Pattern 4: Dark Theme LookAndFeel

**What:** Custom dark theme using JUCE's LookAndFeel_V4 ColourScheme system.

**When to use:** Plugin editor initialization.

**JUCE's built-in dark scheme (getDarkColourScheme):**

| UIColour | Hex | Description |
|----------|-----|-------------|
| windowBackground | `0xff323e44` | Dark blue-grey |
| widgetBackground | `0xff263238` | Darker blue-grey |
| menuBackground | `0xff323e44` | Same as window |
| outline | `0xff8e989b` | Medium grey |
| defaultText | `0xffffffff` | White |
| defaultFill | `0xff42a2c8` | Teal/cyan accent |
| highlightedText | `0xffffffff` | White |
| highlightedFill | `0xff181f22` | Very dark |
| menuText | `0xffffffff` | White |

**Recommendation:** Start with `LookAndFeel_V4` default (which IS the dark scheme — the constructor calls `getDarkColourScheme()` automatically). For a more "studio" feel, create a custom ColourScheme with slightly warmer/neutral tones:

```cpp
// Source: JUCE LookAndFeel_V4.cpp (official source)
// Custom dark scheme for DAW plugin aesthetic
juce::LookAndFeel_V4::ColourScheme getChordPumperColourScheme()
{
    return {
        0xff1e1e2e,  // windowBackground  — very dark, near-black
        0xff181825,  // widgetBackground   — slightly darker
        0xff2a2a3a,  // menuBackground     — subtle contrast
        0xff4a4a5a,  // outline            — muted grey
        0xffe0e0e0,  // defaultText        — soft white (not harsh)
        0xff6c8ebf,  // defaultFill        — muted blue accent
        0xffffffff,  // highlightedText    — bright white
        0xff3a3a4a,  // highlightedFill    — subtle highlight
        0xffe0e0e0   // menuText           — soft white
    };
}
```

**Implementation pattern:**
```cpp
// Source: JUCE LookAndFeel tutorial (official)
class ChordPumperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordPumperLookAndFeel()
        : juce::LookAndFeel_V4(getChordPumperColourScheme())
    {
    }
};
```

Apply in the editor constructor:
```cpp
// In PluginEditor constructor
setLookAndFeel(&lookAndFeel);

// CRITICAL: In PluginEditor destructor
setLookAndFeel(nullptr);
```

### Pattern 5: Minimal AudioProcessor for Phase 1

**What:** The simplest valid AudioProcessor that passes pluginval.

**Example:**
```cpp
// Source: JUCE AudioPlugin example + CMake API docs
class ChordPumperProcessor : public juce::AudioProcessor
{
public:
    ChordPumperProcessor()
        : AudioProcessor(BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    // Phase 1: no-op audio processing — outputs silence
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        buffer.clear();
    }

    // Editor
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    // Metadata
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs (required stubs)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State (Phase 1: no state to save)
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};
```

**Key points:**
- `BusesProperties().withOutput("Output", stereo, true)` — declares stereo output bus. Required when `IS_SYNTH TRUE`.
- `buffer.clear()` in processBlock — outputs silence. Valid behavior for an instrument with no audio engine.
- `acceptsMidi() = true`, `producesMidi() = true`, `isMidiEffect() = false` — must match CMake flags.
- Programs stubs return 1 program — pluginval requires at least one program.

### Anti-Patterns to Avoid

- **Don't use Projucer:** Legacy tool. JUCE team recommends CMake for all new projects. Projucer doesn't support CLAP and generates brittle IDE project files.
- **Don't skip EXCLUDE_FROM_ALL on clap-juce-extensions:** Without it, CJE's internal test targets get added to your build, increasing build time.
- **Don't use `IS_MIDI_EFFECT TRUE` for an Instrument:** Sets the VST3 category to "Fx" and the plugin won't appear in Bitwig's Instrument browser.
- **Don't omit audio output buses when IS_SYNTH is TRUE:** Hosts expect instruments to have audio outputs. Omitting them causes load failures or crashes in some hosts.
- **Don't use `juce_generate_juce_header()`:** Unnecessary since JUCE 6. Include module headers directly (e.g., `#include <juce_audio_processors/juce_audio_processors.h>`).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plugin format wrappers | Custom VST3/CLAP wrappers | `juce_add_plugin()` + `clap_juce_extensions_plugin()` | Hundreds of edge cases in format specs |
| Dark theme base | Full custom painting | `LookAndFeel_V4` with `ColourScheme` | Handles all standard component rendering; override only what you need |
| Build system | Makefiles, shell scripts | CMake with Ninja generator | JUCE requires CMake; Ninja provides fast incremental builds |
| Plugin validation | Manual DAW testing | pluginval + clap-validator | Automated, reproducible, catches regressions |
| Compilation caching | Partial rebuilds | ccache | Zero-config dramatic speedup |

**Key insight:** Phase 1 is pure infrastructure. Every component has a well-tested standard solution. The only custom code is the thin AudioProcessor/Editor shell.

## Common Pitfalls

### Pitfall 1: CLAP Binary Not Found by Bitwig
**What goes wrong:** Build succeeds but Bitwig doesn't see the CLAP plugin.
**Why it happens:** `COPY_PLUGIN_AFTER_BUILD` only copies VST3 to `~/.vst3/`. CLAP binaries are NOT auto-copied.
**How to avoid:** Add a post-build copy command or manual install step:
```bash
mkdir -p ~/.clap
cp build/debug/ChordPumper_artefacts/CLAP/ChordPumper.clap ~/.clap/
```
Or add to CMakeLists.txt:
```cmake
add_custom_command(TARGET ChordPumper_CLAP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.clap"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ChordPumper_CLAP>" "$ENV{HOME}/.clap/"
)
```
**Warning signs:** VST3 loads but CLAP doesn't appear in Bitwig's plugin list.

### Pitfall 2: isMidiEffect() Mismatch With CMake Flags
**What goes wrong:** Plugin crashes on load or fails pluginval.
**Why it happens:** `AudioProcessor::isMidiEffect()` returns a value that contradicts the CMake `IS_MIDI_EFFECT` flag. JUCE's VST3 wrapper checks these at runtime.
**How to avoid:** Ensure C++ overrides exactly match CMake configuration:
- `IS_SYNTH TRUE` ↔ `isMidiEffect() = false`
- `NEEDS_MIDI_INPUT TRUE` ↔ `acceptsMidi() = true`
- `NEEDS_MIDI_OUTPUT TRUE` ↔ `producesMidi() = true`
**Warning signs:** pluginval "Plugin reported different MIDI I/O" error.

### Pitfall 3: Missing Linux Dependencies
**What goes wrong:** CMake configure or build fails with missing headers/libraries.
**Why it happens:** JUCE depends on X11, ALSA, freetype2, and other Linux packages that aren't always installed.
**How to avoid:** Install all dependencies BEFORE first build:
```bash
sudo pacman -S --needed \
  alsa-lib jack2 freetype2 fontconfig \
  libx11 libxcomposite libxcursor libxext \
  libxinerama libxrandr libxrender \
  mesa glu curl pkgconf
```
**Warning signs:** CMake errors mentioning `X11`, `Freetype`, `ALSA`, or `pkg-config` not found.

### Pitfall 4: clap-juce-extensions Submodule Not Recursive
**What goes wrong:** Build fails with missing clap headers.
**Why it happens:** clap-juce-extensions has its own submodules (clap, clap-helpers). A shallow `git submodule add` doesn't fetch them.
**How to avoid:** Always use `--recursive`:
```bash
git submodule update --init --recursive
```
**Warning signs:** CMake error: "Could not find clap/clap.h" or similar.

### Pitfall 5: LookAndFeel Lifetime
**What goes wrong:** Crash on plugin close — dangling LookAndFeel pointer.
**Why it happens:** Component still references a LookAndFeel that was destroyed.
**How to avoid:** In the PluginEditor destructor, ALWAYS call `setLookAndFeel(nullptr)` before the LookAndFeel object is destroyed. If the LookAndFeel is a member of the Editor, ensure it's declared BEFORE any Component members that use it (C++ destruction is reverse declaration order).
**Warning signs:** Use-after-free crash when closing plugin window. May only appear intermittently.

### Pitfall 6: pluginval Doesn't Validate CLAP
**What goes wrong:** Assumes pluginval covers all formats, ships broken CLAP.
**Why it happens:** pluginval supports VST3 and LV2 as of v1.0.4. No CLAP support.
**How to avoid:** Use `clap-validator` (from free-audio) for CLAP validation:
```bash
# Install via cargo
cargo install --git https://github.com/free-audio/clap-validator
# Or from AUR
paru -S clap-validator

# Validate
clap-validator validate build/debug/ChordPumper_artefacts/CLAP/ChordPumper.clap
```
**Warning signs:** Only running pluginval and assuming full coverage.

## Code Examples

### Complete CMakeLists.txt for Phase 1

```cmake
# Source: JUCE CMake API docs + clap-juce-extensions README (both official)
cmake_minimum_required(VERSION 3.22)

project(ChordPumper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ccache
find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

# JUCE
add_subdirectory(libs/JUCE)

# CLAP extensions
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

# Plugin target
juce_add_plugin(ChordPumper
    COMPANY_NAME           "ChordPumperDev"
    PLUGIN_MANUFACTURER_CODE Cpdv
    PLUGIN_CODE            Cpmp
    IS_SYNTH               TRUE
    NEEDS_MIDI_INPUT       TRUE
    NEEDS_MIDI_OUTPUT      TRUE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS                VST3 Standalone
    PRODUCT_NAME           "ChordPumper"
    PLUGIN_NAME            "ChordPumper"
    DESCRIPTION            "Chord progression discovery instrument"
)

# CLAP format
clap_juce_extensions_plugin(
    TARGET ChordPumper
    CLAP_ID "org.chordpumper.chordpumper"
    CLAP_FEATURES instrument utility
)

target_sources(ChordPumper PRIVATE
    src/PluginProcessor.cpp
    src/ui/PluginEditor.cpp
)

target_include_directories(ChordPumper PRIVATE src)

target_compile_definitions(ChordPumper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordPumper
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

### CMakePresets.json

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    }
  ],
  "buildPresets": [
    { "name": "debug", "configurePreset": "debug" },
    { "name": "release", "configurePreset": "release" }
  ]
}
```

### Git Submodule Setup

```bash
# Source: JUCE + clap-juce-extensions official READMEs
git init
git submodule add -b master https://github.com/juce-framework/JUCE.git libs/JUCE
cd libs/JUCE && git checkout 8.0.12 && cd ../..

git submodule add -b main https://github.com/free-audio/clap-juce-extensions.git libs/clap-juce-extensions
cd libs/clap-juce-extensions && git checkout 02f91b7 && cd ../..

git submodule update --init --recursive
```

### Build and Install Commands

```bash
# Configure
cmake --preset debug

# Build
cmake --build build/debug

# Symlink compile_commands.json for clangd
ln -sf build/debug/compile_commands.json .

# Run standalone
./build/debug/ChordPumper_artefacts/Standalone/ChordPumper

# Install CLAP for Bitwig (VST3 auto-copied by COPY_PLUGIN_AFTER_BUILD)
mkdir -p ~/.clap
cp build/debug/ChordPumper_artefacts/CLAP/ChordPumper.clap ~/.clap/

# Validate VST3
pluginval --strictness-level 5 --validate ~/.vst3/ChordPumper.vst3

# Validate CLAP
clap-validator validate ~/.clap/ChordPumper.clap
```

### Recommended .gitignore

```gitignore
# Build output
build/
Builds/
cmake-build-*/

# CMake generated
CMakeLists.txt.user
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
install_manifest.txt
CTestTestfile.cmake
_deps/

# Compile database (symlinked from build dir)
compile_commands.json

# IDE
.idea/
.vscode/
*.swp
*~

# OS
.DS_Store
Thumbs.db

# Testing artifacts
Testing/
```

### Bitwig Plugin Scan Paths (Linux)

| Format | User Path | System Path |
|--------|-----------|-------------|
| VST3 | `~/.vst3/` | `/usr/lib/vst3/`, `/usr/local/lib/vst3/` |
| CLAP | `~/.clap/` | `/usr/lib/clap/` |

Bitwig scans these on startup. After installing a new plugin, use **Settings → Plug-ins → Rescan** or restart Bitwig.

### Linux System Dependencies (CachyOS / Arch)

```bash
# Source: JUCE Linux Dependencies doc (official) + verified on CachyOS
# Build essentials
sudo pacman -S --needed base-devel cmake ninja ccache git

# JUCE runtime dependencies
sudo pacman -S --needed \
  alsa-lib jack2 freetype2 fontconfig \
  libx11 libxcomposite libxcursor libxext \
  libxinerama libxrandr libxrender \
  mesa glu curl pkgconf

# Development tooling (clangd for IDE)
sudo pacman -S --needed clang

# Plugin validation (optional)
# pluginval: download from https://github.com/Tracktion/pluginval/releases
# clap-validator: cargo install --git https://github.com/free-audio/clap-validator
#   or: paru -S clap-validator (AUR)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Projucer for project generation | CMake with `juce_add_plugin()` | JUCE 6 (2020) | Projucer is legacy. All new JUCE projects should use CMake. |
| No CLAP support in JUCE | clap-juce-extensions for CLAP | 2022 | Native CLAP planned for JUCE 9 (no ETA as of Q3 2025 roadmap). Use CJE for now. |
| `juce_generate_juce_header()` | Direct module includes | JUCE 6+ | JuceHeader.h is unnecessary. Include specific module headers. |
| Catch2 v2 (header-only) | Catch2 v3 (compiled library) | 2022 | v2 is EOL. v3 compiles once as static lib → much faster incremental builds. |
| pluginval for all formats | pluginval (VST3) + clap-validator (CLAP) | 2024+ | pluginval added LV2 in v1.0.4 but still no CLAP. Use dedicated clap-validator. |

**Deprecated/outdated:**
- **Projucer**: Legacy, does not support CLAP, poor CI integration. Never use for new projects.
- **JUCE's built-in `UnitTest` class**: Primitive test framework. Use Catch2 v3 instead.
- **`JUCE_USE_CURL=1`**: Unnecessary for MIDI-only plugin. Adds libcurl dependency for no benefit.
- **`JUCE_WEB_BROWSER=1`**: Unnecessary. Adds WebKit dependency. Native JUCE components are appropriate.

## Open Questions

1. **Instrument category vs. MIDI-only output**
   - What we know: User decided "Instrument (not MIDI Effect)." In VST3, `IS_SYNTH TRUE` categorizes as Instrument. In CLAP, "instrument" feature means "processes notes and produces audio."
   - What's unclear: ChordPumper outputs MIDI, not audio. Will Bitwig's signal routing work correctly with an Instrument that outputs silence + MIDI? Most likely yes — Bitwig supports chaining Instruments — but needs verification in Phase 1 testing.
   - Recommendation: Proceed with `IS_SYNTH TRUE` and CLAP feature "instrument". Verify during Phase 1 testing that Bitwig routes MIDI output downstream correctly. If issues arise, the category can be reconsidered.

2. **COPY_PLUGIN_AFTER_BUILD for CLAP**
   - What we know: JUCE's COPY_PLUGIN_AFTER_BUILD copies VST3 to `~/.vst3/` on Linux. CLAP is NOT covered (CJE creates its own target outside JUCE's copy mechanism).
   - What's unclear: Whether `add_custom_command(TARGET ChordPumper_CLAP POST_BUILD ...)` is the best approach, or if a simple install script is cleaner.
   - Recommendation: Use a CMake post-build command for the CLAP target. Document both approaches for the planner.

3. **pluginval strictness level**
   - What we know: Success criteria says "passes pluginval at basic strictness." Level 5 is the minimum for "Verified by pluginval" certification.
   - What's unclear: Whether level 5 is appropriate for a Phase 1 shell with no real functionality.
   - Recommendation: Start with strictness level 5. If tests fail on expected gaps (e.g., no parameters), drop to level 3 for Phase 1 and increase in later phases.

## Sources

### Primary (HIGH confidence)
- JUCE CMake API docs: `https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md` — All juce_add_plugin flags, PLUGIN_CODE/MANUFACTURER_CODE format requirements
- JUCE AudioPlugin CMake example: `https://github.com/juce-framework/JUCE/blob/master/examples/CMake/AudioPlugin/CMakeLists.txt` — Reference CMakeLists.txt
- clap-juce-extensions README: `https://github.com/free-audio/clap-juce-extensions` — CMake integration pattern, CLAP_FEATURES, submodule setup
- CLAP plugin-features.h: `https://github.com/free-audio/clap/blob/main/include/clap/plugin-features.h` — Official CLAP feature definitions (instrument vs note-effect)
- JUCE LookAndFeel_V4 source: `juce_gui_basics/lookandfeel/juce_LookAndFeel_V4.cpp` — getDarkColourScheme hex values
- VST3 plugin locations (Steinberg): `https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html` — Linux scan paths
- pluginval README: `https://github.com/Tracktion/pluginval` — CLI usage, strictness levels, CMake integration
- clap-validator README: `https://github.com/free-audio/clap-validator` — CLAP-specific validation tool
- clap-juce-extensions commit history: `https://github.com/free-audio/clap-juce-extensions/commits/main` — Latest commit 02f91b7 (Jan 27, 2026)

### Secondary (MEDIUM confidence)
- Pamplejuce template .gitignore: `https://github.com/sudara/pamplejuce` — Community standard .gitignore patterns
- Bitwig plugin paths: `https://www.bitwig.com/userguide/latest/vst_plug-in_handling_and_options/` — Bitwig official docs confirm VST3 scan paths
- CLAP scan paths: Community consensus from multiple DAW documentation sources (Zrythm, REAPER forums, clap-host repo)

### Tertiary (LOW confidence)
- CLAP COPY_PLUGIN_AFTER_BUILD workaround: Based on CMake patterns, not officially documented by CJE

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — All versions verified against official releases and local system
- Architecture: HIGH — CMake patterns from official JUCE docs + CJE README
- Pitfalls: HIGH — Common issues well-documented on JUCE forum and GitHub issues
- Dark theme: MEDIUM — Built-in scheme values verified from source; custom palette is recommendation only
- Instrument categorization: MEDIUM — Correct per VST3/CLAP specs, but runtime behavior in Bitwig with MIDI-only instrument needs Phase 1 verification

**Research date:** 2026-02-19
**Valid until:** ~2026-05-19 (JUCE 8.0.12 is stable; CJE main branch may advance)
