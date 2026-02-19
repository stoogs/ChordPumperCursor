---
tools:
  read: true
  write: true
  bash: true
name: gsd-pause-work
description: Create context handoff when pausing work mid-phase

<objective>
Create `.continue-here.md` handoff file to preserve complete work state across sessions.

Routes to the pause-work workflow which handles:
</objective>

<execution_context>
@.planning/STATE.md
@~/.cursor/get-shit-done/workflows/pause-work.md
</execution_context>

<process>
**Follow the pause-work workflow** from `@~/.cursor/get-shit-done/workflows/pause-work.md`.

The workflow handles all logic including:
1. Phase directory detection
2. State gathering with user clarifications
3. Handoff file writing with timestamp
4. Git commit
5. Confirmation with resume instructions
</process>
