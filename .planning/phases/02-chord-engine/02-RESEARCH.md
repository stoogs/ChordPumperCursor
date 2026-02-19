# Phase 2: Chord Engine - Research

**Researched:** 2026-02-19
**Domain:** Music theory primitives in C++20 — note representation, chord construction, naming, and testing
**Confidence:** HIGH

## Summary

Phase 2 builds the music theory core of ChordPumper: data types for notes and intervals, a chord construction engine that produces all v1 chord types from any root note, and a naming system that generates display strings like "Dm7" and "F#aug". The engine lives in `src/engine/` and has **zero JUCE dependency** — it is pure C++20, making it trivially testable with Catch2 without pulling in the plugin framework.

The domain is well-understood and deterministic. Chord construction reduces to a lookup table of semitone intervals per chord type (9 types: 4 triads + 5 sevenths) applied to a root pitch class. Chord naming is string concatenation of the root's display name and a quality suffix. The primary complexity is **enharmonic spelling** — ensuring that pitch classes like C#/Db use the conventionally correct name in context. For Phase 2, a canonical set of 12 preferred root spellings (matching pop/jazz convention) is sufficient; context-sensitive respelling is a Phase 4 concern.

Catch2 v3.13.0 (released Feb 15, 2026) integrates cleanly via CMake FetchContent. The test executable links only against the engine library and Catch2 — no JUCE build overhead in the test cycle. With 9 chord types × 12 root notes = 108 chords, exhaustive testing is both feasible and required.

**Primary recommendation:** Build the engine as a static library (`ChordPumperEngine`) in `src/engine/` with no JUCE includes. Use `enum class` for note letters and chord types, `constexpr` lookup tables for interval data, and a simple `PitchClass` struct (letter + accidental offset). Test exhaustively with Catch2 v3.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CHRD-01 | Plugin supports triads (major, minor, diminished, augmented) and 7th chords (maj7, min7, dom7, dim7, half-dim7) | Interval tables for all 9 chord types documented. `ChordType` enum + `Chord::intervals()` lookup covers construction. Pure C++ engine with constexpr interval data. |
| CHRD-02 | Each pad displays the chord name (e.g., "Dm7", "F#aug") | Chord naming algorithm documented: root display name + quality suffix. Standard notation table provided. Enharmonic root spelling convention defined for all 12 pitch classes. |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++20 (GCC 15) | C++20 / GCC 15.2 | Language standard | Already configured in CMakeLists.txt. Provides constexpr, enum class, std::array, structured bindings. |
| Catch2 | v3.13.0 | Unit testing framework | Latest stable (Feb 2026). Compiled library — fast incremental builds. `Catch2::Catch2WithMain` eliminates boilerplate. Industry standard for C++ testing. |
| CMake FetchContent | CMake ≥3.14 | Catch2 acquisition | Downloads Catch2 at configure time. No git submodule needed. Clean separation from JUCE submodules. |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CTest | (bundled with CMake) | Test runner integration | `catch_discover_tests()` auto-registers TEST_CASEs with CTest. Run via `ctest --test-dir build/debug`. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Catch2 v3 | GoogleTest | GoogleTest is more verbose, requires more boilerplate. Catch2's `TEST_CASE`/`SECTION` pattern is more natural for exhaustive chord testing. |
| FetchContent for Catch2 | Git submodule | Submodule would add another entry to `.gitmodules`. FetchContent keeps test deps separate from plugin deps (JUCE, CJE). |
| Custom chord engine | Existing C++ music theory library (MusicTheoryCpp, mt, Music-Algebra) | Existing libraries are small/unmaintained hobby projects (1-13 GitHub stars). Our requirements are narrow (9 chord types, 12 roots) — a custom engine is ~200 lines, fully under our control, and has no transitive dependencies. |

**Installation:**
```cmake
# In root CMakeLists.txt (or a tests/CMakeLists.txt)
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.13.0
)
FetchContent_MakeAvailable(Catch2)
```

## Architecture Patterns

### Recommended Project Structure

