# Contributing Overview

> Audience: contributors, maintainers
> Purpose: choose the right reading order and the right edit path before changing code
> Source of truth: current source tree, `AGENTS.md`, architecture docs in this directory
> Last reviewed with code: 2026-05-07
> Related: [c-contributor-guide.zh.md](c-contributor-guide.zh.md), [../architecture/overview.md](../architecture/overview.md)

## Start Here

Read these in order if you are new to the codebase:

1. [../architecture/overview.md](../architecture/overview.md)
2. [../architecture/data-flow.md](../architecture/data-flow.md)
3. [../reference/file-map.md](../reference/file-map.md)
4. [c-contributor-guide.zh.md](c-contributor-guide.zh.md) or [c-contributor-guide.en.md](c-contributor-guide.en.md)

## Common Tasks

| Task | Best first page |
|---|---|
| Understand architecture before editing | [../architecture/overview.md](../architecture/overview.md) |
| Find the right folder or file | [../reference/file-map.md](../reference/file-map.md) |
| Add a new object type or tool | [../architecture/extension-model.md](../architecture/extension-model.md) |
| Learn GLDraw-specific coding expectations | [c-contributor-guide.zh.md](c-contributor-guide.zh.md) or [c-contributor-guide.en.md](c-contributor-guide.en.md) |
| Prepare issues and PRs | [github-collaboration-guidelines.md](github-collaboration-guidelines.md) |

## Choose the Right Layer

- Object storage, layers, and spatial queries:
  edit `document/`.
- Undoable document mutations:
  edit `commands/`.
- Tool-local interaction behavior:
  edit `tools/`.
- Viewport, zoom, and world/screen transforms:
  edit `canvas/`.
- File flows, app orchestration, and top-level command dispatch:
  edit `app/`.
- Menus, inspector, dialogs, and chrome:
  edit `ui/`.

## Contributor Expectations

- Durable document changes should go through `CommandExecutor`.
- New object types should be descriptor-driven and registered through the object manifest.
- New tools should be descriptor-driven and registered through the tool manifest.
- Docs should be updated when ownership boundaries, shortcuts, or extension entrypoints change.

## Verification Baseline

- Build the app locally.
- Manually check the user path you changed.
- Update architecture or controls docs if the behavior is user-visible or structural.
- Prefer focused tests in `tests/` when touching command, registry, or persistence behavior.
