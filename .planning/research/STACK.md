# Stack Research

**Domain:** JUCE/C++ Audio Plugin (MIDI)
**Researched:** 2026-02-19
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended | Confidence |
|------------|---------|---------|-----------------|------------|
| JUCE | 8.0.12 | Application framework, plugin formats, GUI, MIDI I/O | Industry-standard C++ audio framework. Handles VST3 wrapper, GUI, MIDI file I/O, and cross-platform abstractions. Dual-licensed AGPLv3/commercial. Latest stable release Dec 2024. | HIGH |
| C++ | C++20 | Language standard | JUCE requires C++17 minimum. C++20 adds `std::ranges`, `std::span`, concepts, and `<format>` — all useful for clean music-theory code. GCC 15 on CachyOS fully supports C++20. No cross-platform concern since Linux-only target. | HIGH |
| CMake | 3.22+ (system: 4.2.3) | Build system | JUCE's native build system since JUCE 6. `juce_add_plugin()` API handles all format targets. JUCE requires minimum 3.22; CachyOS ships 4.2.3. | HIGH |
| clap-juce-extensions | main branch (HEAD) | CLAP format support | Only way to build CLAP plugins with JUCE until JUCE 9 ships native CLAP. Mature — used by Surge XT, ChowDSP, Dexed. MIT licensed. Pin to a known-good commit for stability. | HIGH |
| Ninja | 1.13.2 | Build backend | 3-5x faster incremental builds than Make. CMake generates Ninja files natively with `-G Ninja`. Already installed on CachyOS. | HIGH |

### Supporting Libraries

| Library | Version | Purpose | When to Use | Confidence |
|---------|---------|---------|-------------|------------|
| Catch2 | v3.13.0 | Unit testing | Test music theory engine, chord generation, MIDI output logic. v3 is the actively maintained line (v2 is EOL). Integrates cleanly with CMake via `FetchContent`. | HIGH |
| (custom) Music Theory Engine | N/A — build from scratch | Intervals, scales, chord construction, harmonic proximity | No production-quality C++ music theory library exists. The available ones (mdructor/mt: 2 stars, no README; spectaclelabs/miles: scales only; pd3v/diatonic: GPL-3.0) are too immature, too narrow, or license-incompatible. Roll your own — the domain is well-defined and benefits from tight JUCE MIDI integration. | HIGH |

### JUCE Modules Used

| Module | Purpose | Notes |
|--------|---------|-------|
| `juce_audio_processors` | Plugin format wrappers (VST3, Standalone) | Core module — AudioProcessor base class |
| `juce_audio_basics` | MidiMessage, MidiBuffer, MidiFile, MidiMessageSequence | Chord MIDI output, drag-and-drop MIDI file creation |
| `juce_gui_basics` | Component, LookAndFeel, mouse/keyboard events | Grid UI, pad components, drag interactions |
| `juce_gui_extra` | `performExternalDragDropOfFiles` | MIDI file drag-and-drop to DAW (see Pitfalls) |
| `juce_data_structures` | ValueTree, UndoManager | Plugin state persistence, preset management |
| `juce_core` | File, MemoryOutputStream, JSON, Strings | Utility layer |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| GCC 15.2 | Primary compiler | Ships with CachyOS. Full C++20 support. Use `-Wall -Wextra -Wpedantic`. |
| Clang 21.1 | Alternative compiler + tooling | Available on CachyOS. Use for `clangd` language server even if compiling with GCC. |
| clangd | Language server for IDE | Requires `compile_commands.json` — enable with `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`. Far superior to cpptools for JUCE codebases. |
| clang-format | Code formatting | Use JUCE-compatible `.clang-format` config. Run on save. |
| clang-tidy | Static analysis | Catches thread-safety issues, modernization opportunities. |
| ccache | Compilation caching | Dramatically speeds up rebuilds. Configure via `CMAKE_CXX_COMPILER_LAUNCHER=ccache`. |
| GDB / LLDB | Debugger | GDB for GCC builds, LLDB for Clang. Standalone target enables direct debugging without DAW. |
| Pluginval | Plugin validation | JUCE's official plugin validator. Run against VST3 output to catch host-compatibility issues before manual testing. |

