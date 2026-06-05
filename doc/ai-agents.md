AI Agent Policy
===============

AI agents may help with GLDraw development, but their output is treated like any
other contribution: it must follow the architecture boundaries, pass the relevant
checks, and receive human review.


Required Reading
----------------

Before changing files, an AI agent should read:

* AGENTS.md
* CONTRIBUTING.md
* doc/testing.md
* This document

For architecture-sensitive work, also read:

* doc/adr/README.md
* The ADRs that match the touched area
* The source entry points listed in AGENTS.md

For release, packaging, CI, or dependency work, also read:

* doc/build.md
* doc/release.md
* SECURITY.md when dependency or vulnerability handling is involved


Appropriate Uses
----------------

AI agents are appropriate for:

* Focused bug fixes with clear reproduction steps.
* Small to medium implementation tasks with explicit acceptance criteria.
* Test additions and regression coverage.
* Documentation updates.
* Build, CI, packaging, and repository maintenance.
* Mechanical refactors that preserve behavior and are easy to review.
* First-pass code review or risk analysis.


Restricted Uses
---------------

AI agents should not independently:

* Redesign core architecture without an ADR or maintainer direction.
* Change document file format compatibility without explicit review.
* Bypass `CommandExecutor` for durable document mutations.
* Move editing rules into UI widgets.
* Add direct `workspace_internal.h` dependencies outside approved workspace
  implementation or lifecycle tests.
* Weaken tests, CI, static analysis, security checks, or release validation to
  make a change pass.
* Introduce generated code, large vendored dependencies, or license-sensitive
  assets without maintainer approval.
* Publish releases, rotate credentials, or change repository permissions.


Human Review Requirements
-------------------------

Human review is required for all AI-assisted pull requests. Reviewers should
check:

* Whether the agent read and followed the required context.
* Whether the change is scoped to the task.
* Whether architecture boundaries are preserved.
* Whether tests cover the actual behavior.
* Whether generated text, code, or assets are license-safe and appropriate for
  the repository.
* Whether any manual UI, rendering, or packaging verification is still needed.


Disclosure
----------

Pull requests should disclose meaningful AI assistance in the PR template. The
disclosure should name the agent or tool, describe what it did, list the required
docs read, and summarize human verification.

Small editor completions or syntax suggestions do not need detailed disclosure
unless they materially shaped the design or implementation.


Traceability
------------

Agent-suitable issues should use the AI agent task template. The issue should
define:

* Background.
* Goal.
* Non-goals.
* Required context.
* Expected files or modules.
* Acceptance criteria.
* Required checks.

This keeps agent work reviewable and reduces the chance of broad, unstated
changes.
