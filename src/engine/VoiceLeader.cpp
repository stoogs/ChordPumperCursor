#include "engine/VoiceLeader.h"
#include "engine/ChordType.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace chordpumper {

int voiceLeadingDistance(const std::vector<int>& from, const std::vector<int>& to) {
    if (from.empty() || to.empty())
        return 0;

    int n = static_cast<int>(std::min(from.size(), to.size()));
    int best = std::numeric_limits<int>::max();

    std::vector<int> perm(static_cast<size_t>(n));
    std::iota(perm.begin(), perm.end(), 0);

    do {
        int dist = 0;
        for (int i = 0; i < n; ++i)
            dist += std::abs(from[static_cast<size_t>(i)] - to[static_cast<size_t>(perm[static_cast<size_t>(i)])]);
        best = std::min(best, dist);
    } while (std::next_permutation(perm.begin(), perm.end()));

    // Handle size mismatch: add minimum distance for extra notes
    const auto& longer = (from.size() > to.size()) ? from : to;
    const auto& shorter = (from.size() > to.size()) ? to : from;
    for (size_t i = static_cast<size_t>(n); i < longer.size(); ++i) {
        int minDist = std::numeric_limits<int>::max();
        for (size_t j = 0; j < shorter.size(); ++j)
            minDist = std::min(minDist, std::abs(longer[i] - shorter[j]));
        best += minDist;
    }

    return best;
}

VoicedChord optimalVoicing(const Chord& target, const std::vector<int>& previousNotes,
                           int octave) {
    const auto& intervals = kIntervals[static_cast<int>(target.type)];
    int count = noteCount(target.type);
    int rootSemitone = target.root.semitone();

    if (previousNotes.empty()) {
        int rootMidi = octave * 12 + 12 + rootSemitone;
        std::vector<int> notes;
        notes.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
            notes.push_back(rootMidi + intervals[static_cast<size_t>(i)]);
        return {target, notes};
    }

    // Compute centroid of previous voicing
    double centroid = 0.0;
    for (int note : previousNotes)
        centroid += note;
    centroid /= static_cast<double>(previousNotes.size());

    // Generate candidate voicing: place each pitch class near the centroid
    std::vector<int> voiced;
    voiced.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        int pc = (rootSemitone + intervals[static_cast<size_t>(i)]) % 12;
        // Find the octave placement closest to the centroid
        int baseNote = static_cast<int>(centroid) / 12 * 12 + pc;
        if (baseNote < static_cast<int>(centroid) - 6)
            baseNote += 12;
        else if (baseNote > static_cast<int>(centroid) + 6)
            baseNote -= 12;
        voiced.push_back(baseNote);
    }

    // Try shifting each note ±12 to find the permutation-optimal voicing
    int bestDist = voiceLeadingDistance(previousNotes, voiced);
    std::vector<int> bestVoicing = voiced;

    int n = static_cast<int>(voiced.size());
    int combos = 1;
    for (int i = 0; i < n; ++i)
        combos *= 3;

    for (int mask = 0; mask < combos; ++mask) {
        std::vector<int> candidate = voiced;
        int m = mask;
        for (int i = 0; i < n; ++i) {
            int shift = (m % 3) - 1; // -1, 0, +1
            candidate[static_cast<size_t>(i)] += shift * 12;
            m /= 3;
        }
        int dist = voiceLeadingDistance(previousNotes, candidate);
        if (dist < bestDist) {
            bestDist = dist;
            bestVoicing = candidate;
        }
    }

    return {target, bestVoicing};
}

} // namespace chordpumper
