# Contributing to GLDraw

GLDraw is a C11 OpenGL drawing editor. This guide keeps day-to-day
contributions aligned across contributors, reviewers, and CI.

## Development Setup

Use the repository build scripts for normal local builds:

```sh
./build.sh
./build.sh debug
```

On Windows with MinGW/MSYS2 from CMD:

```bat
build.bat
build.bat debug
```

Manual CMake and packaging details are documented in [doc/build.md](doc/build.md).
Optional CMake presets are available for contributors with a recent CMake:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

## Branches and Commits

Use short, scoped branch names:

* `feature/<topic>` for user-facing behavior.
* `fix/<topic>` for bug fixes.
* `refactor/<topic>` for internal reshaping without intended behavior changes.
* `infra/<topic>` for build, CI, documentation, packaging, or tooling.
* `release/<version>` for release preparation.

Prefer focused commits that can be reviewed independently. Commit subjects should
start with a useful scope when possible, for example:

```text
fix(commands): preserve selection after undo
feat(tools): add polygon drag lifecycle
chore(ci): align builds with CMake presets
```

## Pull Requests

Before opening a pull request:

* Rebase or merge the target branch if the change depends on recent CI or build
  updates.
* Run the smallest local verification that covers the touched area.
* Include screenshots or short recordings for visible UI, canvas, rendering, or
  theme changes.
* Explain any compatibility, file format, packaging, or release impact.
* Link issues with `Closes #...` or `Refs #...` when applicable.

Reviewers should focus first on behavior, ownership boundaries, and tests. Style
feedback should point back to the shared formatting rules where possible.

## Architecture Boundaries

Keep changes aligned with the current ownership model:

* Durable document mutations should go through `CommandExecutor`.
* Object storage, layers, IDs, revisions, and spatial queries belong in
  `document/`.
* Selection, clipboard, dialogs, dirty state, and layout are workspace/session
  concerns.
* Tool input should flow through `editor_controller` and `tool_runtime`.
* UI code should consume view-model or workspace query state and emit actions; it
  should not own editing rules.
* Rendering consumes render scene state; it should not define document behavior.
* Platform-specific input values should be converted at the app/input boundary.
* Resource lookup for shaders, themes, and scripts should use
  `base/resource_path`.

Avoid adding new includes of `workspace_internal.h` outside workspace
implementation files unless a test explicitly verifies internal lifecycle
behavior.

## Adding Document Objects

Objects are descriptor-driven through `GraphicObjectDescriptor`.

When adding or changing an object type:

* Implement create, clone, destroy, translate, bounds, hit-test, path, property,
  and persistence callbacks as needed.
* Register descriptors with `register_object_type()`.
* Add built-in object registration through `src/app/extension_manifest.c`.
* Prefer `GRAPHIC_OBJECT_INVALID` for new auto-assigned object type IDs.
* Add focused tests for persistence, bounds, hit-testing, and property behavior
  when the object affects those contracts.

## Adding Tools

Tools are descriptor-driven through `ToolDescriptor`.

When adding or changing a tool:

* Register custom tools with `register_tool()`.
* Use `register_shape_tool()` when the standard shape-drag lifecycle is enough.
* Add built-in tool registration through `src/app/tool_manifest.c`.
* Keep durable edits command-based; use overlays for previews.
* Test command creation and cancellation behavior where practical.

## Adding Commands

Command metadata, availability, and execution are split:

* `command_catalog` owns command descriptors and IDs.
* `command_availability` owns executable-state checks.
* `command_registry_execute()` remains the public execution entrypoint.
* Concrete workspace behavior is split across focused workspace command modules.

Command changes should include tests for success paths, unavailable states, and
undo/redo behavior when durable document state changes.

## UI Changes

UI frame construction is split across focused `src/ui/` modules. Keep UI changes
thin: read workspace query state, present controls, and dispatch actions. Avoid
moving editing rules into widgets.

Visible UI changes should include a screenshot or short recording in the pull
request when possible.

## Local Verification

Use the local check scripts when available:

```sh
./scripts/check.sh
```

On Windows PowerShell:

```powershell
./scripts/check.ps1
```

For targeted work, run the relevant CMake preset directly:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

CI remains the source of truth for platform coverage.
