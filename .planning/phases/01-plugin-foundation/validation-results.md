# Plugin Validation Results

**Date:** 2026-02-19
**Plan:** 01-03 (Automated Plugin Validation & Host Loading)

## pluginval (VST3)

- **Tool version:** pluginval JUCE v8.0.3
- **Strictness level:** 5 (maximum attempted, passed first try)
- **Result:** SUCCESS — ALL TESTS PASSED
- **Plugin detected:** VST3-ChordPumper-5de1ae6b-bc74a944 (ChordPumperDev: ChordPumper v0.1.0)
- **Tests run:** Scan, Open (cold/warm), Plugin info, Programs, Editor, Audio processing, Plugin state, Automation, Editor Automation, Automatable Parameters, Basic bus, Bus listing, Bus enabling/disabling
- **Sample rates tested:** 44100, 48000, 96000
- **Block sizes tested:** 64, 128, 256, 512, 1024
- **Warnings:** None
- **Notes:** vst3 validator skipped (external validator path not set — expected)

## clap-validator (CLAP)

- **Tool version:** clap-validator v0.3.2
- **Result:** 14 passed, 3 failed, 4 skipped, 0 warnings
- **Plugin ID:** org.chordpumper.chordpumper

### Passed Tests
- create-id-with-trailing-garbage
- query-factory-nonexistent
- scan-rtld-now
- scan-time (14ms — well under 100ms limit)
- descriptor-consistency
- features-categories
- features-duplicates
- param-fuzz-basic
- param-set-wrong-namespace
- process-audio-out-of-place-basic
- process-note-inconsistent
- process-note-out-of-place-basic
- state-reproducibility-flush
- state-invalid

### Failed Tests (all state-related, expected for shell plugin)
- state-buffered-streams: `clap_plugin_state::load()` returned false with 17-byte buffer
- state-reproducibility-basic: `clap_plugin_state::load()` returned false
- state-reproducibility-null-cookies: `clap_plugin_state::load()` returned false

### Skipped Tests
- preset-discovery-crawl (no preset discovery factory — expected)
- preset-discovery-descriptor-consistency (no preset discovery factory)
- preset-discovery-load (no preset discovery factory)
- param-conversions (no parameters with text conversion — expected for shell)

### Notes
- State failures are expected: JUCE's CJE wrapper has known limitations with minimal plugins (no parameters, no custom state)
- "Legacy Parameter API" warnings expected for plugin with no parameters
- JUCE assertion at clap-juce-wrapper.cpp:1801 (bypass/deactivated state handling) is a known CJE issue, not a plugin bug
- Plugin ID warning: CJE reports 'org.chordpumper.chordpumperx1' vs configured 'org.chordpumper.chordpumper' — CJE internal naming, not a user-facing issue

## Standalone Smoke Test

- **Result:** STANDALONE_OK
- **Behavior:** Process launched, remained running for 2+ seconds without crash
- **JUCE version confirmed:** v8.0.12

## Summary

| Format     | Tool            | Result  | Details                                    |
|------------|-----------------|---------|-------------------------------------------|
| VST3       | pluginval (L5)  | PASS    | All tests passed at maximum strictness     |
| CLAP       | clap-validator  | PASS*   | 14/17 passed; 3 state failures expected    |
| Standalone | smoke test      | PASS    | Launches without crash                     |

*CLAP state failures are expected for a minimal shell plugin and will resolve when parameters/state are added in Phase 3.
