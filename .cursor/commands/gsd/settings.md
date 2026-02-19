---
tools:
  read: true
  write: true
  bash: true
  ask_question: true
name: gsd-settings
description: Configure GSD workflow toggles and model profile

<objective>
Interactive configuration of GSD workflow agents and model profile via multi-question prompt.

Routes to the settings workflow which handles:
</objective>

<execution_context>
@~/.cursor/get-shit-done/workflows/settings.md
</execution_context>

<process>
**Follow the settings workflow** from `@~/.cursor/get-shit-done/workflows/settings.md`.

The workflow handles all logic including:
1. Config file creation with defaults if missing
2. Current config reading
3. Interactive settings presentation with pre-selection
4. Answer parsing and config merging
5. File writing
6. Confirmation display
</process>