## Installation / Setup

### System Dependencies (CachyOS / Arch)

```bash
# Build essentials (likely already installed)
sudo pacman -S --needed base-devel cmake ninja ccache git

# JUCE Linux dependencies
sudo pacman -S --needed \
  alsa-lib \
  jack2 \
  freetype2 \
  fontconfig \
  libx11 \
  libxcomposite \
  libxcursor \
  libxext \
  libxinerama \
  libxrandr \
  libxrender \
  webkit2gtk-4.1 \
  mesa \
  glu \
  curl \
  pkgconf

# Development tooling
sudo pacman -S --needed clang lld
```

### Project Structure

```
ChordGrid/
├── CMakeLists.txt              # Root CMake — project config
├── CMakePresets.json            # Debug/Release presets
├── libs/
│   ├── JUCE/                   # git submodule: juce-framework/JUCE @ 8.0.12
│   └── clap-juce-extensions/   # git submodule: free-audio/clap-juce-extensions @ main
├── src/
│   ├── PluginProcessor.h/cpp   # AudioProcessor — MIDI output, state
│   ├── PluginEditor.h/cpp      # AudioProcessorEditor — grid UI
│   ├── theory/                 # Music theory engine (intervals, chords, scales)
│   └── grid/                   # Grid logic (morphing, harmonic proximity)
├── test/
│   └── ...                     # Catch2 tests for theory engine
└── resources/                  # Icons, presets, etc.
```

### CMakeLists.txt Skeleton

```cmake
cmake_minimum_required(VERSION 3.22)

project(ChordGrid VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler cache
find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

# JUCE
add_subdirectory(libs/JUCE)

# CLAP extensions
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

# Plugin target
juce_add_plugin(ChordGrid
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE YRNM
    PLUGIN_CODE Chgd
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    FORMATS VST3 Standalone
    PRODUCT_NAME "Chord Grid"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# CLAP format via clap-juce-extensions
clap_juce_extensions_plugin(
    TARGET ChordGrid
    CLAP_ID "com.yourname.chord-grid"
    CLAP_FEATURES "note-effect" "utility"
)

target_sources(ChordGrid PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    # ... theory/, grid/ sources
)

target_compile_definitions(ChordGrid PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ChordGrid
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

# --- Tests ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(ChordGrid_Tests test/main.cpp)
    target_link_libraries(ChordGrid_Tests PRIVATE Catch2::Catch2WithMain)
    # Link theory engine sources here
endif()
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
git init
git submodule add -b master https://github.com/juce-framework/JUCE.git libs/JUCE
cd libs/JUCE && git checkout 8.0.12 && cd ../..

git submodule add -b main https://github.com/free-audio/clap-juce-extensions.git libs/clap-juce-extensions
git submodule update --init --recursive
```

### Build Commands

```bash
# Configure (debug)
cmake --preset debug

# Build
cmake --build build/debug

# Symlink compile_commands.json for clangd
ln -sf build/debug/compile_commands.json .

# Run standalone for quick testing
./build/debug/ChordGrid_artefacts/Standalone/ChordGrid

# Run tests
./build/debug/ChordGrid_Tests
```

### VST3/CLAP Installation

```bash
# VST3 — Bitwig scans ~/.vst3/ by default
mkdir -p ~/.vst3
cp -r build/release/ChordGrid_artefacts/VST3/ChordGrid.vst3 ~/.vst3/

# CLAP — Bitwig scans ~/.clap/ by default
mkdir -p ~/.clap
cp build/release/ChordGrid_artefacts/CLAP/ChordGrid.clap ~/.clap/
```

## Alternatives Considered

