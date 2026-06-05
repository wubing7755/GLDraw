Testing GLDraw
==============

GLDraw uses CTest for automated tests. CI is the source of truth for platform
coverage, while local checks should focus on the modules touched by a change.


Quick Commands
--------------

Recommended local check:

```sh
./scripts/check.sh
```

Windows PowerShell:

```powershell
./scripts/check.ps1
```

Target a specific preset when needed:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

AddressSanitizer and coverage presets are available for supported toolchains:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

```sh
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage --output-on-failure
```


Test Layers
-----------

Use the narrowest test layer that proves the behavior:

* Document tests should cover object storage, layers, IDs, bounds, persistence,
  hit-testing, property bags, and spatial behavior.
* Command tests should cover executable-state checks, success paths, undo/redo,
  and dirty-state consequences when applicable.
* Tool tests should cover lifecycle decisions, command creation, cancellation,
  and preview state where practical.
* Workspace/service tests should cover save, load, export, clipboard, selection,
  dialogs, and cross-module behavior.
* UI logic tests should cover view-model and state derivation. Avoid testing
  Nuklear drawing details directly unless the behavior is owned locally.
* Render tests should cover renderer-owned contracts without moving document
  behavior into render code.

Test files are grouped by ownership area:

```text
tests/
  app/
  base/
  commands/
  document/
  image/
  render/
  script/
  support/
  ui/
```

Shared test-only shims belong in `tests/support/`. Keep new tests near the
module boundary they verify.


Regression Expectations
-----------------------

Bug fixes should include a regression test unless the behavior is only
observable through platform graphics output or manual UI interaction. When a
test is not practical, document the reason in the pull request and include a
manual verification note.

New object types should usually include tests for:

* Creation and destruction.
* Clone and translate behavior.
* Bounds and hit-testing.
* Property read/write behavior.
* Persistence round trips when the object is serialized.

New commands should usually include tests for:

* Availability checks.
* Successful execution.
* Undo and redo.
* No-op or invalid-input behavior.

New tools should usually include tests for:

* Activation and cancellation.
* Input sequence handling.
* Command emission for durable edits.
* Overlay or preview state when applicable.


Coverage
--------

Coverage reports are generated in CI for Linux. Treat coverage as a guide for
risk, not as a replacement for focused assertions. Avoid adding tests that only
exercise lines without validating behavior.


Manual Verification
-------------------

Use manual checks for changes that affect:

* Canvas interaction feel.
* Visual rendering output.
* Menus, dialogs, panels, or theme appearance.
* Platform packaging and executable layout.

Attach screenshots or short recordings to the pull request when the result is
visible.
