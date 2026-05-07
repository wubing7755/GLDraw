# GitHub Templates

> Audience: contributors, maintainers
> Purpose: provide copy-paste issue, PR, and historical metadata templates
> Source of truth: repository collaboration workflow
> Last reviewed with code: 2026-05-07
> Related: [github-collaboration-guidelines.md](github-collaboration-guidelines.md)

## Feature, Maintenance, or Refactor Issue

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

## Bug Issue

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

## Primary Implementation PR

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

## Follow-up PR

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

## Optional PR Sections

```md
## Related
- refs #yy

## Notes
- ...
```

## Tracking Status Update

```md
Status update: completed items in this tracking issue are #xx, #yy, and #zz.

Remaining open items are #aa, #bb, and #cc.
```

## Standard Implementation Comment

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
