# Phase 4: Morphing Suggestions - Research

**Researched:** 2026-02-19
**Domain:** Music theory algorithms — chord suggestion scoring, voice leading optimization, Roman numeral analysis
**Confidence:** HIGH

## Summary

Phase 4 is the product's soul: after clicking any pad, all 32 pads morph to show harmonically related chord suggestions with Roman numeral labels and voice-led transitions. The challenge is novel — no reference implementation exists that combines keyless exploration, hybrid scoring (diatonic + modal interchange + proximity), and voice-led output in a single system. However, each individual component is well-understood music theory translated into straightforward computation.

The algorithm decomposes into four discrete, testable subsystems: (1) a **MorphEngine** that scores all 108 possible chords against the last-played chord using a weighted combination of diatonic membership, modal interchange membership, common-tone count, and voice-leading distance; (2) a **VoiceLeader** that finds optimal voicings minimizing total semitone movement; (3) a **Roman numeral calculator** that labels each suggestion relative to the last-played root; and (4) **integration wiring** that connects pad clicks to grid updates. Each subsystem lives in `src/engine/` with zero JUCE dependency, enabling fast Catch2 testing.

The key architectural insight is that all scoring is pitch-class-set math. Representing each chord's pitch classes as a 12-bit bitset enables common-tone counting via `popcount(a & b)`, and the voice leading problem reduces to brute-force permutation search over at most 4! = 24 orderings — trivially fast. Scoring all 108 candidates and sorting takes <1ms on any modern CPU, so the morph runs synchronously on the GUI thread with no latency concern.

**Primary recommendation:** Build four engine classes (MorphEngine, VoiceLeader, RomanNumeral, ChordScorer) in `src/engine/` with no JUCE dependency. Use bitset pitch-class representation for common-tone scoring, brute-force permutation for voice leading (max 24 permutations), and a composite weighted score combining four dimensions. Test exhaustively with Catch2 using golden-progression tests and transposition-invariance properties.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| GRID-03 | After user clicks a pad, grid morphs to show 32 contextually related chord suggestions | MorphEngine scores all 108 chords, selects top 32 by composite score. GridPanel calls morph on padClicked, updates all PadComponent instances atomically. |
| GRID-04 | Suggestions use a hybrid algorithm combining music theory rules (diatonic, modal interchange) and harmonic proximity (common tones, voice-leading distance) | Four-dimension scoring: diatonic membership (7 modes), modal interchange detection, common-tone count via bitset intersection, voice-leading distance via permutation search. Weighted composite produces final rank. |
| GRID-05 | User can start exploring from any chord — no key or scale selection required | Keyless design: every chord IS the temporary tonic. Diatonic/modal scoring treats the clicked chord's root as I and generates all 7 parallel mode chord families from that root. No persistent key state. |
| GRID-06 | Each pad displays a contextual Roman numeral relative to the last-played chord | Semitone interval from last-played root to suggestion root maps to Roman numeral via lookup table. Chord quality determines uppercase/lowercase. Chromatic alterations use ♭/♯ prefixes. |
| CHRD-03 | Chord transitions use smart voice leading that minimizes note movement between consecutive chords | VoiceLeader class finds optimal voicing per chord by minimizing sum of absolute semitone movement across all voices. Brute-force over permutations (3! or 4!) for each candidate. Stores voiced MIDI notes per pad, not just root-position. |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++20 (GCC 15) | C++20 / GCC 15.2 | Language standard | Already configured. Provides constexpr, std::array, bitwise ops, std::sort. |
| ChordPumperEngine | (existing) | Static library for all theory code | Phase 2/3 pattern. New morph engine classes add to this library. Zero JUCE dependency enables fast test compilation. |
| Catch2 | v3.13.0 | Unit testing | Already integrated. GENERATE for exhaustive testing of 108 chord × 108 chord combinations. |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<algorithm>` | C++20 STL | `std::sort`, `std::partial_sort` | Ranking 108 candidates, selecting top 32 |
| `<bitset>` or `uint16_t` | C++20 STL | 12-bit pitch class set representation | Common-tone counting via bitwise AND + popcount |
| `<cmath>` | C++20 STL | `std::abs` | Voice-leading distance calculation |
| `<array>` | C++20 STL | Fixed-size containers | Interval tables, mode scale patterns, Roman numeral lookup |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Brute-force voice leading | Hungarian algorithm (O(n³)) | Hungarian is overkill for n≤4. Brute-force over 24 permutations is simpler, faster, and has zero implementation risk. |
| Weighted composite score | Machine learning model | ML is a black box (explicitly excluded in REQUIREMENTS.md out-of-scope). Deterministic weights are transparent and tunable. |
| Custom scoring engine | Septima C++ library | Septima focuses on seventh-chord networks with GLPK/GSL dependencies. Our needs are broader (triads + 7ths, 9 chord types) and simpler (no LP solving). Custom is ~300 lines, zero deps. |
| Bitset common-tone count | Iterate-and-compare | Bitset intersection + popcount is O(1) vs O(n²). 108 candidates × 12 bits = trivial. |

## Architecture Patterns

### Recommended Project Structure

```
src/engine/
├── PitchClass.h/.cpp          # (existing) Note representation
├── Chord.h/.cpp               # (existing) Chord construction
├── ChordType.h                # (existing) Chord type enum + intervals
├── NoteLetter.h               # (existing) Note letter enum
├── PitchClassSet.h            # NEW: 12-bit bitset for pitch class sets
├── VoiceLeader.h/.cpp         # NEW: Optimal voicing for chord transitions
├── MorphEngine.h/.cpp         # NEW: Score + rank 108 chords, select top 32
├── RomanNumeral.h/.cpp        # NEW: Interval-to-Roman-numeral mapping
└── ScaleDatabase.h            # NEW: Constexpr mode/scale interval patterns

