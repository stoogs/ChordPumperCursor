#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "PersistentState.h"
#include "engine/PitchClass.h"
#include "engine/Chord.h"
#include "engine/ChordType.h"

using namespace chordpumper;
using namespace chordpumper::pitches;
using Catch::Matchers::WithinAbs;

TEST_CASE("Default state round-trips", "[state]")
{
    PersistentState original;
    auto tree = original.toValueTree();
    auto restored = PersistentState::fromValueTree(tree);

    REQUIRE(restored.hasMorphed == false);
    REQUIRE(restored.progression.empty());
    REQUIRE_THAT(restored.weights.diatonic, WithinAbs(0.40, 0.001));
    REQUIRE_THAT(restored.weights.commonTones, WithinAbs(0.25, 0.001));
    REQUIRE_THAT(restored.weights.voiceLeading, WithinAbs(0.25, 0.001));

    for (int i = 0; i < 32; ++i)
    {
        auto idx = static_cast<size_t>(i);
        REQUIRE(restored.gridChords[idx].root == original.gridChords[idx].root);
        REQUIRE(restored.gridChords[idx].type == original.gridChords[idx].type);
    }
}

TEST_CASE("Populated state round-trips", "[state]")
{
    PersistentState original;

    original.gridChords[0] = {C, ChordType::Major};
    original.gridChords[1] = {D, ChordType::Minor};
    original.gridChords[5] = {Fs, ChordType::Augmented};

    original.romanNumerals[0] = "I";
    original.romanNumerals[1] = "ii";
    original.romanNumerals[5] = "IV#";

    original.hasMorphed = true;
    original.lastPlayedChord = {C, ChordType::Major};
    original.lastVoicing = {60, 64, 67};

    original.progression.push_back({C, ChordType::Major});
    original.progression.push_back({F, ChordType::Major});
    original.progression.push_back({G, ChordType::Dom7});

    original.weights = {0.5f, 0.3f, 0.2f};

    auto tree = original.toValueTree();
    auto restored = PersistentState::fromValueTree(tree);

    REQUIRE(restored.gridChords[0].root == C);
    REQUIRE(restored.gridChords[0].type == ChordType::Major);
    REQUIRE(restored.gridChords[1].root == D);
    REQUIRE(restored.gridChords[1].type == ChordType::Minor);
    REQUIRE(restored.gridChords[5].root == Fs);
    REQUIRE(restored.gridChords[5].type == ChordType::Augmented);

    REQUIRE(restored.romanNumerals[0] == "I");
    REQUIRE(restored.romanNumerals[1] == "ii");
    REQUIRE(restored.romanNumerals[5] == "IV#");

    REQUIRE(restored.hasMorphed == true);
    REQUIRE(restored.lastPlayedChord.root == C);
    REQUIRE(restored.lastPlayedChord.type == ChordType::Major);
    REQUIRE(restored.lastVoicing == std::vector<int>{60, 64, 67});

    REQUIRE(restored.progression.size() == 3);
    REQUIRE(restored.progression[0].root == C);
    REQUIRE(restored.progression[0].type == ChordType::Major);
    REQUIRE(restored.progression[1].root == F);
    REQUIRE(restored.progression[1].type == ChordType::Major);
    REQUIRE(restored.progression[2].root == G);
    REQUIRE(restored.progression[2].type == ChordType::Dom7);

    REQUIRE_THAT(restored.weights.diatonic, WithinAbs(0.5, 0.001));
    REQUIRE_THAT(restored.weights.commonTones, WithinAbs(0.3, 0.001));
    REQUIRE_THAT(restored.weights.voiceLeading, WithinAbs(0.2, 0.001));
}

TEST_CASE("ValueTree determinism", "[state]")
{
    PersistentState state;
    state.gridChords[0] = {C, ChordType::Major};
    state.gridChords[7] = {Ab, ChordType::Minor};
    state.hasMorphed = true;
    state.lastPlayedChord = {E, ChordType::Min7};
    state.lastVoicing = {52, 55, 59, 62};
    state.progression.push_back({A, ChordType::Major});
    state.weights = {0.6f, 0.2f, 0.2f};

    auto tree1 = state.toValueTree();
    auto restored = PersistentState::fromValueTree(tree1);
    auto tree2 = restored.toValueTree();

    REQUIRE(tree1.isEquivalentTo(tree2));
}

TEST_CASE("Corrupt data handling", "[state]")
{
    SECTION("Invalid (empty) tree returns default state")
    {
        auto result = PersistentState::fromValueTree(juce::ValueTree());
        REQUIRE(result.hasMorphed == false);
        REQUIRE(result.progression.empty());
        REQUIRE(result.gridChords[0].root == pitches::C);
        REQUIRE(result.gridChords[0].type == ChordType::Major);
    }

    SECTION("Wrong type returns default state")
    {
        auto result = PersistentState::fromValueTree(juce::ValueTree("WrongType"));
        REQUIRE(result.hasMorphed == false);
        REQUIRE(result.progression.empty());
    }

    SECTION("Version 0 returns default state")
    {
        juce::ValueTree tree("ChordPumperState");
        tree.setProperty("version", 0, nullptr);
        auto result = PersistentState::fromValueTree(tree);
        REQUIRE(result.hasMorphed == false);
        REQUIRE(result.progression.empty());
    }
}

TEST_CASE("Missing children handled gracefully", "[state]")
{
    juce::ValueTree tree("ChordPumperState");
    tree.setProperty("version", 1, nullptr);

    auto result = PersistentState::fromValueTree(tree);

    PersistentState defaults;
    REQUIRE(result.hasMorphed == false);
    REQUIRE(result.progression.empty());
    REQUIRE_THAT(result.weights.diatonic, WithinAbs(0.40, 0.001));
    REQUIRE_THAT(result.weights.commonTones, WithinAbs(0.25, 0.001));
    REQUIRE_THAT(result.weights.voiceLeading, WithinAbs(0.25, 0.001));

    for (int i = 0; i < 32; ++i)
    {
        auto idx = static_cast<size_t>(i);
        REQUIRE(result.gridChords[idx].root == defaults.gridChords[idx].root);
        REQUIRE(result.gridChords[idx].type == defaults.gridChords[idx].type);
    }
}

TEST_CASE("Partial state restores available fields", "[state]")
{
    PersistentState original;
    original.gridChords[0] = {Bb, ChordType::Dim7};
    original.romanNumerals[0] = "bVII7";
    original.weights = {0.7f, 0.15f, 0.15f};

    auto fullTree = original.toValueTree();

    juce::ValueTree partial("ChordPumperState");
    partial.setProperty("version", 1, nullptr);
    partial.addChild(fullTree.getChildWithName("Grid").createCopy(), -1, nullptr);
    partial.addChild(fullTree.getChildWithName("Weights").createCopy(), -1, nullptr);

    auto result = PersistentState::fromValueTree(partial);

    REQUIRE(result.gridChords[0].root == Bb);
    REQUIRE(result.gridChords[0].type == ChordType::Dim7);
    REQUIRE(result.romanNumerals[0] == "bVII7");
    REQUIRE_THAT(result.weights.diatonic, WithinAbs(0.7, 0.001));
    REQUIRE_THAT(result.weights.commonTones, WithinAbs(0.15, 0.001));
    REQUIRE_THAT(result.weights.voiceLeading, WithinAbs(0.15, 0.001));

    REQUIRE(result.hasMorphed == false);
    REQUIRE(result.progression.empty());
}