```
src/
├── engine/
│   ├── NoteLetter.h          # enum class NoteLetter { C, D, E, F, G, A, B }
│   ├── PitchClass.h          # struct PitchClass { NoteLetter letter; int8_t accidental; }
│   ├── PitchClass.cpp         # PitchClass methods: name(), semitone(), midiNote(octave)
│   ├── ChordType.h           # enum class ChordType (9 types) + interval lookup
│   ├── Chord.h               # struct Chord { PitchClass root; ChordType type; }
│   └── Chord.cpp             # Chord methods: name(), intervals(), midiNotes(octave)
tests/
├── CMakeLists.txt            # Test executable, links ChordPumperEngine + Catch2
├── test_pitch_class.cpp      # PitchClass construction, naming, semitone conversion
├── test_chord.cpp            # Chord construction for all 9 types × 12 roots
└── test_chord_naming.cpp     # Chord name generation, enharmonic edge cases
```

**Key architectural decision:** The engine is a CMake STATIC library (`ChordPumperEngine`) with no JUCE dependency. The plugin target links against it. The test target links against it + Catch2. This gives:
- Fast test compilation (no JUCE headers)
- Clean dependency graph
- Engine code reusable outside plugin context

### Pattern 1: JUCE-Independent Engine Library

**What:** A static library for theory code that the plugin and tests both link against.
**When to use:** Any logic that doesn't need JUCE types (audio buffers, GUI components, etc.)

```cmake
# Engine library — pure C++20, no JUCE
add_library(ChordPumperEngine STATIC
    src/engine/PitchClass.cpp
    src/engine/Chord.cpp
)
target_include_directories(ChordPumperEngine PUBLIC src)
target_compile_features(ChordPumperEngine PUBLIC cxx_std_20)

# Plugin links the engine
target_link_libraries(ChordPumper PRIVATE ChordPumperEngine)

# Tests link the engine + Catch2 (no JUCE!)
add_executable(ChordPumperTests
    tests/test_pitch_class.cpp
    tests/test_chord.cpp
    tests/test_chord_naming.cpp
)
target_link_libraries(ChordPumperTests PRIVATE ChordPumperEngine Catch2::Catch2WithMain)
```

### Pattern 2: PitchClass with Letter + Accidental

**What:** Represent a pitch class (octave-independent note) as a letter name + accidental offset.
**When to use:** All chord construction and naming operations.

```cpp
namespace chordpumper {

enum class NoteLetter : uint8_t { C, D, E, F, G, A, B };

struct PitchClass {
    NoteLetter letter;
    int8_t accidental;  // -2=bb, -1=b, 0=natural, 1=#, 2=##

    constexpr int semitone() const;       // 0-11, C=0
    std::string name() const;             // "C", "F#", "Bb", etc.
    int midiNote(int octave) const;       // MIDI note number

    constexpr bool operator==(const PitchClass&) const = default;
};

// Canonical 12 pitch classes with preferred enharmonic spellings
namespace pitches {
    inline constexpr PitchClass C  {NoteLetter::C,  0};
    inline constexpr PitchClass Cs {NoteLetter::C,  1};  // C#
    inline constexpr PitchClass D  {NoteLetter::D,  0};
    inline constexpr PitchClass Eb {NoteLetter::E, -1};
    inline constexpr PitchClass E  {NoteLetter::E,  0};
    inline constexpr PitchClass F  {NoteLetter::F,  0};
    inline constexpr PitchClass Fs {NoteLetter::F,  1};  // F#
    inline constexpr PitchClass G  {NoteLetter::G,  0};
    inline constexpr PitchClass Ab {NoteLetter::A, -1};
    inline constexpr PitchClass A  {NoteLetter::A,  0};
    inline constexpr PitchClass Bb {NoteLetter::B, -1};
    inline constexpr PitchClass B  {NoteLetter::B,  0};
}

} // namespace chordpumper
```