tests/
├── test_pitch_class_set.cpp   # NEW: Bitset operations, common tones
├── test_voice_leader.cpp      # NEW: Voice leading optimization
├── test_morph_engine.cpp      # NEW: Scoring, ranking, top-32 selection
├── test_roman_numeral.cpp     # NEW: Roman numeral generation
└── ... (existing tests)
```

### Pattern 1: Pitch Class Set as 12-Bit Integer

**What:** Represent a chord's pitch classes as a bitmask where bit N = 1 if semitone N is present.
**When to use:** All common-tone calculations, chord similarity comparisons.

```cpp
namespace chordpumper {

using PitchClassSet = uint16_t;

inline constexpr PitchClassSet pitchClassSet(const Chord& chord) {
    PitchClassSet set = 0;
    auto& intervals = kIntervals[static_cast<int>(chord.type)];
    int rootSemitone = chord.root.semitone();
    int count = noteCount(chord.type);
    for (int i = 0; i < count; ++i) {
        int pc = (rootSemitone + intervals[i]) % 12;
        set |= (1u << pc);
    }
    return set;
}

inline constexpr int commonToneCount(PitchClassSet a, PitchClassSet b) {
    return __builtin_popcount(a & b);
}

} // namespace chordpumper
```

### Pattern 2: Brute-Force Voice Leading Distance

**What:** Compute the minimum total semitone movement across all voice assignments between two chords.
**When to use:** Scoring chord transitions for voice-leading smoothness.

For chords of equal size (both triads or both 7ths): try all n! permutations of the target chord's notes and find the assignment with minimum sum of absolute semitone differences. For n=3, that's 6 permutations. For n=4, that's 24.

For chords of different sizes (triad → 7th or vice versa): the extra voice gets a penalty distance (e.g., the interval between the nearest note and the new note). Alternatively, pad the smaller chord with a doubled note for comparison.

```cpp
namespace chordpumper {

int voiceLeadingDistance(const std::vector<int>& from, const std::vector<int>& to);

struct VoicedChord {
    Chord chord;
    std::vector<int> midiNotes;
};

VoicedChord optimalVoicing(const Chord& target, const std::vector<int>& previousNotes,
                           int octave);

} // namespace chordpumper
```

**Implementation detail:** When `from` and `to` have different sizes, use the common size for permutation search and add the minimum distance for the extra note(s).

### Pattern 3: Diatonic and Modal Interchange Scoring via Scale Database

**What:** Pre-define the interval patterns for all 7 modes from any root, then check if a candidate chord's root+quality matches any mode's chord family.
**When to use:** Computing the diatonic and modal-interchange dimensions of the morph score.

The 7 diatonic modes built on root C produce these scales:

| Mode | Scale intervals (semitones from root) | Chord qualities on degrees I–VII |
|------|--------------------------------------|----------------------------------|
| Ionian | 0,2,4,5,7,9,11 | Maj, min, min, Maj, Maj, min, dim |
| Dorian | 0,2,3,5,7,9,10 | min, min, Maj, Maj, min, dim, Maj |
| Phrygian | 0,1,3,5,7,8,10 | min, Maj, Maj, min, dim, Maj, min |
| Lydian | 0,2,4,6,7,9,11 | Maj, Maj, min, dim, Maj, min, min |
| Mixolydian | 0,2,4,5,7,9,10 | Maj, min, dim, Maj, min, min, Maj |
| Aeolian | 0,2,3,5,7,8,10 | min, dim, Maj, min, min, Maj, Maj |
| Locrian | 0,1,3,5,6,8,10 | dim, Maj, min, min, Maj, Maj, min |

The triad quality for each scale degree follows from the intervals: stack two thirds from that degree and the resulting intervals determine major/minor/diminished.

To score a candidate chord against a reference root:
1. For each of the 7 modes built on the reference root, generate the 7 diatonic triads
2. If candidate root+quality matches a diatonic triad in Ionian → highest diatonic score
3. If it matches in another mode → modal interchange score (slightly lower)
4. If no match → zero diatonic/modal score

Seventh chords can be scored similarly by extending the stacking to include the 7th degree above each scale tone.

```cpp
namespace chordpumper {

struct ScalePattern {
    std::array<int, 7> intervals;
    std::array<ChordType, 7> triadQualities;
    std::array<ChordType, 7> seventhQualities;
};

inline constexpr std::array<ScalePattern, 7> kModePatterns = {{
    // Ionian
    {{0,2,4,5,7,9,11},
     {ChordType::Major, ChordType::Minor, ChordType::Minor, ChordType::Major,
      ChordType::Major, ChordType::Minor, ChordType::Diminished},
     {ChordType::Maj7, ChordType::Min7, ChordType::Min7, ChordType::Maj7,
      ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7}},
    // Dorian
    {{0,2,3,5,7,9,10},
     {ChordType::Minor, ChordType::Minor, ChordType::Major, ChordType::Major,
      ChordType::Minor, ChordType::Diminished, ChordType::Major},
     {ChordType::Min7, ChordType::Min7, ChordType::Maj7, ChordType::Dom7,
      ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7}},
    // Phrygian
    {{0,1,3,5,7,8,10},
     {ChordType::Minor, ChordType::Major, ChordType::Major, ChordType::Minor,
      ChordType::Diminished, ChordType::Major, ChordType::Minor},
     {ChordType::Min7, ChordType::Maj7, ChordType::Dom7, ChordType::Min7,
      ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7}},
    // Lydian
    {{0,2,4,6,7,9,11},
     {ChordType::Major, ChordType::Major, ChordType::Minor, ChordType::Diminished,
      ChordType::Major, ChordType::Minor, ChordType::Minor},
     {ChordType::Maj7, ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7,
      ChordType::Maj7, ChordType::Min7, ChordType::Min7}},
    // Mixolydian
    {{0,2,4,5,7,9,10},
     {ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Major,
      ChordType::Minor, ChordType::Minor, ChordType::Major},
     {ChordType::Dom7, ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7,
      ChordType::Min7, ChordType::Min7, ChordType::Maj7}},
    // Aeolian
    {{0,2,3,5,7,8,10},
     {ChordType::Minor, ChordType::Diminished, ChordType::Major, ChordType::Minor,
      ChordType::Minor, ChordType::Major, ChordType::Major},
     {ChordType::Min7, ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7,
      ChordType::Min7, ChordType::Maj7, ChordType::Dom7}},
    // Locrian
    {{0,1,3,5,6,8,10},
     {ChordType::Diminished, ChordType::Major, ChordType::Minor, ChordType::Minor,
      ChordType::Major, ChordType::Major, ChordType::Minor},
     {ChordType::HalfDim7, ChordType::Maj7, ChordType::Min7, ChordType::Min7,
      ChordType::Maj7, ChordType::Dom7, ChordType::Min7}},
}};

} // namespace chordpumper
```

### Pattern 4: Roman Numeral from Semitone Interval

**What:** Map the semitone interval between two chord roots to a Roman numeral string, using chord quality for case.
**When to use:** Labeling pads after morph (GRID-06).

The mapping treats the last-played chord's root as I. For any suggestion chord:
1. Compute `interval = (suggestion.root.semitone() - reference.root.semitone() + 12) % 12`
2. Map interval to scale degree and accidental
3. Choose uppercase (major/augmented/dominant) or lowercase (minor/diminished)

```cpp
namespace chordpumper {

struct RomanNumeralInfo {
    int interval;
    const char* upperCase;
    const char* lowerCase;
};

inline constexpr std::array<RomanNumeralInfo, 12> kRomanNumerals = {{
    { 0, "I",    "i"},
    { 1, "♭II",  "♭ii"},
    { 2, "II",   "ii"},
    { 3, "♭III", "♭iii"},
    { 4, "III",  "iii"},
    { 5, "IV",   "iv"},
    { 6, "♯IV",  "♯iv"},     // or ♭V/♭v — tritone ambiguity
    { 7, "V",    "v"},
    { 8, "♭VI",  "♭vi"},
    { 9, "VI",   "vi"},
    {10, "♭VII", "♭vii"},
    {11, "VII",  "vii"},
}};

inline bool isUpperCase(ChordType type) {
    switch (type) {
        case ChordType::Major:
        case ChordType::Augmented:
        case ChordType::Maj7:
        case ChordType::Dom7:
            return true;
        default:
            return false;
    }
}

std::string romanNumeral(const Chord& reference, const Chord& suggestion);

} // namespace chordpumper
```

**Design decision — tritone ambiguity (semitone 6):** Use ♯IV for augmented-fourth relationships (e.g., C→F♯) and ♭V for diminished-fifth relationships. Since ChordPumper has no key context, default to ♯IV for major/augmented qualities and ♭V for minor/diminished qualities. This heuristic works for the vast majority of musical contexts.

### Pattern 5: Composite Scoring for Morph Ranking

**What:** Combine four scoring dimensions into a single float for ranking.
**When to use:** MorphEngine selecting top 32 from 108 candidates.

```cpp
namespace chordpumper {

struct MorphScore {
    float diatonic;        // 0.0–1.0: diatonic membership strength
    float commonTones;     // 0.0–1.0: normalized common tone count
    float voiceLeading;    // 0.0–1.0: normalized voice leading distance (inverted — closer = higher)
    float composite;       // weighted sum
};

struct MorphWeights {
    float diatonic     = 0.40f;
    float commonTones  = 0.25f;
    float voiceLeading = 0.25f;
    float variety       = 0.10f;
};

} // namespace chordpumper
```

**Scoring dimensions explained:**

1. **Diatonic score (0.0–1.0):** 1.0 if the chord appears in the Ionian mode, 0.7–0.8 if it appears in another diatonic mode (modal interchange), 0.0 if chromatic only. Ionian is weighted highest because it represents the strongest functional relationship.

2. **Common tone score (0.0–1.0):** `commonToneCount / max(noteCount(from), noteCount(to))`. More shared pitch classes = smoother harmonic relationship.

3. **Voice leading score (0.0–1.0):** `1.0 - (voiceLeadingDistance / maxPossibleDistance)`. Lower movement = higher score. Max possible distance is bounded (e.g., a tritone in every voice ≈ 6 semitones × 4 voices = 24).

4. **Variety bonus (0.0–1.0):** Ensures quality diversity in the 32 suggestions. Penalize if too many of one chord type already selected. Ensures the grid has a mix of major, minor, 7th, etc.

### Anti-Patterns to Avoid

- **Don't require key selection:** The entire product value is keyless exploration. Treat every played chord as a temporary I. Never store a persistent "current key" variable.
- **Don't use Dijkstra/graph search for a single transition:** Graph-based voice leading (Dijkstra over voicing nodes) is for multi-chord progressions. For a single source→target transition, brute-force permutation is simpler, faster, and correct.
- **Don't score on the audio thread:** Morph scoring runs on the GUI thread in response to mouseDown. The audio thread only sees the resulting MIDI notes via MidiKeyboardState. Never do allocation or sorting on the audio thread.
- **Don't lose chord identity during voicing:** The VoiceLeader produces voiced MIDI notes, but the Chord struct (root + type) must be preserved for display (name, Roman numeral) and for the next morph cycle.
- **Don't hard-code magic numbers in scoring weights:** Use a named struct (MorphWeights) so weights can be tuned iteratively. The STATE.md warns "Phase 4 will need iterative tuning."
- **Don't treat augmented/diminished as second-class:** The chromatic palette already includes them. They should appear as suggestions when harmonically appropriate (e.g., diminished vii° is diatonic).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Common-tone counting | Nested loops comparing note arrays | Bitset intersection + popcount | O(1) vs O(n²), zero allocation, constexpr-friendly |
| Scale degree chord qualities | Manual if/else chains per mode | Constexpr lookup table (kModePatterns) | 7 modes × 7 degrees = 49 entries. Single source of truth, auditable. |
| Roman numeral strings | String building with conditional logic | Lookup table indexed by semitone interval | 12 entries cover all cases. No string parsing, no edge cases. |
| Voice leading for n≤4 | Hungarian algorithm, LP solver | Brute-force permutation | 24 iterations max. Simpler, no dependencies, provably correct. |
| All 108 candidate chords | Generate on each morph call | Pre-built constexpr array | `std::array<Chord, 108>` built once at compile time. Zero runtime allocation. |

**Key insight:** This entire phase is lookup tables + arithmetic + sorting. No complex data structures, no graph algorithms, no external dependencies. The novelty is in combining dimensions and tuning weights, not in algorithmic complexity.

## Common Pitfalls

### Pitfall 1: Transpose-Dependent Scoring Bugs
**What goes wrong:** Morphing from C major gives different suggestion rankings than morphing from D major (shifted by 2 semitones), when they should be structurally identical.
**Why it happens:** Hard-coding specific pitch classes instead of computing intervals from the reference root. Using absolute semitone values instead of relative intervals.
**How to avoid:** All scoring computations must use `(candidate.semitone() - reference.semitone() + 12) % 12` — relative intervals, never absolute pitches. Test for transposition invariance.
**Warning signs:** Tests pass for C major but fail for F♯ major as reference.

### Pitfall 2: Voice Leading Distance Ignoring Octave Choices
**What goes wrong:** Voice leading says C major → F major has distance 5 (C→F) + 2 (E→A) + 2 (G→C) = 9, when the optimal voicing (C→C, E→F, G→A) has distance 0+1+2 = 3.
**Why it happens:** Comparing root-position chords instead of finding the optimal inversion/voicing of the target.
**How to avoid:** The VoiceLeader must try all inversions/octave placements of the target chord within a reasonable range (e.g., ±6 semitones of each source note), then find the permutation minimizing total movement.
**Warning signs:** All voice-leading distances are suspiciously large (>6 semitones average per voice).

### Pitfall 3: Symmetric Chord Deduplication
**What goes wrong:** Three distinct augmented chords (C aug, E aug, G♯ aug) all have pitch class set {0, 4, 8} and score identically. Similarly, dim7 chords repeat every minor third. The grid fills with redundant entries.
**Why it happens:** Scoring only looks at pitch class content, not root identity.
**How to avoid:** When multiple chords have identical pitch class sets, keep only the one whose root creates the most conventional Roman numeral label. Or: augmented chords always score once regardless of enharmonic root, with the root chosen to produce the simplest numeral.
**Warning signs:** Grid shows "C aug", "E aug", and "G♯ aug" simultaneously when they sound identical.

### Pitfall 4: Modal Interchange Scores Swamping Diatonic
**What goes wrong:** Modal interchange chords (from Dorian, Phrygian, etc.) outnumber pure Ionian chords 6:1, pushing diatonic suggestions off the grid.
**Why it happens:** Each of the 7 modes produces up to 7 chords = 49 total modal candidates. Many are duplicates, but without careful weighting, borrowed chords dominate.
**How to avoid:** Score Ionian matches highest (1.0), Aeolian/Mixolydian next (0.8), other modes lower (0.5–0.7). Deduplicate: if a chord appears in multiple modes, use the highest-scoring mode only.
**Warning signs:** Grid shows mostly obscure chords like ♭II, ♭VII instead of expected I, IV, V, vi.

### Pitfall 5: PadComponent Display Flicker During Morph
**What goes wrong:** Clicking a pad causes visible flicker as all 32 pads update sequentially.
**Why it happens:** Each `setChord()` call triggers `repaint()` individually, causing 32 paint calls in sequence.
**How to avoid:** Batch the morph update: compute all 32 new chords, then update all pads, then call `repaint()` once on the GridPanel. Or use `setBufferedToImage(true)` during transition.
**Warning signs:** Visible sequential redrawing of pads on each click.

### Pitfall 6: Triad-to-Seventh Voice Leading Size Mismatch
**What goes wrong:** Voice leading from a triad (3 notes) to a 7th chord (4 notes) produces incorrect distances or crashes.
**Why it happens:** Permutation search assumes equal-sized chords. The extra voice has no source note to measure distance from.
**How to avoid:** When sizes differ, pad the smaller chord: duplicate the note nearest to the extra target note. Or: compute voice leading on the common subset (3 voices) and add a fixed penalty for the extra note. The penalty should be small — an added 7th is a natural extension.
**Warning signs:** Distance function returns nonsensical values or segfaults when chord types change from triad to 7th.

### Pitfall 7: Roman Numeral Encoding Issues
**What goes wrong:** ♭ and ♯ symbols display as garbage in the JUCE UI.
**Why it happens:** Using Unicode characters (U+266D, U+266F) without verifying the font supports them.
**How to avoid:** Test both approaches: (1) Unicode ♭/♯ if the LookAndFeel font supports it, or (2) ASCII 'b'/'#' as fallback. JUCE's default font typically supports common Unicode music symbols. Test early in development.
**Warning signs:** Roman numerals show as □ or ? on pads.

## Code Examples

### Pre-Built Candidate Chord Array

```cpp
namespace chordpumper {

inline constexpr std::array<Chord, 108> allChords() {
    std::array<Chord, 108> result{};
    constexpr std::array<PitchClass, 12> roots = {
        pitches::C, pitches::Cs, pitches::D, pitches::Eb,
        pitches::E, pitches::F, pitches::Fs, pitches::G,
        pitches::Ab, pitches::A, pitches::Bb, pitches::B
    };
    constexpr std::array<ChordType, 9> types = {
        ChordType::Major, ChordType::Minor, ChordType::Diminished, ChordType::Augmented,
        ChordType::Maj7, ChordType::Min7, ChordType::Dom7, ChordType::Dim7, ChordType::HalfDim7
    };
    int idx = 0;
    for (auto root : roots)
        for (auto type : types)
            result[idx++] = Chord{root, type};
    return result;
}

inline constexpr auto kAllChords = allChords();

} // namespace chordpumper
```

### Voice Leading Distance Calculation

```cpp
int voiceLeadingDistance(const std::vector<int>& from, const std::vector<int>& to) {
    int n = static_cast<int>(std::min(from.size(), to.size()));
    int best = std::numeric_limits<int>::max();

    // Generate all permutations of target indices
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);

    do {
        int dist = 0;
        for (int i = 0; i < n; ++i)
            dist += std::abs(from[i] - to[perm[i]]);
        best = std::min(best, dist);
    } while (std::next_permutation(perm.begin(), perm.end()));

    return best;
}
```

### MorphEngine Scoring Loop

```cpp
struct ScoredChord {
    Chord chord;
    float score;
    std::string romanNumeral;
};

