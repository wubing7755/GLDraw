# GLDraw Documentation

This directory is organized by audience, task, and source-of-truth ownership.

## Start by Task

| I want to... | Start here | Then read |
|---|---|---|
| Build and run the app | [user/getting-started.md](user/getting-started.md) | [user/controls.md](user/controls.md) |
| Learn default controls | [user/controls.md](user/controls.md) | [user/getting-started.md](user/getting-started.md) |
| Understand the current architecture | [architecture/overview.md](architecture/overview.md) | [architecture/core-systems.md](architecture/core-systems.md), [architecture/data-flow.md](architecture/data-flow.md) |
| Plan architecture refactoring | [architecture/refactor-roadmap.md](architecture/refactor-roadmap.md) | [architecture/overview.md](architecture/overview.md), [architecture/core-systems.md](architecture/core-systems.md) |
| Add a new object type or tool | [architecture/extension-model.md](architecture/extension-model.md) | [reference/file-map.md](reference/file-map.md), [contributing/overview.md](contributing/overview.md) |
| Find the right source file quickly | [reference/file-map.md](reference/file-map.md) | [architecture/core-systems.md](architecture/core-systems.md) |
| Start contributing code | [contributing/overview.md](contributing/overview.md) | [contributing/c-contributor-guide.zh.md](contributing/c-contributor-guide.zh.md) or [contributing/c-contributor-guide.en.md](contributing/c-contributor-guide.en.md) |
| Prepare issues and pull requests | [contributing/github-collaboration-guidelines.md](contributing/github-collaboration-guidelines.md) | [contributing/github-templates.md](contributing/github-templates.md) |
| Read historical design context | [archive/architecture-layering-plan.md](archive/architecture-layering-plan.md) | [architecture/overview.md](architecture/overview.md) |

## Document Map

| Area | Purpose |
|---|---|
| `user/` | Build, run, controls, and first-session guidance |
| `contributing/` | Contributor workflows, code-reading guidance, and GitHub policy |
| `architecture/` | Current runtime structure, subsystem boundaries, and extension model |
| `reference/` | Stable lookup material such as file maps |
| `archive/` | Historical notes that are no longer the current source of truth |

## Source of Truth

| Topic | Canonical page |
|---|---|
| Build and run | [user/getting-started.md](user/getting-started.md) |
| Default controls | [user/controls.md](user/controls.md) |
| Runtime layering | [architecture/overview.md](architecture/overview.md) |
| Architecture refactor sequence | [architecture/refactor-roadmap.md](architecture/refactor-roadmap.md) |
| Subsystem ownership | [architecture/core-systems.md](architecture/core-systems.md) |
| Event and render flow | [architecture/data-flow.md](architecture/data-flow.md) |
| Object and tool extension model | [architecture/extension-model.md](architecture/extension-model.md) |
| Source file entrypoints | [reference/file-map.md](reference/file-map.md) |
| GitHub workflow rules | [contributing/github-collaboration-guidelines.md](contributing/github-collaboration-guidelines.md) |

## Maintenance Rules

- Prefer linking to a canonical page over copying instructions.
- When ownership boundaries or file names change, update `architecture/` and `reference/` together.
- When shortcuts or tool metadata change, update `user/controls.md`.
- When build outputs or supported toolchains change, update `user/getting-started.md`.
- Historical notes belong in `archive/`, not in current architecture pages.

## Legacy Redirects

The old `doc/wiki/` pages and several top-level legacy Markdown files are retained as lightweight redirects so existing links do not fail abruptly. New documentation should link to the canonical pages listed above.
