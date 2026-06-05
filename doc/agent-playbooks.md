AI Agent Playbooks
==================

These playbooks define the default workflow for common AI-assisted tasks. They
do not replace AGENTS.md, CONTRIBUTING.md, or the ADRs.


General Workflow
----------------

1. Read AGENTS.md and the task issue.
2. Read CONTRIBUTING.md, doc/ai-agents.md, and doc/testing.md.
3. Inspect the smallest relevant set of files before editing.
4. State or record the intended scope.
5. Make focused changes.
6. Run the relevant local checks.
7. Summarize changed files, validation, and residual risk.


Bug Fix
-------

Use this for reproducible defects.

1. Reproduce or identify the failing behavior.
2. Locate the owning module before editing.
3. Add or update a regression test when practical.
4. Fix the smallest behavior surface that owns the bug.
5. Run targeted tests, then the broader preset when risk justifies it.
6. Document any manual verification that remains.


Feature Work
------------

Use this for user-visible additions.

1. Confirm the goal, non-goals, and affected workflow.
2. Identify whether the feature belongs in document, command, tool, workspace,
   UI, render, or platform code.
3. Add durable document changes through commands.
4. Keep UI code thin and action-oriented.
5. Add tests for behavior and state transitions.
6. Update controls, build, release, or user documentation when behavior changes.


CI or Build Fix
---------------

Use this for workflow, compiler, test, or packaging failures.

1. Inspect the failing check, logs, and workflow.
2. Reproduce locally when practical with the matching preset or script.
3. Fix the project cause rather than masking the check.
4. Keep workflow changes aligned with CMakePresets.json and scripts/check.*.
5. Run `cmake --list-presets`, `ctest --list-presets`, and any targeted check
   touched by the change.


Code Review
-----------

Use this for review assistance.

1. Read the PR diff and relevant source context.
2. Prioritize correctness, regressions, ownership boundaries, and missing tests.
3. Cite file paths and lines for findings.
4. Avoid style-only feedback unless it violates shared project rules.
5. Distinguish confirmed issues from questions or risks.


Refactor
--------

Use this for behavior-preserving internal changes.

1. Define the behavior that must remain unchanged.
2. Identify the owning boundary and avoid cross-module reshaping unless required.
3. Prefer small mechanical steps with tests passing between them.
4. Avoid mixing refactor and feature behavior in the same change.
5. Run tests that cover all touched modules.


Release or Packaging
--------------------

Use this for release scripts, package layouts, or workflow publishing.

1. Read doc/build.md and doc/release.md.
2. Preserve the documented package asset names and installed layout unless the
   task explicitly changes them.
3. Run release smoke checks locally or through GitHub Actions when practical.
4. Update CHANGELOG.md and release notes guidance for user-visible changes.
5. Do not publish tags or releases unless explicitly instructed by a maintainer.
