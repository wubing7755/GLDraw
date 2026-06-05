# ADR 0005: AI Agent Contribution Boundaries

## Status

Accepted.

## Context

AI agents can accelerate repository maintenance, tests, documentation, and
implementation work. They can also make broad changes quickly, miss project
context, or bypass architecture boundaries if the rules are not explicit.

## Decision

AI-assisted contributions are allowed, but they must follow the same repository
rules as human-authored contributions. AGENTS.md is the required entry point for
AI agents. AI agent policy, playbooks, issue templates, and PR disclosure are
used to make agent work reviewable and auditable.

## Consequences

* AI agents must read the required repository context before editing.
* AI-generated code must preserve Workspace, CommandExecutor, tool, UI, render,
  and document ownership boundaries.
* Pull requests should disclose meaningful AI assistance and human verification.
* Agent-suitable issues should define goals, non-goals, acceptance criteria, and
  required checks.
* Maintainers may reject AI-assisted changes that are too broad, weakly tested,
  or not traceable to the task context.
