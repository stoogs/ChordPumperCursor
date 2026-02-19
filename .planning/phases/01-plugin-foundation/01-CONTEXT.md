# Phase 1: Plugin Foundation - Context

**Gathered:** 2026-02-19
**Status:** Ready for planning

<domain>
## Phase Boundary

Build infrastructure for ChordPumper: CMake project with JUCE 8 as a git submodule, clap-juce-extensions for CLAP format, producing VST3 + CLAP + Standalone binaries on Linux x86_64. Plugin loads in Bitwig Studio and standalone displays a window.

</domain>

<decisions>
## Implementation Decisions

### Plugin Identity
- Plugin name: **ChordPumper**
- Manufacturer: User's personal name/alias (use placeholder if not provided, easy to change later)
- DAW category: **Instrument** (not MIDI Effect)
- Unique plugin ID: Generate a standard JUCE plugin code

### JUCE Licensing
- Use **AGPLv3** license (free tier) for development
- JUCE acquired as **git submodule** pinned to release tag 8.0.12
- Open source / proprietary decision deferred — set up build to work with either

### Source Organization
- **Modular layout**: `src/engine/`, `src/ui/`, `src/midi/` subdirectories
- C++ namespace: `chordpumper`
- clap-juce-extensions as git submodule alongside JUCE

### Initial Plugin Window
- Window size: **1000x600** (large — room for 8x4 grid + controls in future phases)
- Visual direction: **Dark theme** (standard for DAW plugins)
- Phase 1 shell content: **Plugin name + version** text centered in window

### Claude's Discretion
- Test directory structure (separate `tests/` or alongside source)
- Header style (.h/.cpp pairs vs header-only for pure logic)
- Specific dark theme colors
- CMake structure details (single vs multi-file)
- .gitignore contents

</decisions>

<specifics>
## Specific Ideas

- Plugin should feel native alongside other Bitwig instruments in the browser
- Dark theme should be professional, not flashy — this is a tool, not a light show
- Window size chosen to accommodate the future 8x4 grid comfortably

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-plugin-foundation*
*Context gathered: 2026-02-19*
