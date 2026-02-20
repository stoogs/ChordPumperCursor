#include "engine/MorphEngine.h"
#include "engine/PitchClassSet.h"
#include "engine/ScaleDatabase.h"
#include "engine/VoiceLeader.h"
#include "engine/RomanNumeral.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace chordpumper {

namespace {

constexpr std::array<float, 7> kModeScores = {
    1.00f, // Ionian
    0.75f, // Dorian
    0.60f, // Phrygian
    0.70f, // Lydian
    0.80f, // Mixolydian
    0.85f, // Aeolian
    0.60f, // Locrian
};

constexpr int kCategoryCount = 3;

int qualityCategoryIndex(ChordType type) {
    switch (type) {
        case ChordType::Major:
        case ChordType::Maj7:
        case ChordType::Dom7:
            return 0;
        case ChordType::Minor:
        case ChordType::Min7:
            return 1;
        default:
            return 2;
    }
}

} // anonymous namespace

float MorphEngine::scoreDiatonic(const PitchClass& referenceRoot,
                                  const Chord& candidate) const {
    int refSemitone = referenceRoot.semitone();
    int candSemitone = candidate.root.semitone();
    float best = 0.0f;

    for (size_t mode = 0; mode < 7; ++mode) {
        const auto& pattern = kModePatterns[mode];
        for (size_t degree = 0; degree < 7; ++degree) {
            int degreeRoot = (refSemitone + pattern.intervals[degree]) % 12;
            if (degreeRoot != candSemitone)
                continue;
            if (candidate.type == pattern.triadQualities[degree] ||
                candidate.type == pattern.seventhQualities[degree]) {
                best = std::max(best, kModeScores[mode]);
            }
        }
    }

    return best;
}

std::array<ScoredChord, 64> MorphEngine::morph(
    const Chord& reference,
    const std::vector<int>& currentVoicing) const {

    std::vector<int> vlBaseline = currentVoicing;
    if (vlBaseline.empty())
        vlBaseline = reference.midiNotes(4);

    PitchClassSet refSet = pitchClassSet(reference);
    int refNotes = noteCount(reference.type);
    int refSemitone = reference.root.semitone();

    double centroid = 0.0;
    for (int note : vlBaseline)
        centroid += note;
    centroid /= static_cast<double>(vlBaseline.size());
    int vlOctave = static_cast<int>(centroid) / 12 - 1;

    struct Candidate {
        ScoredChord sc;
        PitchClassSet pcs;
        int interval;
    };

    std::vector<Candidate> all;
    all.reserve(108);

    for (const auto& chord : kAllChords) {
        float ds = scoreDiatonic(reference.root, chord);

        PitchClassSet cs = pitchClassSet(chord);
        int cn = noteCount(chord.type);
        float ctScore = static_cast<float>(commonToneCount(refSet, cs)) /
                        static_cast<float>(std::max(refNotes, cn));

        // Try ±1 octave to find minimum VL distance (avoids octave-boundary bias)
        int bestDist = std::numeric_limits<int>::max();
        for (int oct = vlOctave - 1; oct <= vlOctave + 1; ++oct) {
            auto notes = chord.midiNotes(oct);
            bestDist = std::min(bestDist, voiceLeadingDistance(vlBaseline, notes));
        }
        float vlScore = std::max(0.0f, 1.0f - static_cast<float>(bestDist) / 24.0f);

        float weightSum = weights.diatonic + weights.commonTones + weights.voiceLeading;
        float composite = weights.diatonic * ds +
                          weights.commonTones * ctScore +
                          weights.voiceLeading * vlScore;
        if (weightSum > 0.0f)
            composite /= weightSum;

        int interval = (chord.root.semitone() - refSemitone + 12) % 12;

        all.push_back({{chord, composite, romanNumeral(reference, chord)},
                       cs,
                       interval});
    }

    // Deduplicate symmetric chords by pitch-class set — keep closest to I
    std::map<PitchClassSet, size_t> seen;
    for (size_t i = 0; i < all.size(); ++i) {
        auto it = seen.find(all[i].pcs);
        if (it == seen.end())
            seen[all[i].pcs] = i;
        else if (all[i].interval < all[it->second].interval)
            seen[all[i].pcs] = i;
    }

    std::vector<ScoredChord> pool;
    pool.reserve(seen.size());
    for (auto& [_, idx] : seen)
        pool.push_back(std::move(all[idx].sc));

    // Deterministic sort: score desc → interval asc → type asc
    auto cmp = [refSemitone](const ScoredChord& a, const ScoredChord& b) {
        if (a.score != b.score)
            return a.score > b.score;
        int ai = (a.chord.root.semitone() - refSemitone + 12) % 12;
        int bi = (b.chord.root.semitone() - refSemitone + 12) % 12;
        if (ai != bi)
            return ai < bi;
        return static_cast<int>(a.chord.type) < static_cast<int>(b.chord.type);
    };

    std::sort(pool.begin(), pool.end(), cmp);

    size_t poolSize = std::min(pool.size(), size_t(72));
    constexpr size_t kFinal = 64;
    size_t selectEnd = std::min(poolSize, kFinal);

    // Variety post-filter: ensure >= 2 from each quality category
    std::array<int, kCategoryCount> catCount{};
    for (size_t i = 0; i < selectEnd; ++i)
        catCount[static_cast<size_t>(qualityCategoryIndex(pool[i].chord.type))]++;

    for (int cat = 0; cat < kCategoryCount; ++cat) {
        while (catCount[static_cast<size_t>(cat)] < 4) {
            int bestRes = -1;
            for (size_t j = selectEnd; j < poolSize; ++j) {
                if (qualityCategoryIndex(pool[j].chord.type) == cat) {
                    bestRes = static_cast<int>(j);
                    break;
                }
            }
            if (bestRes < 0)
                break;

            int maxCat = -1;
            for (int c = 0; c < kCategoryCount; ++c) {
                if (catCount[static_cast<size_t>(c)] > 4 &&
                    (maxCat < 0 ||
                     catCount[static_cast<size_t>(c)] >
                         catCount[static_cast<size_t>(maxCat)]))
                    maxCat = c;
            }
            if (maxCat < 0)
                break;

            int worst = -1;
            for (int j = static_cast<int>(selectEnd) - 1; j >= 0; --j) {
                if (qualityCategoryIndex(pool[static_cast<size_t>(j)].chord.type) ==
                    maxCat) {
                    worst = j;
                    break;
                }
            }
            if (worst < 0)
                break;

            std::swap(pool[static_cast<size_t>(worst)],
                      pool[static_cast<size_t>(bestRes)]);
            catCount[static_cast<size_t>(cat)]++;
            catCount[static_cast<size_t>(maxCat)]--;
        }
    }

    std::sort(pool.begin(),
              pool.begin() + static_cast<ptrdiff_t>(selectEnd), cmp);

    std::array<ScoredChord, 64> result{};
    for (size_t i = 0; i < kFinal && i < selectEnd; ++i)
        result[i] = std::move(pool[i]);

    return result;
}

} // namespace chordpumper