**Why this spelling convention** (C, C#, D, Eb, E, F, F#, G, Ab, A, Bb, B):
- Matches standard pop/jazz chord charts
- Uses sharps for the black keys adjacent to natural notes with no flat equivalent in common use (C#, F#)
- Uses flats for the remaining black keys (Eb, Ab, Bb)
- The examples in CHRD-02 ("F#aug") confirm sharp notation is expected

### Pattern 3: Constexpr Interval Lookup Tables

**What:** Compile-time lookup tables mapping chord types to semitone intervals.
**When to use:** Chord construction — determining which notes are in a chord.

```cpp
enum class ChordType : uint8_t {
    Major, Minor, Diminished, Augmented,
    Maj7, Min7, Dom7, Dim7, HalfDim7
};

struct ChordIntervals {
    std::array<int, 4> semitones;  // offsets from root; -1 = unused
    int noteCount;                  // 3 for triads, 4 for sevenths
};

constexpr std::array<ChordIntervals, 9> kChordIntervals = {{
    // Triads
    {{0, 4, 7, -1}, 3},   // Major
    {{0, 3, 7, -1}, 3},   // Minor
    {{0, 3, 6, -1}, 3},   // Diminished
    {{0, 4, 8, -1}, 3},   // Augmented
    // Seventh chords
    {{0, 4, 7, 11}, 4},   // Maj7
    {{0, 3, 7, 10}, 4},   // Min7
    {{0, 4, 7, 10}, 4},   // Dom7
    {{0, 3, 6,  9}, 4},   // Dim7
    {{0, 3, 6, 10}, 4},   // HalfDim7
}};
```

### Pattern 4: Chord Naming via Suffix Lookup

**What:** Generate display names by concatenating root name with quality suffix.
**When to use:** Producing strings for pad labels (CHRD-02).

```cpp
constexpr std::array<const char*, 9> kChordSuffix = {
    "",       // Major       → "C", "F#"
    "m",      // Minor       → "Cm", "F#m"
    "dim",    // Diminished  → "Cdim", "F#dim"
    "aug",    // Augmented   → "Caug", "F#aug"
    "maj7",   // Maj7        → "Cmaj7"
    "m7",     // Min7        → "Dm7"
    "7",      // Dom7        → "G7"
    "dim7",   // Dim7        → "Bdim7"
    "m7b5",   // HalfDim7   → "Bm7b5"
};

std::string Chord::name() const {
    return root.name() + kChordSuffix[static_cast<int>(type)];
}
```

**Naming convention justification:**
- Major triad: no suffix (standard: "C" means C major)
- "m" for minor (matches CHRD-02 example "Dm7")
- "aug" for augmented (matches CHRD-02 example "F#aug")
- "dim" for diminished (universally understood)
- "7" for dominant 7th (standard jazz/pop convention — bare "7" always means dominant)
- "maj7" for major 7th (explicit "maj" distinguishes from dominant)
- "m7" for minor 7th (minor + seventh)
- "dim7" for diminished 7th (fully diminished)
- "m7b5" for half-diminished 7th (ASCII-friendly; the Unicode "ø" is an alternative but harder to render in all contexts)

### Pattern 5: Catch2 Test Structure for Exhaustive Chord Testing

**What:** Use Catch2 `GENERATE` to test all chord types across all 12 root notes.
**When to use:** Ensuring 108 chord combinations are all correct.

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "engine/Chord.h"
#include "engine/PitchClass.h"

using namespace chordpumper;

TEST_CASE("All triads produce correct interval count", "[chord]") {
    auto root = GENERATE(
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    );
    auto type = GENERATE(
        ChordType::Major, ChordType::Minor,
        ChordType::Diminished, ChordType::Augmented
    );

    Chord chord(root, type);
    REQUIRE(chord.noteCount() == 3);
}

TEST_CASE("Chord names match expected format", "[chord][naming]") {
    CHECK(Chord(pitches::D, ChordType::Min7).name() == "Dm7");
    CHECK(Chord(pitches::Fs, ChordType::Augmented).name() == "F#aug");
    CHECK(Chord(pitches::Bb, ChordType::Maj7).name() == "Bbmaj7");
    CHECK(Chord(pitches::C, ChordType::Major).name() == "C");
}
```

### Anti-Patterns to Avoid

- **Don't include JUCE headers in engine code:** The theory engine must be pure C++. If you need `juce::String`, convert at the boundary (in PluginProcessor or UI code), not in the engine.
- **Don't use `std::string` as a pitch class identifier:** String-based note representation is slow, error-prone, and requires parsing. Use structured types (letter enum + accidental int).
- **Don't hand-roll MIDI note number arithmetic without the semitone lookup table:** Mapping letters to semitones (C=0, D=2, E=4, F=5, G=7, A=9, B=11) is error-prone if done ad-hoc. Define it once in a constexpr array.
- **Don't represent chords as raw vectors of MIDI notes:** Chord identity (root + type) must be preserved for Phase 4 (morph engine needs to compare chord structures, not just note sets).
- **Don't skip the static library pattern:** If engine `.cpp` files are compiled directly into the plugin target, tests can't link against them without pulling in JUCE.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test framework | Custom test harness or JUCE UnitTest | Catch2 v3 with `Catch2::Catch2WithMain` | Auto test discovery, GENERATE for combinatorial testing, JUnit XML output for CI |
| Note-to-semitone mapping | Ad-hoc if/else chains | constexpr lookup array indexed by NoteLetter | Single source of truth, compile-time verified, no branching bugs |
| Chord interval definitions | Scattered magic numbers | Central constexpr interval table indexed by ChordType | One table to audit, impossible to have inconsistent intervals |

**Key insight:** The entire chord engine is essentially two lookup tables (letter→semitone, chordtype→intervals) plus string concatenation for naming. Keep it simple — the complexity in this project is in Phase 4 (morph algorithm), not Phase 2.

## Common Pitfalls

### Pitfall 1: Enharmonic Mismatch in Chord Names
**What goes wrong:** "Db" chord shows as "C#" or vice versa, confusing users.
**Why it happens:** Using a `int semitone` to represent a pitch class loses the letter/accidental distinction. Two pitch classes with the same semitone (C#=1, Db=1) become indistinguishable.
**How to avoid:** Always store `PitchClass` as `{NoteLetter, accidental}`, never as a bare semitone. The semitone is a derived value. Define a canonical set of 12 preferred spellings.
**Warning signs:** Tests pass for semitone values but display wrong note names.

### Pitfall 2: Off-by-One in MIDI Note Calculation
**What goes wrong:** Chords sound in the wrong octave or wrong pitch.
**Why it happens:** MIDI note 60 = C4 (middle C). The formula is `midiNote = semitone + (octave + 1) * 12`. Getting the octave offset wrong shifts everything by 12 semitones.
**How to avoid:** Define the formula once in `PitchClass::midiNote(int octave)` and test against known values: C4=60, A4=69, C5=72.
**Warning signs:** All intervals sound correct but the absolute pitch is wrong.

### Pitfall 3: Modular Arithmetic With Negative Accidentals
**What goes wrong:** `PitchClass::semitone()` returns negative values or values > 11 for flatted notes.
**Why it happens:** C's natural semitone is 0. Cb should be 11 (wrapping), but naive `0 + (-1) = -1`.
**How to avoid:** Use modular arithmetic: `((baseSemitone + accidental) % 12 + 12) % 12`. The double-mod-add pattern handles negatives correctly in C++.
**Warning signs:** Cb, Fb, or any flat of a natural note produces assertion failures or wrong MIDI notes.

### Pitfall 4: FetchContent Downloading on Every Build
**What goes wrong:** Build is slow because Catch2 is re-downloaded each configure.
**Why it happens:** FetchContent re-fetches if the build directory is cleaned or the cache is invalidated.
**How to avoid:** Set `FETCHCONTENT_UPDATES_DISCONNECTED ON` after initial configure. Or pin `GIT_SHALLOW TRUE` and `GIT_PROGRESS TRUE` to speed up initial fetch. The build directory caches the download — don't clean it unnecessarily.
**Warning signs:** `cmake --preset debug` takes 30+ seconds on subsequent runs.

### Pitfall 5: Test Target Built During Plugin Build
**What goes wrong:** Building the plugin also builds and links Catch2, slowing down the edit-compile-run cycle.
**Why it happens:** The test target is part of the default `ALL` build target.
**How to avoid:** Guard tests with an option: `option(CHORDPUMPER_BUILD_TESTS "Build tests" ON)` and use `EXCLUDE_FROM_ALL` on the test executable. Or conditionally add the `tests/` subdirectory. The test target should only build when explicitly requested (`cmake --build build --target ChordPumperTests`).
**Warning signs:** Full rebuild after engine changes takes much longer than expected.

### Pitfall 6: Chord Identity Lost in MIDI Conversion
**What goes wrong:** Phase 4's morph engine can't determine what chord type a set of MIDI notes represents.
**Why it happens:** Chords are converted to MIDI note arrays early and the root+type metadata is discarded.
**How to avoid:** The `Chord` struct (root + type) is the canonical representation. MIDI notes are a **derived output**, not the storage format. Never store chords as `std::vector<int>` — always as `Chord{root, type}`.
**Warning signs:** Downstream code needs to "reverse-engineer" chord types from MIDI note sets.

## Code Examples

### Complete PitchClass Semitone Mapping

```cpp
// Natural note semitones (C=0 through B=11)
constexpr std::array<int, 7> kNaturalSemitones = {
    0,  // C
    2,  // D
    4,  // E
    5,  // F
    7,  // G
    9,  // A
    11  // B
};

constexpr int PitchClass::semitone() const {
    int base = kNaturalSemitones[static_cast<int>(letter)];
    return ((base + accidental) % 12 + 12) % 12;
}

int PitchClass::midiNote(int octave) const {
    return semitone() + (octave + 1) * 12;
}
```

### PitchClass Display Name Generation

```cpp
constexpr std::array<const char*, 7> kLetterNames = {
    "C", "D", "E", "F", "G", "A", "B"
};

std::string PitchClass::name() const {
    std::string result = kLetterNames[static_cast<int>(letter)];
    if (accidental > 0) {
        for (int i = 0; i < accidental; ++i) result += '#';
    } else if (accidental < 0) {
        for (int i = 0; i < -accidental; ++i) result += 'b';
    }
    return result;
}
```

### Complete Chord Interval Table

```cpp
// Semitone intervals from root for each chord type
// Index matches ChordType enum order
constexpr std::array<std::array<int, 4>, 9> kIntervals = {{
    {0, 4, 7, -1},   // Major:     root, M3, P5
    {0, 3, 7, -1},   // Minor:     root, m3, P5
    {0, 3, 6, -1},   // Dim:       root, m3, d5
    {0, 4, 8, -1},   // Aug:       root, M3, A5
    {0, 4, 7, 11},   // Maj7:      root, M3, P5, M7
    {0, 3, 7, 10},   // Min7:      root, m3, P5, m7
    {0, 4, 7, 10},   // Dom7:      root, M3, P5, m7
    {0, 3, 6,  9},   // Dim7:      root, m3, d5, d7
    {0, 3, 6, 10},   // HalfDim7:  root, m3, d5, m7
}};

constexpr int noteCount(ChordType type) {
    return (static_cast<int>(type) < 4) ? 3 : 4;
}
```

### Chord MIDI Note Generation

```cpp
std::vector<int> Chord::midiNotes(int octave) const {
    int rootMidi = root.midiNote(octave);
    auto& intervals = kIntervals[static_cast<int>(type)];
    int count = noteCount(type);

    std::vector<int> notes;
    notes.reserve(count);
    for (int i = 0; i < count; ++i) {
        notes.push_back(rootMidi + intervals[i]);
    }
    return notes;
}
```

### CMake Test Integration

```cmake
# In root CMakeLists.txt, after plugin target
option(CHORDPUMPER_BUILD_TESTS "Build unit tests" ON)

if(CHORDPUMPER_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)

    add_executable(ChordPumperTests
        tests/test_pitch_class.cpp
        tests/test_chord.cpp
        tests/test_chord_naming.cpp
    )
    target_link_libraries(ChordPumperTests PRIVATE
        ChordPumperEngine
        Catch2::Catch2WithMain
    )
    catch_discover_tests(ChordPumperTests)
endif()
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Catch2 v2 (header-only, single-include) | Catch2 v3 (compiled static library) | 2022 | v2 is EOL. v3 compiles once → dramatically faster incremental test builds. |
| String-based note representation | Structured types (enum + int) | Always best practice in C++ | 2 bytes vs 24 bytes per pitch. No parsing, no invalid states. |
| Ad-hoc interval lists per chord | Central constexpr lookup table | C++17+ constexpr improvements | Single source of truth. Compile-time verified. No scattered magic numbers. |
| Tests linked against full plugin | JUCE-free engine library + test target | Pamplejuce template pattern (2023+) | Test builds in <1 second instead of 10+ seconds. |

**Deprecated/outdated:**
- **Catch2 v2:** End-of-life. The single-header `catch.hpp` no longer receives updates. v3 is a different library structure (compiled, not header-only).
- **JUCE's built-in `UnitTest` class:** Primitive. No test discovery, no generators, no assertion macros beyond `expect()`. Use Catch2.
- **Music theory libraries on GitHub (MusicTheoryCpp, mt, Music-Algebra):** All are hobby projects with minimal maintenance (0-13 stars). Not worth the dependency for our narrow requirements.

## Open Questions

1. **Half-diminished notation: "m7b5" vs "ø7"**
   - What we know: The Unicode half-diminished symbol (ø) is standard in jazz notation. ASCII "m7b5" is universally readable.
   - What's unclear: Whether the plugin UI will support Unicode rendering in pad labels (JUCE's font rendering should handle it).
   - Recommendation: Use "m7b5" for Phase 2. If the UI designer prefers "ø7" in Phase 3, the suffix table is a one-line change.

2. **Enharmonic context for chord TONES (not roots)**
   - What we know: Phase 2 names chords by root + suffix ("Dm7"). Individual chord tones (D, F, A, C) are not displayed.
   - What's unclear: Phase 4's morph engine may need correctly-spelled chord tones for voice-leading comparison. Should PitchClass store diatonic interval information now?
   - Recommendation: No. Phase 2 stores root + type. Chord tones can be derived when needed. If Phase 4 requires spelled-out chord tones, add a `Chord::pitchClasses()` method that uses diatonic interval rules. The `PitchClass` struct already supports arbitrary letter+accidental combinations.

3. **Canonical root set: should both C# and Db be constructible?**
   - What we know: The canonical 12 use C# (not Db) and Eb (not D#). But users may expect Db major on a pad.
   - What's unclear: Whether Phase 3's grid will ever display Db vs C# as different chords.
   - Recommendation: Yes, allow construction from any PitchClass (the struct supports it). The canonical 12 are convenience constants, not restrictions. Phase 3 decides which roots to display.

## Sources

### Primary (HIGH confidence)
- Catch2 official releases: https://github.com/catchorg/Catch2/releases — v3.13.0 confirmed as latest (Feb 15, 2026)
- Catch2 CMake integration docs: https://catch2-temp.readthedocs.io/en/latest/cmake-integration.html — FetchContent pattern, `catch_discover_tests()`
- Open Music Theory (openmusictheory.github.io/triads.html) — authoritative reference for triad/seventh chord interval structures
- Music theory interval tables: standard reference, cross-verified with multiple sources
- MIDI note number standard: C4=60, A4=69, octave formula universally documented
- Existing codebase CMakeLists.txt — verified current build structure, namespace, include paths

### Secondary (MEDIUM confidence)
- "Representing Pitches" (dinoslice.com/posts/representing-pitches/) — Letter+Accidental and Circle-of-Fifths representation patterns. Well-reasoned analysis, cross-verified with music theory principles.
- fshstk/JUCE-Project-Template — CMake pattern for JUCE + Catch2 integration. Small project (1 star) but demonstrates the key pattern correctly.
- Chord notation conventions (musicnotes.com, Wikipedia) — standard notation verified across multiple sources

### Tertiary (LOW confidence)
- None. All findings verified with at least two sources.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Catch2 v3.13.0 version verified from releases page. FetchContent pattern from official Catch2 docs.
- Architecture: HIGH — JUCE-independent engine library is a proven pattern. Constexpr lookup tables are standard C++20.
- Interval data: HIGH — Music theory intervals are mathematically defined and universally agreed upon.
- Naming conventions: HIGH — Standard pop/jazz notation. Confirmed by CHRD-02 examples ("Dm7", "F#aug").
- Pitfalls: HIGH — Based on common C++ and music theory programming mistakes, verified from multiple StackOverflow/forum sources.

**Research date:** 2026-02-19
**Valid until:** ~2026-06-19 (Catch2 may release newer versions; music theory won't change)
