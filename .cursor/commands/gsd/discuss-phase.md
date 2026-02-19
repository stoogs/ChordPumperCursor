---
tools:
  read: true
  write: true
  bash: true
  glob: true
  grep: true
  ask_question: true
name: gsd-discuss-phase
description: Gather phase context through adaptive questioning before planning
argument-hint: "<phase> [--auto]"

<objective>
Extract implementation decisions that downstream agents need — researcher and planner will use CONTEXT.md to know what to investigate and what choices are locked.

**How it works:**
1. Analyze the phase to identify gray areas (UI, UX, behavior, etc.)
2. Present gray areas — user selects which to discuss
3. Deep-dive each selected area until satisfied
4. Create CONTEXT.md with decisions that guide research and planning

**Output:** `{phase_num}-CONTEXT.md` — decisions clear enough that downstream agents can act without asking the user again
</objective>

<execution_context>
@~/.cursor/get-shit-done/workflows/discuss-phase.md
@~/.cursor/get-shit-done/templates/context.md
</execution_context>

<context>
Phase number: $ARGUMENTS (required)

**Load project state:**
@.planning/STATE.md

**Load roadmap:**
@.planning/ROADMAP.md
</context>

<process>
1. Validate phase number (error if missing or not in roadmap)
2. Check if CONTEXT.md exists (offer update/view/skip if yes)
3. **Analyze phase** — Identify domain and generate phase-specific gray areas
4. **Present gray areas** — Multi-select: which to discuss? (NO skip option)
5. **Deep-dive each area** — 4 questions per area, then offer more/next
6. **Write CONTEXT.md** — Sections match areas discussed
7. Offer next steps (research or plan)

**CRITICAL: Scope guardrail**

**Domain-aware gray areas:**
Gray areas depend on what's being built. Analyze the phase goal:

Generate 3-4 **phase-specific** gray areas, not generic categories.

**Probing depth:**

**Do NOT ask about (Claude handles these):**
</process>

<success_criteria>
</success_criteria>
