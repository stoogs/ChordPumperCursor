Project Brief: "Chord-Grid" MIDI Plugin
1. Vision & Mashup Concept

    The Scaler Side: Deep harmonic "awareness." The plugin knows what key you are in and suggests chords that are "legal" (diatonic) or "spicy" (borrowed/extended).

    The Reason Side: A 4x4 or 8x4 grid of "Chord Pads." Playing a chord highlights neighbor pads in different colors (Bright Green = Safe, Dark Green = Risky, Red = Out of Key) to suggest the next move.

    The Integration: Native Linux performance. Zero-latency MIDI transformation.

2. Technical Specifications

    Framework: JUCE 8 (C++)

    Formats: VST3, CLAP (using clap-juce-extensions), and Standalone.

    Platform: Linux (optimized for x86_64, CachyOS).

    Target DAW: Bitwig Studio 5+ (must support polyphonic expressions/MPE if possible).

3. User Journey (The "Full Experience")
Step 1: Detection & Setup

    User opens the plugin in Bitwig.

    User plays a MIDI progression into the plugin or selects a Key/Scale from a dropdown.

    The plugin populates a "Bank" of 16 pads with chords related to that scale.

Step 2: Exploration (The Mashup Heart)

    User clicks/triggers Pad 1 (C Major).

    The UI instantly updates: Pads for F Major and G Major glow bright green (strong cadences). The pad for Am glows dim green (relative minor). A pad for Bb Major glows yellow (modal interchange).

    User drags a "Complexity" slider. At 0%, chords are simple triads. At 100%, chords become 9ths, 11ths, and suspended voicings.

Step 3: Sequencing & Performance

    User clicks "Record" on the internal mini-sequencer (Reason-style).

    User triggers pads via MIDI notes (C1 to D#2). Each trigger is snapped to the Bitwig transport clock.

    User drags the resulting "Sequence" directly into a Bitwig MIDI track.

4. Detailed Feature Requirements for Claude Code
Feature Group	Requirement
Logic Engine	Must use a MusicTheory helper class to calculate intervals for Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, and Locrian.
Chord Voicing	Implement "Smart Inversions" so that chord changes minimize note jumping (Voice Leading).
Grid UI	A 4x4 juce::Drawable grid. Each cell must display the Chord Name (e.g., "Dm7") and its Roman Numeral (e.g., "ii7").
MIDI Routing	Support MidiBuffer processing. Input MIDI notes C1-D#2 trigger pads; notes C3 and above pass through for "top-line" jamming.
Linux Build	Use CMake with juce_add_plugin. Include free-audio/clap-juce-extensions as a git submodule for CLAP support.
