---
tools:
  read: true
  write: true
  bash: true
type: prompt
name: gsd-complete-milestone
description: Archive completed milestone and prepare for next version
argument-hint: <version>

<objective>
Mark milestone {{version}} complete, archive to milestones/, and update ROADMAP.md and REQUIREMENTS.md.

Purpose: Create historical record of shipped version, archive milestone artifacts (roadmap + requirements), and prepare for next milestone.
Output: Milestone archived (roadmap + requirements), PROJECT.md evolved, git tagged.
</objective>

<execution_context>
**Load these files NOW (before proceeding):**

  </execution_context>

<context>
**Project files:**

**User input:**

  </context>

<process>

**Follow complete-milestone.md workflow:**

0. **Check for audit:**


   ```markdown
   ## Pre-flight Check

   {If no v{{version}}-MILESTONE-AUDIT.md:}
   ⚠ No milestone audit found. Run `/gsd-audit-milestone` first to verify
   requirements coverage, cross-phase integration, and E2E flows.

   {If audit has gaps:}
   ⚠ Milestone audit found gaps. Run `/gsd-plan-milestone-gaps` to create
   phases that close the gaps, or proceed anyway to accept as tech debt.

   {If audit passed:}
   ✓ Milestone audit passed. Proceeding with completion.
   ```

1. **Verify readiness:**


2. **Gather stats:**


3. **Extract accomplishments:**


4. **Archive milestone:**


5. **Archive requirements:**


6. **Update PROJECT.md:**


7. **Commit and tag:**


8. **Offer next steps:**

</process>

<success_criteria>

  </success_criteria>

<critical_rules>

  </critical_rules>
