# Chord-Grid

## What This Is

A MIDI chord exploration plugin for Linux (VST3/CLAP) built with JUCE 8 and C++. It presents an 8x4 grid of chord pads that morph after each trigger, suggesting harmonically related next chords through a blend of music theory and harmonic proximity. Users explore progressions by feel — clicking pads, hearing them through their DAW instrument, and dragging individual chords into Bitwig as MIDI clips.

## Core Value

The morphing grid — play a chord, see what comes next, follow the thread. The plugin is a discovery engine for chord progressions, not a theory textbook.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] 8x4 grid of 32 chord pads, starting with a chromatic palette (all 12 major, all 12 minor, plus diminished/augmented/sus fills)
- [ ] Click or MIDI-trigger any pad to send that chord as MIDI output to downstream instruments
- [ ] After triggering a pad, grid morphs to show harmonically related next-chord suggestions (hybrid of theory rules + harmonic proximity)
- [ ] Each pad displays chord name (e.g. "Dm7") and contextual Roman numeral relative to the last-played chord
- [ ] Smart voice leading — chord transitions minimize note jumping between voicings
- [ ] Triads and 7th chords for v1 voicing palette
- [ ] Drag any individual chord from a pad to the DAW as a MIDI clip
- [ ] All incoming MIDI notes trigger pads (no split point / passthrough in v1)
- [ ] MIDI output only — no built-in sounds
- [ ] No key/scale selection required — start from any chord, explore freely
- [ ] Linux-native build targeting x86_64 (CachyOS)
- [ ] VST3 and CLAP plugin formats (using clap-juce-extensions)
- [ ] Standalone application format

### Out of Scope

- Color-coded safety indicators (green/red/yellow) — exploration by feel, not visual gatekeeping
- Key/scale detection from incoming MIDI — deferred, adds complexity without core value
- Complexity slider for extended voicings (9ths, 11ths, sus) — v2, start with triads + 7ths
- Mini-sequencer / chord sequence recording — v2, single-chord drag is enough for v1
- Sequence drag-to-DAW (dragging a full progression) — v2, depends on sequencer
- MIDI split point / note passthrough for "top-line jamming" — v2
- MPE / polyphonic expression support — v2+
- Built-in preview sounds — rely on DAW instruments
- Windows / macOS builds — Linux-first

## Context

The concept mashes up two proven products: Scaler's deep harmonic awareness and Reason's tactile pad grid. But Chord-Grid deliberately strips away the prescriptive theory UI (no "this chord is wrong" signaling) in favor of pure exploration. The user builds progressions by ear, guided by an intelligent suggestion engine that surfaces both expected and unexpected harmonic neighbors.

The target user opens the plugin, clicks a pad, hears it through their Bitwig instrument chain, sees the grid transform to show what comes next, and follows threads until they've found a 4-8 chord progression they love. Each chord gets dragged into a Bitwig MIDI track individually. Session over.

Roman numerals are contextual — they update dynamically relative to the last-played chord, giving harmonic context without requiring a fixed key. This teaches theory passively while keeping the focus on sound.

## Constraints

- **Tech Stack**: JUCE 8 (C++), CMake build system with juce_add_plugin
- **Platform**: Linux x86_64, optimized for CachyOS
- **Plugin Formats**: VST3 + CLAP (via clap-juce-extensions git submodule) + Standalone
- **Target DAW**: Bitwig Studio 5+ (primary testing target)
- **Dependencies**: clap-juce-extensions as git submodule for CLAP support

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| No fixed key/scale required | Removes friction, encourages free exploration | — Pending |
| Hybrid morph logic (theory + proximity) | Pure theory is too rigid, pure proximity loses musical coherence | — Pending |
| Contextual Roman numerals (relative to last played) | Provides harmonic context without requiring key selection | — Pending |
| MIDI output only, no built-in sounds | Leverages DAW instrument ecosystem, reduces plugin complexity | — Pending |
| 8x4 grid (32 pads) | Enough room for chromatic palette + morphed suggestions | — Pending |
| All MIDI triggers pads (no passthrough) | Simplifies v1, split can be added later | — Pending |
| Defer sequencer to v2 | Single-chord drag covers the core "capture" workflow | — Pending |

---
*Last updated: 2026-02-19 after initialization*
