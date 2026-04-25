# GitHub Collaboration Guidelines

## Goal

This document standardizes how GLDraw uses GitHub for issues, branches, commits, pull requests, and historical metadata maintenance.

The goals are:

- keep issues and pull requests easy to scan
- keep issue and PR relationships accurate
- make merged work easy to audit later
- reduce ambiguity in team collaboration

## Core Principles

- Accuracy comes before formatting.
- One PR should have one primary purpose.
- One PR should have at most one primary issue link.
- Metadata should help long-term maintenance, not just one-time discussion.
- Do not create issue or PR links based on guesses.

## Issue Guidelines

### When to open an issue

Open an issue for:

- bugs
- features
- documentation improvements
- testing work
- maintenance or cleanup work
- architecture or refactor work
- tracking, roadmap, or epic items

Avoid opening a separate issue for:

- tiny typo-only changes
- work already clearly covered by an existing issue
- discussion with no actionable outcome

### Issue title format

Use a short type prefix when practical:

- `bug:`
- `feat:`
- `docs:`
- `test:`
- `cleanup:`
- `maintenance:`
- `architecture:`
- `tracking:`
- `roadmap:`
- `epic:`

Examples:

- `bug: custom canvas rendering uses window coordinates instead of framebuffer coordinates`
- `feat: add zoom to fit (Ctrl+0)`
- `test: add CTest coverage for document and history semantics`

### Issue body structure

For feature, maintenance, and refactor issues:

```md
## Summary
One sentence describing the goal.

## Background
Context or motivation.

## Scope
- Included item
- Excluded item

## Acceptance Criteria
- [ ] Condition 1
- [ ] Condition 2
```

For bug issues:

```md
## Summary
One sentence describing the defect.

## Evidence
- Symptom
- Code location
- Reproduction clue

## Impact
User impact or technical risk.

## Suggested Fix
Implementation direction without over-constraining the solution.

## Acceptance Criteria
- [ ] Bug no longer reproduces
- [ ] No obvious regression remains
```

### Tracking, roadmap, and epic issues

Use tracking-style issues to group related work.

These issues:

- should list completed and remaining child issues
- should stay open until the tracked work is actually done
- should not require a single PR to `Closes` them
- may be updated through short status comments

Recommended status comment:

```md
Status update: completed items in this tracking issue are #49, #53, #56, and #51.

Remaining open items are #52, #50, #58, #59, #57, and #55.
```

## Branch Naming

Preferred format:

```md
type/issue-number-short-summary
```

Examples:

- `fix/51-pointer-capture-acceptance`
- `feat/28-zoom-to-fit`
- `docs/61-readme-improvements`
- `cleanup/57-shared-file-helpers`

If there is no issue:

- `docs/readme-cleanup`
- `chore/build-script-update`

Rules:

- use lowercase
- separate words with `-`
- include the issue number when possible
- keep names short and descriptive

## Commit Guidelines

### Commit title format

Use a concise Conventional-Commits-style prefix:

```md
type: short summary
```

Common types:

- `fix:`
- `feat:`
- `docs:`
- `refactor:`
- `test:`
- `build:`
- `chore:`
- `cleanup:`

Examples:

- `fix: capture pointer only after tool acceptance`
- `feat: add zoom to fit action`
- `docs: improve README build instructions`

### Commit expectations

- One commit should usually represent one coherent change.
- Describe the result, not a vague action.
- Avoid low-information titles such as:
  - `update`
  - `misc changes`
  - `fix stuff`

## Pull Request Guidelines

### When to open a PR

Open a PR when:

- the issue implementation is ready for review
- a coherent change slice is complete
- a documentation, test, or cleanup change is ready to land
- a post-merge fix needs its own review trail

### PR title format

Use:

```md
type: short summary
```

Examples:

- `fix: restore renderer-owned OpenGL state`
- `feat: add zoom to fit action`
- `docs: improve bilingual README and getting started docs`

### PR body format

For a primary implementation PR:

```md
Closes #xx

## Summary
- ...
- ...
- ...

## Testing
- ...
- ...
```

For a follow-up PR:

```md
Follow-up to #xx

## Summary
- ...
- ...
- ...

## Testing
- ...
- ...
```

Optional sections when needed:

```md
## Related
- refs #yy
- refs #zz

## Notes
- ...
```

### PR body rules

- Put the primary issue relationship at the top.
- Use at most one primary issue link.
- Use only:
  - `Closes #xx`
  - `Follow-up to #xx`