std::array<ScoredChord, 32> MorphEngine::morph(const Chord& reference,
                                                const std::vector<int>& currentVoicing) {
    std::array<ScoredChord, 108> candidates;
    PitchClassSet refSet = pitchClassSet(reference);
    int refNoteCount = reference.noteCount();

    for (int i = 0; i < 108; ++i) {
        const auto& cand = kAllChords[i];
        PitchClassSet candSet = pitchClassSet(cand);

        float diatonic = scoreDiatonic(reference.root, cand);
        float common = static_cast<float>(commonToneCount(refSet, candSet))
                       / static_cast<float>(std::max(refNoteCount, cand.noteCount()));
        float vl = scoreVoiceLeading(currentVoicing, cand);

        candidates[i] = {
            cand,
            weights.diatonic * diatonic + weights.commonTones * common
                + weights.voiceLeading * vl,
            romanNumeral(reference, cand)
        };
    }

    std::partial_sort(candidates.begin(), candidates.begin() + 32, candidates.end(),
                      [](const auto& a, const auto& b) { return a.score > b.score; });

    std::array<ScoredChord, 32> result;
    std::copy_n(candidates.begin(), 32, result.begin());
    return result;
}
```

### Diatonic Membership Scoring

```cpp
float MorphEngine::scoreDiatonic(const PitchClass& referenceRoot, const Chord& candidate) {
    int refSemitone = referenceRoot.semitone();

    for (int modeIdx = 0; modeIdx < 7; ++modeIdx) {
        const auto& mode = kModePatterns[modeIdx];
        for (int degree = 0; degree < 7; ++degree) {
            int degreeSemitone = (refSemitone + mode.intervals[degree]) % 12;
            if (degreeSemitone != candidate.root.semitone()) continue;

            bool triadMatch = (candidate.noteCount() == 3 && candidate.type == mode.triadQualities[degree]);
            bool seventhMatch = (candidate.noteCount() == 4 && candidate.type == mode.seventhQualities[degree]);

            if (triadMatch || seventhMatch) {
                if (modeIdx == 0) return 1.0f;     // Ionian — strongest
                if (modeIdx == 5) return 0.85f;     // Aeolian — parallel minor
                if (modeIdx == 4) return 0.80f;     // Mixolydian — common in pop
                if (modeIdx == 1) return 0.75f;     // Dorian — common in jazz/pop
                if (modeIdx == 3) return 0.70f;     // Lydian
                return 0.60f;                        // Phrygian, Locrian
            }
        }
    }
    return 0.0f; // Not diatonic in any mode
}
```

### Roman Numeral Generation

```cpp
std::string romanNumeral(const Chord& reference, const Chord& suggestion) {
    int interval = (suggestion.root.semitone() - reference.root.semitone() + 12) % 12;
    bool upper = isUpperCase(suggestion.type);
    return upper ? kRomanNumerals[interval].upperCase
                 : kRomanNumerals[interval].lowerCase;
}
```

### Integration: Morph on Pad Click

```cpp
// In GridPanel
void GridPanel::padClicked(const Chord& chord) {
    releaseCurrentChord();

    // Voice-lead the new chord from current voicing
    auto voiced = voiceLeader.optimalVoicing(chord, activeNotes, defaultOctave);

    // Send MIDI
    for (auto note : voiced.midiNotes)
        keyboardState.noteOn(midiChannel, note, velocity);
    activeNotes.assign(voiced.midiNotes.begin(), voiced.midiNotes.end());
    startTimer(noteDurationMs);

    // Morph grid
    auto suggestions = morphEngine.morph(chord, voiced.midiNotes);
    for (int i = 0; i < 32; ++i) {
        pads[i]->setChord(suggestions[i].chord);
        pads[i]->setRomanNumeral(suggestions[i].romanNumeral);
    }
    repaint();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Fixed-key chord suggestion (Scaler 2, Captain Chords) | Keyless exploration from any chord | ChordPumper design | Eliminates key-selection friction. User starts from any chord. |
| Graph/Dijkstra voice leading for progressions | Brute-force permutation for single transitions | Always valid for n≤4 | Simpler code, no external deps, provably optimal for the problem size. |
| String-based interval computation | Bitset pitch-class operations | Pitch-class set theory standard | O(1) common-tone counting, no allocation, constexpr-friendly. |
| Neo-Riemannian PLR-only navigation | Hybrid scoring (diatonic + proximity + voice leading) | Novel to ChordPumper | PLR is limited to major/minor triads. Hybrid works for all 9 chord types including 7ths. |

**Deprecated/outdated:**
- **PLR-only navigation:** Neo-Riemannian PLR transforms are elegant for triads but don't extend naturally to 7th chords. The hybrid scoring approach subsumes PLR relationships (parallel = P transform, relative = R transform) while also covering non-PLR transitions.
- **Key-required suggestion engines:** Both Scaler 2 and Captain Chords require users to select a key first. ChordPumper's keyless design is a conscious product differentiator, not a limitation.

## Open Questions

1. **Weight tuning methodology**
   - What we know: Initial weights (diatonic 0.40, common tones 0.25, voice leading 0.25, variety 0.10) are educated guesses based on music theory importance.
   - What's unclear: How these weights feel in practice. Will the grid feel "too diatonic" or "too chromatic"?
   - Recommendation: Ship with these defaults. Expose weights as named constants (MorphWeights struct) for easy iteration. Future GRID-07 (v2) adds user-adjustable weights. For now, the developer tunes by editing constants and listening.

2. **Variety dimension implementation**
   - What we know: Without a variety score, the grid might show 10 minor chords in different keys and few other qualities.
   - What's unclear: Whether variety should be a score modifier (bonus for underrepresented types) or a post-selection filter (ensure at least N of each quality category).
   - Recommendation: Implement as a post-selection adjustment in the first iteration: after scoring, ensure the top 32 includes at least 2 chords from each major quality category (major, minor, diminished/augmented, 7ths). If a category is underrepresented, swap the lowest-scoring chord of the over-represented category with the highest-scoring chord from the under-represented one.

3. **First morph from chromatic palette**
   - What we know: The grid starts with the chromatic palette (Phase 3). On first click, it morphs.
   - What's unclear: The first click has no "previous voicing" for voice leading. What MIDI notes to use as the reference?
   - Recommendation: Use root-position voicing of the clicked chord as the reference. The first morph's voice-leading dimension scores all candidates against root-position. After the first click, subsequent morphs use the actual voiced notes.

4. **Augmented chord Roman numeral ambiguity**
   - What we know: C augmented = E augmented = G♯ augmented (same pitch class set). If C major is reference, E augmented at interval 4 would be "III+". But it's enharmonically the same as A♭ augmented at interval 8 ("♭VI+").
   - What's unclear: Which root spelling to prefer for augmented chords in Roman numeral context.
   - Recommendation: Use the root as stored in the Chord struct. The canonical spelling from `pitches::` namespace determines the display. Don't attempt re-rooting based on context — it's a deep rabbit hole with diminishing returns.

5. **Performance at scale (108 candidates)**
   - What we know: 108 candidates × 4 scoring dimensions = ~432 simple computations. Voice leading is the heaviest (24 permutations per candidate = 2592 total iterations), but each is just 4 subtractions + 4 abs + 1 comparison.
   - What's unclear: Whether this is perceptible on low-end hardware.
   - Recommendation: Profile, but expect <1ms. If needed, pre-compute voice-leading distances from each chord to all 108 others (108² = 11,664 pairs, ~50KB). This table is computed once at startup and reused.

## Sources

### Primary (HIGH confidence)
- Tymoczko, Dmitri. "The Geometry of Musical Chords" (2006), Princeton — voice-leading distance as aggregate semitone movement in orbifold spaces
- Open Music Theory (viva.pressbooks.pub/openmusictheory) — diatonic modes, chord-scale relationships, Roman numeral conventions
- music21 documentation (music21.org) — RomanNumeral implementation patterns, key-relative chord labeling conventions
- Existing codebase (src/engine/) — verified Chord, PitchClass, ChordType APIs, interval tables, naming conventions

### Secondary (MEDIUM confidence)
- willdickerson/optimal-voice-leading (GitHub) — Dijkstra-based voice leading implementation in Python, confirms graph+permutation approach
- Septima C++ library (github.com/marohnicluka/septima) — seventh-chord voice leading with LP optimization, confirms L1 distance metric and common-tone-fixing heuristic
- Neo-Riemannian PLR theory (Cohn 1997, Offtonic Theory) — P, L, R transforms preserve 2 common tones with single-semitone/whole-tone movement
- LearnJazzStandards.com, PianoWithJonny.com, ComposerDeck.com — modal interchange chord borrowing patterns and practical usage
- Scaler 2 forum/changelog — "Live Suggest" feature (v2.9.0) suggests chords compatible with current progression; key/scale detection is core approach

### Tertiary (LOW confidence)
- Weight values (diatonic 0.40, common 0.25, VL 0.25, variety 0.10) — educated guess, needs iterative tuning in practice. No authoritative source for "correct" weights.
- Variety scoring approach — novel design decision, no reference implementation found. Needs user testing.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — builds entirely on existing ChordPumperEngine pattern with no new dependencies
- Music theory (modes, intervals, Roman numerals): HIGH — well-established theory with multiple authoritative sources
- Scoring algorithm design: MEDIUM — individual components are verified, but the composite weighting is novel and needs tuning
- Voice leading: HIGH — brute-force permutation is mathematically optimal for n≤4, confirmed by multiple implementations
- Integration pattern: HIGH — builds on existing padClicked→MIDI flow from Phase 3
- Edge cases (symmetric chords, enharmonics): MEDIUM — identified and documented, but specific handling details need validation during implementation

**Research date:** 2026-02-19
**Valid until:** ~2026-08-19 (music theory doesn't change; C++ patterns are stable; weight tuning is an ongoing concern)