| Category | Recommended | Alternative | When to Use Alternative |
|----------|-------------|-------------|------------------------|
| Framework | JUCE 8 | iPlug2 | Only if you need a header-only, MIT-licensed framework. JUCE is more mature for Linux, has better docs, larger community. |
| Framework | JUCE 8 | DPF (DISTRHO Plugin Framework) | If you want CLAP/VST3/LV2 natively without JUCE licensing. Lighter, but no built-in MIDI file I/O, weaker GUI toolkit. |
| CLAP support | clap-juce-extensions | Wait for JUCE 9 native CLAP | Only if timeline allows waiting for an unannounced release date. JUCE 9 has no ETA. Ship now with clap-juce-extensions. |
| Build system | CMake + Ninja | Projucer | Never for new projects in 2026. Projucer is legacy; JUCE team recommends CMake. |
| Build system | CMake + Ninja | Meson | Only if team is Meson-native. JUCE has zero Meson support — you'd be fighting the framework. |
| Testing | Catch2 v3 | GoogleTest | If team already uses GTest. Catch2 is lighter, more expressive for value-based testing. GTest better for mocking (GMock). |
| Music theory | Custom engine | mdructor/mt | Do not use — 2 GitHub stars, empty README, unknown maintenance. |
| Music theory | Custom engine | pd3v/diatonic | Do not use — GPL-3.0 license forces copyleft on your entire codebase. |
| C++ standard | C++20 | C++17 | Only if targeting cross-platform with old compilers. Linux-only with GCC 15 → C++20 is safe and beneficial. |
| Compiler | GCC 15 | Clang 21 | Either works. GCC produces slightly faster release builds on Linux. Use both in CI if you set up CI. |
| Dep management | Git submodules | CPM (CMake Package Manager) | CPM is cleaner for many deps, but for just JUCE + clap-juce-extensions, submodules are simpler and let you pin exact commits. |
| Dep management | Git submodules | vcpkg / Conan | Overkill for 2 dependencies. These shine when you have 10+ transitive deps. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Projucer | Legacy tool. Generates IDE-specific project files, doesn't support CLAP, poor CI integration, no CMake preset support. JUCE team themselves recommend CMake for new projects. | CMake with `juce_add_plugin()` |
| JUCE's built-in `UnitTest` class | Primitive — no test discovery, no matchers, no sections, no parameterized tests. Exists for internal JUCE testing, not for application code. | Catch2 v3 |
| `juce_audio_plugin_client/LV2` | LV2 requires a forked JUCE with patches. Unnecessary complexity when Bitwig supports VST3 + CLAP natively. | VST3 + CLAP via clap-juce-extensions |
| Any external music theory library | None are production-ready for C++. All discovered options are unmaintained (<5 stars), incomplete, or GPL-encumbered. | Custom theory engine — well-scoped, testable, JUCE-integrated |
| `JUCE_USE_CURL=1` | Pulls in libcurl dependency for no reason — this is a MIDI plugin with no network needs. | `JUCE_USE_CURL=0` |
| `JUCE_WEB_BROWSER=1` | Pulls in WebKit dependency. Only needed for JUCE's WebView UI. A chord-pad grid doesn't need web tech — native JUCE components are faster and simpler. | `JUCE_WEB_BROWSER=0` + native Component UI |
| Make (default CMake generator) | Slow incremental builds, no build-progress reporting, poor parallel build heuristics compared to Ninja. | Ninja (`-G Ninja`) |
| Header-only Catch2 (v2.x) | EOL. v3 compiles as a static library — faster compile times after initial build. v2 added 5-10s to every TU that included the header. | Catch2 v3 via `FetchContent` |

## Version Compatibility Matrix

| Component | Version | Compatibility Notes |
|-----------|---------|-------------------|
| JUCE | 8.0.12 | Latest stable. Pin via git submodule tag. Check releases quarterly. |
| clap-juce-extensions | main (pin commit) | Supports JUCE 6/7/8. Track main branch, pin to tested commit hash. No semver releases — use commit SHA. |
| CMake | >= 3.22 | JUCE minimum. CachyOS ships 4.2.3 — well above minimum. |
| GCC | 15.2 | CachyOS rolling. Full C++20 + C++23 partial. |
| Clang | 21.1 | CachyOS rolling. Full C++20 + C++23 partial. Used primarily for clangd. |
| Catch2 | 3.13.0 | Latest stable (Feb 2026). Use `FetchContent` to pin exact version. |
| Ninja | 1.13.2 | CachyOS ships current. No compatibility concerns. |
| Bitwig Studio | 5.x | VST3 and CLAP scanning from `~/.vst3/` and `~/.clap/`. MIDI effect routing supported. |

## JUCE Licensing Note