- `## Summary` is required.
- `## Testing` is required.
- `## Related` is optional.
- `## Notes` is optional.

Avoid mixing alternate section names such as:

- `## Test plan`
- `## Validation`
- `### Related`

### When to use `Closes` vs `Follow-up to`

Use `Closes #xx` when:

- the PR is the main implementation for the issue
- the issue should close when the PR merges
- the PR directly delivers the tracked work

Use `Follow-up to #xx` when:

- the main issue implementation already merged earlier
- the PR is a patch, cleanup, compatibility fix, or post-merge correction
- the PR should not pretend to be the original issue-closing change

Do not force a primary issue link when the relationship is unclear. In that case, keep only:

```md
## Summary
...

## Testing
...
```

### How to write `Summary`

Write the result and scope, not a file-by-file diary.

Recommended:

```md
## Summary
- make pointer capture reflect actual tool acceptance
- keep ignored clicks from entering captured mode
- align controller state with the tool interaction lifecycle
```

Avoid:

- long implementation play-by-play
- listing every edited file
- excessive background that belongs in the issue

### How to write `Testing`

Only list testing that actually happened.

Recommended:

```md
## Testing
- cmake --build build --config Release
- ctest --test-dir build --output-on-failure
- manual check: ignored clicks no longer trigger pointer capture
```

Avoid:

- `Tested`
- `Works`
- claiming verification that did not happen

## Historical Metadata Normalization

### Merged PRs

Historical merged PR bodies may be edited to match the current format.

Allowed edits:

- primary issue line
- `## Summary`
- `## Testing`
- optional `## Related`
- optional `## Notes`

Do not change by default:

- PR title
- code
- commit history

### Closed issues

Each implemented issue should have one standard implementation comment.

Single PR:

```md
Implemented by merged PR #xx.
```

Multiple PRs:

```md
Implemented by merged PRs #xx and #yy.
```

Primary PR plus follow-up:

```md
Implemented by merged PR #xx, with follow-up fixes in merged PR #yy.
```

### Implemented but still-open issues

If an issue is already implemented by merged work but remains open:

- close the issue manually
- add the standard implementation comment

### Tracking and roadmap issues

Tracking-style issues may remain open across multiple PRs.

Use short status comments instead of forcing one merged PR to close them.

## Recommended Workflow

1. Create or confirm the issue.
2. Create a branch from the issue.
3. Implement and verify the change locally.
4. Commit with a clear commit title.
5. Open a PR with the standard body format.
6. Merge the PR.
7. Confirm the issue closed automatically.
8. If it did not close automatically, close it manually and add the standard implementation comment.

## Example Commands

Create a branch:

```sh
git checkout -b fix/51-pointer-capture-acceptance
```

Commit a change:

```sh
git add .
git commit -m "fix: capture pointer only after tool acceptance"
```

Push and open a PR:

```sh
git push -u origin fix/51-pointer-capture-acceptance
gh pr create
```

## Review Checklist

Before merging, check:

- the PR has the correct primary issue line, or intentionally has none
- the PR includes `## Summary`
- the PR includes `## Testing`
- the issue relationship is accurate
- the issue will close automatically if `Closes #xx` is used

## Anti-Patterns

Avoid:

- multiple `Closes #xx` lines at the top of one PR
- guessed issue links
- long free-form PR descriptions with no structure
- duplicate implementation comments on the same issue
- missing testing details in a PR
- vague commit messages
- treating roadmap issues like ordinary single-delivery issues

## Preferred Templates

### Primary implementation PR

```md
Closes #xx

## Summary
- ...
- ...
- ...

## Testing
- ...
- ...
```

### Follow-up PR

```md
Follow-up to #xx

## Summary
- ...
- ...
- ...

## Testing
- ...
- ...
```

### Standard implementation comment

```md
Implemented by merged PR #xx.
```

```md
Implemented by merged PRs #xx and #yy.
```

```md
Implemented by merged PR #xx, with follow-up fixes in merged PR #yy.
```

### Tracking status update

```md
Status update: completed items in this tracking issue are #xx, #yy, and #zz.

Remaining open items are #aa, #bb, and #cc.
```

## Outcome

The point of this guideline is not cosmetic consistency. The point is to make it easy for anyone reading the repository later to understand:

- what changed
- why it changed
- how it was verified
- which issue the change belongs to
- which tracked work is done versus still pending
