---
tools:
  read: true
  write: true
  bash: true
  ask_question: true
name: gsd-new-milestone
description: Start a new milestone cycle — update PROJECT.md and route to requirements
argument-hint: "[milestone name, e.g., 'v1.1 Notifications']"
<objective>
Start a new milestone: questioning → research (optional) → requirements → roadmap.

Brownfield equivalent of new-project. Project exists, PROJECT.md has history. Gathers "what's next", updates PROJECT.md, then runs requirements → roadmap cycle.

**Creates/Updates:**

**After:** `/gsd-plan-phase [N]` to start execution.
</objective>

<execution_context>
@~/.cursor/get-shit-done/workflows/new-milestone.md
@~/.cursor/get-shit-done/references/questioning.md
@~/.cursor/get-shit-done/references/ui-brand.md
@~/.cursor/get-shit-done/templates/project.md
@~/.cursor/get-shit-done/templates/requirements.md
</execution_context>

<context>
Milestone name: $ARGUMENTS (optional - will prompt if not provided)

**Load project context:**
@.planning/PROJECT.md
@.planning/STATE.md
@.planning/MILESTONES.md
@.planning/config.json

**Load milestone context (if exists, from /gsd-discuss-milestone):**
@.planning/MILESTONE-CONTEXT.md
</context>

<process>
Execute the new-milestone workflow from @~/.cursor/get-shit-done/workflows/new-milestone.md end-to-end.
Preserve all workflow gates (validation, questioning, research, requirements, roadmap approval, commits).
</process>
