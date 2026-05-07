# GitHub Collaboration Guidelines

> Audience: contributors, maintainers
> Purpose: keep issues, branches, commits, and pull requests consistent and auditable
> Source of truth: repository workflow conventions
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [github-templates.md](github-templates.md)

## Goal

This document standardizes how GLDraw uses GitHub for issues, branches, commits, pull requests, and historical metadata maintenance.

The goals are:

- keep issues and pull requests easy to scan
- keep issue and PR relationships accurate
- make merged work easy to audit later
- reduce ambiguity in team collaboration

## Use This Page With

- [github-templates.md](github-templates.md) for copy-paste issue, PR, and status templates

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

### Issue body structure

Use the templates in [github-templates.md](github-templates.md).

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

### Commit expectations

- One commit should usually represent one coherent change.
- Describe the result, not a vague action.
- Avoid low-information titles such as `update`, `misc changes`, or `fix stuff`.

## Pull Request Guidelines

### PR title format

Use:

```md
type: short summary
```

### PR body format

Use the templates in [github-templates.md](github-templates.md).

### PR body rules

- Put the primary issue relationship at the top.
- Use at most one primary issue link.
- Use only `Closes #xx` or `Follow-up to #xx`.
- `## Summary` is required.
- `## Testing` is required.
- `## Related` and `## Notes` are optional.

### How to write `Summary`

Write the result and scope, not a file-by-file diary.

Recommended shape:

- result-focused
- scoped
- short enough to scan quickly

### How to write `Testing`

Only list testing that actually happened.

Recommended shape:

- only list testing that actually happened
- use real commands when possible
- include manual verification notes when behavior was checked interactively

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

Use the standard comment forms in [github-templates.md](github-templates.md).

## Recommended Workflow

1. Create or confirm the issue.
2. Create a branch from the issue.
3. Implement and verify the change locally.
4. Commit with a clear commit title.
5. Open a PR with the standard body format.
6. Merge the PR.
7. Confirm the issue closed automatically.
8. If it did not close automatically, close it manually and add the standard implementation comment.

## Review Checklist

- the PR has the correct primary issue line, or intentionally has none
- the PR includes `## Summary`
- the PR includes `## Testing`
- the issue relationship is accurate
- the issue will close automatically if `Closes #xx` is used

## Anti-Patterns

- multiple `Closes #xx` lines at the top of one PR
- guessed issue links
- long free-form PR descriptions with no structure
- duplicate implementation comments on the same issue
- missing testing details in a PR
- vague commit messages
- treating roadmap issues like ordinary single-delivery issues
