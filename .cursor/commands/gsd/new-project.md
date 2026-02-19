---
tools:
  read: true
  bash: true
  write: true
  ask_question: true
name: gsd-new-project
description: Initialize a new project with deep context gathering and PROJECT.md
argument-hint: "[--auto]"
<context>
**Flags:**
</context>

<objective>
Initialize a new project through unified flow: questioning → research (optional) → requirements → roadmap.

**Creates:**

**After this command:** Run `/gsd-plan-phase 1` to start execution.
</objective>

<execution_context>
@~/.cursor/get-shit-done/workflows/new-project.md
@~/.cursor/get-shit-done/references/questioning.md
@~/.cursor/get-shit-done/references/ui-brand.md
@~/.cursor/get-shit-done/templates/project.md
@~/.cursor/get-shit-done/templates/requirements.md
</execution_context>

<process>
Execute the new-project workflow from @~/.cursor/get-shit-done/workflows/new-project.md end-to-end.
Preserve all workflow gates (validation, approvals, commits, routing).
</process>