JUCE 8 is dual-licensed: **AGPLv3** or **commercial**.

- AGPLv3 is fine for open-source distribution
- Personal/Indie license (free up to $20K revenue, $40/mo up to $300K) required for closed-source
- The project description doesn't specify licensing intent — decide before first release
- clap-juce-extensions is MIT — no licensing concern there

## Key Technical Decisions

### Why C++20 over C++17

JUCE requires C++17 minimum, but for a Linux-only plugin with GCC 15:

- **`std::ranges`** — cleaner chord/scale filtering and transformation pipelines
- **`std::span`** — zero-copy views into MIDI buffers
- **`concepts`** — constrain template interfaces in the theory engine
- **Designated initializers** — readable struct construction for chord definitions
- **`std::format`** — cleaner debug/log strings (GCC 15 supports it)
- **No portability cost** — Linux-only with a modern compiler

### Why Roll Your Own Music Theory Engine

The chord-grid morphing logic is the product's core differentiator. It needs:

1. Interval arithmetic (semitones, scale degrees)
2. Chord construction from root + intervals
3. Scale membership testing
4. Harmonic proximity scoring (the "morph" algorithm)
5. MIDI note number generation from chord voicings

This is ~500-1000 lines of well-defined, heavily-tested code. External libraries either don't cover harmonic proximity at all, or would add dependency risk for trivial savings. Keep it in-house, test exhaustively with Catch2.

### Why `performExternalDragDropOfFiles` for MIDI Drag-and-Drop

JUCE provides `DragAndDropContainer::performExternalDragDropOfFiles()` which triggers the OS-level X11 drag-and-drop protocol. The workflow:

1. User long-presses or drags a chord pad
2. Plugin writes a temporary `.mid` file via `juce::MidiFile::writeTo()`
3. Plugin calls `performExternalDragDropOfFiles()` with the temp file path
4. Bitwig receives the file drop as a MIDI clip

**Linux caveat:** External drag-and-drop from plugin windows on Linux has historically been unreliable in JUCE. There are JUCE forum patches (thread #35203) addressing X11 XEmbed issues. Test early on Bitwig. If it doesn't work, fallback to a "copy MIDI to clipboard" or "save .mid file" workflow. This is a known risk area (see PITFALLS.md).

## Sources

- JUCE 8.0.12 release: https://github.com/juce-framework/JUCE/releases (HIGH confidence — official GitHub)
- JUCE CMake API: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md (HIGH confidence — official docs)
- JUCE Linux dependencies: https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md (HIGH confidence — official docs)
- JUCE licensing: https://juce.com/get-juce (HIGH confidence — official site)
- JUCE C++17 minimum: https://github.com/juce-framework/JUCE/blob/master/modules/juce_core/system/juce_CompilerSupport.h (HIGH confidence — source code)
- JUCE roadmap (CLAP in JUCE 9): https://juce.com/blog/juce-roadmap-update-q3-2025/ (HIGH confidence — official blog)
- clap-juce-extensions: https://github.com/free-audio/clap-juce-extensions (HIGH confidence — official repo, README verified)
- Catch2 v3.13.0: https://github.com/catchorg/Catch2/releases (HIGH confidence — official releases)
- Ninja 1.13.2: https://github.com/ninja-build/ninja/releases (HIGH confidence — official releases)
- JUCE drag-and-drop Linux issues: https://forum.juce.com/t/bug-patch-implements-drag-and-drop-for-plugin-windows-on-linux/35203 (MEDIUM confidence — forum report, not official fix status)
- JUCE MidiFile API: https://docs.juce.com/master/classMidiFile.html (HIGH confidence — official API docs)
- Arch JUCE package (dep reference): https://archlinux.org/packages/extra/x86_64/juce/ (HIGH confidence — Arch official repos)
- GCC 15.2 on CachyOS: https://archlinux.org/packages/core/x86_64/gcc/ (HIGH confidence — verified locally: `gcc --version` → 15.2.1)
- CMake 4.2.3 on CachyOS: verified locally (`cmake --version`)
- Ninja 1.13.2 on CachyOS: verified locally (`ninja --version`)
- Clang 21.1 on CachyOS: verified locally (`clang --version`)
