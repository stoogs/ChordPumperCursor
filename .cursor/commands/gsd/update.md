---
tools:
  bash: true
  ask_question: true
name: gsd-update
description: Update GSD to latest version with changelog display

<objective>
Check for GSD updates, install if available, and display what changed.

Routes to the update workflow which handles:
</objective>

<execution_context>
@~/.cursor/get-shit-done/workflows/update.md
</execution_context>

<process>
**Follow the update workflow** from `@~/.cursor/get-shit-done/workflows/update.md`.

The workflow handles all logic including:
1. Installed version detection (local/global)
2. Latest version checking via npm
3. Version comparison
4. Changelog fetching and extraction
5. Clean install warning display
6. User confirmation
7. Update execution
8. Cache clearing
</process>
