# Refactor Log

## 2026-05-28 - Phase 3: Persistence Regression Coverage

- Branch:
  `refactor-editor-architecture-roadmap`
- Audit:
  `src/document/persistence.c` currently combines JSON tokenization/skipping/primitive parsing, document JSON writing, layer parsing, object parsing and loaded-object construction, plus root load orchestration.
  The intended split boundaries are JSON reader, writer, layer parser, object parser, and public save/load orchestration.
- Modified files:
  `tests/test_document_core.c`,
  `doc/reference/file-map.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Extended persistence coverage for save/load round-trip, layer visibility/lock/name/blend mode round-trip, object geometry/style property round-trip, unknown top-level/layer/object/stroke fields being ignored, malformed JSON rejection, and load failure preserving the existing document.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 2: Consolidate Theme System Modules

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme.c`,
  `REFACTOR_LOG.md`
- Key changes:
  Updated the `ui_theme.c` file contract to match its new role as public theme registry orchestration over focused built-in, parse, external, settings, and Nuklear apply modules.
  Confirmed `include/ui/ui_theme.h` remains the public API boundary and does not expose internal parsing/loading details.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 2: Split Nuklear Theme Application

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme_apply.c`,
  `src/ui/ui_theme.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Moved `ui_theme_apply()` and its color-mixing helper into `ui_theme_apply.c`.
  Kept public theme lookup/orchestration APIs in `ui_theme.c`.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed after removing a trailing blank line.

## 2026-05-28 - Phase 2: Split Theme Settings Persistence

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme_settings.c`,
  `src/ui/ui_theme.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Moved selected theme ID load/save behavior into `ui_theme_settings.c`.
  Kept the public `ui_theme_load_selected_id()` and `ui_theme_save_selected_id()` API unchanged.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 2: Split External Theme Loading

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme_external.c`,
  `src/ui/ui_theme_internal.h`,
  `src/ui/ui_theme.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Moved external theme registry storage, custom theme lookup/accessors, external file parsing, directory reload/rollback, reload error storage, and directory signature hashing into `ui_theme_external.c`.
  Kept public lookup APIs in `ui_theme.c` as orchestration over built-in and external theme helpers.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 2: Split Theme Parsing Helpers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme_parse.c`,
  `src/ui/ui_theme_internal.h`,
  `src/ui/ui_theme.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Moved ad hoc JSON string/number/bool extraction, key detection, hex color parsing, path-derived theme IDs, color/float/bool override application, safe theme ID validation, and token clamp logic into `ui_theme_parse.c`.
  Published only private cross-module helper declarations through `ui_theme_internal.h`; the public theme API remains unchanged.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 2: Split Built-In Theme Definitions

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_theme_builtin.c`,
  `src/ui/ui_theme_internal.h`,
  `src/ui/ui_theme.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Moved light, dark plus, and high-contrast token construction plus built-in descriptor lookup/count helpers into `ui_theme_builtin.c`.
  Added a private `ui_theme_internal.h` boundary for shared theme constants and built-in helper declarations while keeping `include/ui/ui_theme.h` unchanged.
  Left external theme loading, settings persistence, parsing, and Nuklear apply behavior in `ui_theme.c` for subsequent focused splits.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-28 - Phase 1: Theme System Regression Coverage

- Branch:
  `refactor-editor-architecture-roadmap`
- Audit:
  `src/ui/ui_theme.c` currently combines built-in theme definitions, external theme registry/reload, ad hoc JSON value extraction, hex color parsing, token clamp/validation, selected-theme settings persistence, external directory signatures, and Nuklear style application.
  The intended split boundaries are built-ins, parse helpers, external loading, settings persistence, and Nuklear apply.
- Modified files:
  `tests/test_ui_theme.c`,
  `tests/test_nuklear_impl.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `REFACTOR_LOG.md`
- Key changes:
  Added focused theme regression coverage for default/built-in lookup, invalid theme fallback, external hex color loading, reload error rollback, selected theme ID load/save, compatibility settings key load, and numeric token clamp behavior.
  Registered the new `gldraw_ui_theme_tests` target and included it in `gldraw_tests`.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed with 12/12 tests passing.
  `git diff --check` passed.

## 2026-05-26 - P2: Complete UI Composition Refactor Round

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_context_menu_internal.h`,
  `src/ui/ui_context_menu_render.c`,
  `src/ui/ui_context_menu.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Moved canvas context menu item rendering and popup host rendering into `ui_context_menu_render.c`.
  Kept `ui_context_menu.c` focused on context menu lifecycle, open/reset state, and input handling.
  Added a small private `ui_context_menu_internal.h` boundary for shared context menu constants and popup bounds.
  Updated roadmap and file map to mark this refactor round complete for command routing, workspace API coupling, view-model construction, and low-risk UI composition cleanup.
- Boundary audit:
  Public app/UI headers no longer include `workspace_internal.h`.
  Remaining `workspace_internal.h` includes are limited to workspace implementation modules and tests that inspect internal lifecycle/state.
  Remaining `workspace.h` includes are implementation-level consumers or application/input integration points that call workspace accessors directly.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.
  `git diff --check` passed.

## 2026-05-26 - P2: Split Menu Bar Rendering From State

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_menubar_internal.h`,
  `src/ui/ui_menubar_render.c`,
  `src/ui/ui_menubar.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Moved top-menu dropdown rendering, theme menu rendering, quick action buttons, and command tooltip formatting into `ui_menubar_render.c`.
  Kept `ui_menubar.c` focused on menu bar lifecycle, state accessors, theme request consumption, and action dispatch after rendering.
  Added a small private `ui_menubar_internal.h` boundary shared only by menu bar implementation files.
  Registered the new render implementation in the Nuklear UI target and updated architecture/file-map docs.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split Layer Panel From Inspector UI

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_layer_panel.c`,
  `src/ui/ui_inspector_panel.c`,
  `src/ui/ui_system_internal.h`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Moved active-layer controls, layer rename state synchronization, and layer list rendering into `ui_layer_panel.c`.
  Kept `ui_inspector_panel.c` focused on selection overview and editable property rendering.
  Exposed `ui_layers_panel()` through the private UI system internal header for inspector composition.
  Registered the new UI implementation file in the Nuklear UI target and updated architecture/file-map docs.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split Editor ViewModel Snapshot Builders

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_viewmodel.c`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Added an internal `EditorViewModelBuildContext` so workspace accessors are captured once at the view-model build boundary.
  Added an internal selection snapshot shared by summary and property builders.
  Split view-model population into summary, command, tool, property, layer, and dialog builders while keeping the public `EditorViewModel` shape and call sites stable.
  Updated the architecture roadmap to mark view-model construction cleanup complete and move the suggested next slice to UI implementation decomposition.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Replace Command Registry Switch With Route Table

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/command_registry.c`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Replaced the fixed-command execution switch in `command_registry_execute()` with a static command-to-handler route table.
  Kept dynamic tool command execution as a separate path delegated to `workspace_tool_commands`.
  Removed `command_registry.c`'s direct include of the full Workspace API because routing only needs the lightweight workspace service type declaration.
  Updated the architecture roadmap so the suggested next slice moves on to view-model construction cleanup.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Forward Declare Workspace In App Facade Headers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_dispatcher.h`,
  `include/app/editor_action_handler.h`,
  `include/app/editor_controller.h`
- Key changes:
  Replaced full `workspace.h` includes in command dispatcher, editor action handler, and editor controller public headers with lightweight workspace service type declarations where possible.
  Kept concrete workspace API dependencies in implementation files that actually call workspace accessors.
  Reduced public app facade header coupling while preserving the existing controller and action dispatch contracts.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split Workspace Service Types From Workspace API

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_service_types.h`,
  `include/app/workspace.h`,
  `include/app/workspace_actions.h`,
  `include/app/workspace_dialogs.h`,
  `include/app/workspace_service.h`,
  `include/app/workspace_file_commands.h`,
  `include/app/workspace_dialog_commands.h`,
  `include/app/workspace_edit_commands.h`,
  `include/app/workspace_clipboard.h`,
  `include/app/workspace_view_commands.h`,
  `src/app/workspace_actions.c`,
  `src/app/workspace_dialog_commands.c`,
  `src/app/workspace_edit_commands.c`,
  `src/app/workspace_file_commands.c`,
  `doc/reference/file-map.md`
- Key changes:
  Moved `WorkspaceActionType`, `WorkspaceServiceType`, and workspace service callback typedefs into `workspace_service_types.h`.
  Kept `workspace.h` as a consumer of those service types for service accessors and callback registration.
  Replaced full Workspace API includes in action, dialog, service, file, edit, clipboard, and view command headers where only `Workspace*` or service/action value types are needed.
  Added explicit implementation includes where previous transitive dependencies hid direct keymap, tool, dialog, or workspace API usage.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split Workspace Layout Types From Workspace API

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_layout_types.h`,
  `include/app/workspace.h`,
  `include/ui/ui_system.h`,
  `src/ui/ui_system_internal.h`,
  `doc/reference/file-map.md`
- Key changes:
  Moved the `WorkspaceLayout` snapshot type into `workspace_layout_types.h`.
  Kept `workspace.h` as a consumer of the layout type for layout accessors.
  Removed the UI system public and private headers' dependency on the full Workspace API when only the layout value type is needed.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split UI Dialog Types From Workspace API

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/ui_dialog_types.h`,
  `include/app/workspace.h`,
  `include/ui/editor_action.h`,
  `include/ui/editor_viewmodel.h`,
  `include/ui/ui_dialog.h`,
  `include/ui/ui_system.h`,
  `doc/reference/file-map.md`
- Key changes:
  Moved shared `UiDialog*` request, kind, result, payload, button, and state types into `ui_dialog_types.h`.
  Kept `workspace.h` as a consumer of those types instead of their owner.
  Removed Workspace API includes from UI action, view-model, and dialog headers where only dialog value types were needed.
  Added an explicit `ui_system.h` dependency for the `WorkspaceLayout` return type that was previously supplied transitively.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Forward Declare ToolContext In Command Entry Headers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/tools/tool.h`,
  `include/app/command_registry.h`,
  `include/app/workspace_tool_commands.h`
- Key changes:
  Made `ToolContext` a named struct type so command execution headers can forward declare it.
  Removed full `tools/tool.h` includes from `command_registry.h` and `workspace_tool_commands.h`.
  Kept the `ToolContext` layout and tool runtime behavior unchanged.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Split Command Types From Execution

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_types.h`,
  `include/app/command_registry.h`,
  `include/app/command_catalog.h`,
  `include/app/command_availability.h`,
  `include/app/workspace_tool_commands.h`,
  `include/ui/editor_action.h`,
  `include/ui/editor_viewmodel.h`,
  `src/input/keymap.c`,
  `src/ui/ui_context_menu.c`,
  `src/app/application.c`,
  `tests/test_registry.c`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Moved `EditorCommand` and `CommandDescriptor` into `command_types.h`.
  Kept `command_registry.h` focused on the execution entry point and `ToolContext` dependency.
  Replaced type-only command-registry includes with `command_types.h` in catalog, availability, UI, keymap, and registry tests.
  Updated the architecture roadmap progress snapshot and file map for the new type boundary.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Route Inspector Toggle Through View Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_view_commands.h`,
  `src/app/workspace_view_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`
- Key changes:
  Added `workspace_view_toggle_inspector()` so all view commands are represented in the workspace view-command module.
  Updated `command_registry_execute()` to delegate `EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR` to the view-command module instead of returning directly.
  Added a UI logic regression assertion for the command route.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Extract Workspace Tool Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_tool_commands.h`,
  `src/app/workspace_tool_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Added a dedicated workspace tool-command module for dynamic tool command activation.
  Updated `command_registry_execute()` to delegate dynamic tool commands to the new module.
  Added UI logic regression coverage for command-driven tool activation.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Extract Workspace Edit Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_edit_commands.h`,
  `src/app/workspace_edit_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Added a dedicated workspace edit-command module for undo, redo, delete selection, select all, and selection pruning after undo/redo.
  Updated `command_registry_execute()` to delegate undo/redo/delete/select-all commands to the new module.
  Added UI logic regression coverage for delete pruning locked-layer selections while preserving undo behavior.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Extract Workspace Dialog Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_dialog_commands.h`,
  `src/app/workspace_dialog_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Added a dedicated workspace dialog-command module for keyboard shortcuts help, About, modal confirm, and modal cancel behavior.
  Updated `command_registry_execute()` to delegate help and modal commands to the new module.
  Added UI logic regression coverage for shortcut dialog toggling, About dialog construction, and unsaved-change modal confirm/cancel routing.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Extract Workspace File Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_file_commands.h`,
  `src/app/workspace_file_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Added a dedicated workspace file-command module for New, Open, Save, Save As, Export PNG, and Exit behavior.
  Updated `command_registry_execute()` to delegate file commands to the new module while keeping the registry as the public execution entry point.
  Added UI logic regression coverage for file commands routing to workspace action callbacks, service callbacks, and Save As dialog construction.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-26 - P2: Remove Command Registry Compatibility Shims

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_registry.h`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `tests/test_registry.c`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Migrated remaining tests from command-registry metadata and availability wrappers to `command_catalog` and `command_availability`.
  Removed the now-unused command-registry descriptor lookup, availability, unavailable-reason, menu availability, and shortcut-format compatibility functions.
  Updated the roadmap to reflect that command metadata and availability callers are now fully separated from command execution.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Migrate Command Metadata Callers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/input/input_router.c`,
  `src/ui/ui_menubar.c`,
  `src/ui/ui_menu_actions.c`,
  `src/app/editor_viewmodel.c`,
  `src/app/command_registry.c`,
  `doc/architecture/refactor-roadmap.md`
- Key changes:
  Updated input routing, menu UI, menu action emission, and view-model construction to query command metadata through `command_catalog` and command state through `command_availability`.
  Updated `command_registry.c` internals to call the extracted catalog/availability modules directly instead of routing through its own compatibility wrappers.
  Refreshed the roadmap pressure points and progress snapshot so it reflects completed workspace, UI frame, view-model, and render descriptor work.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - Architecture Refactor Roadmap Baseline

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `doc/architecture/refactor-roadmap.md`,
  `doc/architecture/overview.md`,
  `doc/README.md`
- Key changes:
  Added a current architecture refactor roadmap that sequences command registry cleanup, action handling consolidation, workspace boundary tightening, UI frame decomposition, view-model cleanup, and render scene descriptor work.
  Linked the roadmap from the architecture overview and documentation index.
- Validation:
  `cmake --build build --parallel` passed before documentation changes.
  `ctest --test-dir build --output-on-failure` passed before documentation changes with 11/11 tests passing.

## 2026-05-25 - P1: Extract Command Catalog Metadata

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_catalog.h`,
  `src/app/command_catalog.c`,
  `src/app/command_registry.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a dedicated command catalog module for stable command descriptors, menu ID lookup, command ID lookup, and dynamic tool command descriptor creation.
  Kept `command_registry_*` metadata lookup functions as compatibility wrappers while command execution and availability remain in `command_registry.c`.
  Added the new catalog source to the `editor_runtime` target and updated the file map.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Command Availability Rules

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/command_availability.h`,
  `src/app/command_availability.c`,
  `src/app/command_registry.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a dedicated command availability module for command executable-state checks, unavailable-reason strings, and menu-backed availability queries.
  Kept the existing `command_registry_is_available()`, `command_registry_unavailable_reason()`, and `command_registry_is_menu_action_available()` functions as compatibility wrappers.
  Reduced `command_registry.c` to command execution plus compatibility lookup/query wrappers for this slice.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Workspace Clipboard Operations

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_clipboard.h`,
  `src/app/workspace_clipboard.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace clipboard module for selection copy, paste, and cut behavior, including paste offset handling and the cut transaction flow.
  Updated `command_registry_execute()` to delegate cut/copy/paste commands to the clipboard module.
  Added UI logic regression coverage for copy/paste/cut behavior and for cut pruning locked-layer selections before deleting editable objects.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extract Workspace View Commands

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_view_commands.h`,
  `src/app/workspace_view_commands.c`,
  `src/app/command_registry.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace view command module for zoom in, zoom out, zoom-to-fit, and grid toggling.
  Updated `command_registry_execute()` to delegate view commands to the new module.
  Added UI logic regression coverage for zoom in/out, zoom-to-fit on an empty document, zoom-to-fit on a small object, and grid toggling.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Introduce Editor Action Handler

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_action_handler.h`,
  `src/app/editor_action_handler.c`,
  `src/app/command_dispatcher.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added an editor action handler module that owns execution of `EditorAction` payloads against a workspace.
  Reduced `command_dispatcher.c` to dispatcher state, null checks, and delegation to the handler.
  Preserved existing command dispatcher API so UI action sinks and tests continue to use the same entry point.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|command|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Introduce Editor Controller Facade

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_controller.h`,
  `src/app/editor_controller.c`,
  `include/app/application_runtime.h`,
  `src/app/application_runtime.c`,
  `src/app/application.c`,
  `tests/test_ui_logic.c`,
  `CMakeLists.txt`,
  `doc/reference/file-map.md`
- Key changes:
  Added a workspace-level editor controller facade for tool context creation, pointer capture checks, pointer event dispatch, scroll handling, pointer-anchor synchronization, tool event construction, and render scene snapshots.
  Migrated `application.c` pointer callbacks and render submission setup away from direct `app->workspace.core.*` / `app->workspace.session.*` access for those paths.
  Added UI logic regression coverage for editor-controller tool event construction and render scene snapshot creation.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Extend Editor Controller Canvas Access

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/editor_controller.h`,
  `src/app/editor_controller.c`,
  `src/app/application_runtime.c`,
  `src/app/application_file_actions.c`,
  `tests/test_ui_logic.c`
- Key changes:
  Added controller APIs for canvas viewport updates, canvas background updates, canvas content-bounds queries, and read-only canvas access for export.
  Migrated resize and PNG export paths away from direct application-layer access to `Workspace` internals for those canvas operations.
  Added UI logic regression coverage for the new canvas controller APIs.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|workspace|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Input Router Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/application.c`,
  `src/input/input_router.c`
- Key changes:
  Updated `input_router.c` to use public workspace accessors for keymap lookup and tool-controller key dispatch instead of including `workspace_internal.h`.
  Added `workspace_set_service_callbacks()` so application startup can register save/load/export/action callbacks without directly writing the internal `workspace.services` fields.
  Replaced the direct service-field assignments in `application.c` with the new workspace registration API.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Command Availability Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/command_availability.c`
- Key changes:
  Added read-only workspace accessors for document, command executor, selection, clipboard count, and service availability checks.
  Updated command availability rules to use the public workspace query surface instead of `workspace_internal.h`.
  Kept the command availability behavior unchanged while moving internal layout knowledge back behind `workspace.c`.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor ViewModel Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/editor_viewmodel.c`
- Key changes:
  Added const workspace accessors for canvas, tool controller, and keymap so read-only UI model building can stay outside the internal workspace layout.
  Updated `editor_viewmodel.c` to build command, tool, property, layer, dialog, and summary views through public workspace accessors instead of `workspace_internal.h`.
  Preserved the existing view-model output shape and UI-facing API while reducing direct `Workspace` field coupling.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor Action Handler Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_action_handler.c`
- Key changes:
  Updated action dispatch to use public workspace accessors for document, command executor, selection, and tool-controller access instead of `workspace_internal.h`.
  Replaced direct dialog-state inspection with `workspace_active_dialog_kind()`.
  Added a local command-execution helper to keep repeated command/executor/document dispatch logic in one place while preserving existing action behavior.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|workspace|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Command Registry Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace.h`,
  `src/app/workspace.c`,
  `src/app/command_registry.c`
- Key changes:
  Added `workspace_execute_service()` so command execution can invoke save/export callbacks without direct access to `workspace.services`.
  Updated command registry selection pruning, delete, select-all, undo/redo, tool activation, Save As path lookup, and shortcut formatting to use public workspace accessors.
  Removed `workspace_internal.h` from `command_registry.c`, leaving the command registry dependent on workspace behavior rather than workspace layout.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "registry|ui_logic|command|workspace"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Remove Editor Controller Internal Workspace Dependency

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_controller.c`
- Key changes:
  Updated the editor controller facade to use public workspace accessors for canvas, document, selection, layout, and tool-controller access.
  Removed `workspace_internal.h` from `editor_controller.c` while keeping the existing controller API and render-scene snapshot behavior unchanged.
  Left internal workspace layout access limited to workspace-owned modules and the application storage struct.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|renderer|workspace|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P1: Make Application Hold an Opaque Workspace

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/app/workspace_service.h`,
  `src/app/workspace_service.c`,
  `src/app/registration_manifest.c`,
  `src/app/application_internal.h`,
  `src/app/application.c`,
  `src/app/application_runtime.c`,
  `src/app/application_file_actions.c`,
  `src/app/application_dialog_actions.c`,
  `tests/test_workspace_service.c`
- Key changes:
  Added `workspace_create()` / `workspace_destroy()` so application code can own `Workspace` through an opaque pointer instead of embedding the internal struct layout.
  Updated `Application` to store `Workspace*`, removing the final app-layer include of `workspace_internal.h` and adjusting application runtime/file/dialog paths to pass the workspace pointer directly.
  Made `app_register_all_manifests()` idempotent so multiple workspace lifetimes in the same process do not fail on duplicate global object/tool registration.
  Added workspace service coverage for opaque workspace allocation and destruction.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "workspace|ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Reuse Command Catalog in ViewModel Construction

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/app/editor_viewmodel.c`
- Key changes:
  Removed the duplicate static command ID/scope mapping from `editor_viewmodel.c`.
  Updated command view construction to read command IDs and key scopes from `command_catalog_find_by_command()`.
  Kept command availability and unavailable-reason lookup behavior unchanged.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Decompose UI Frame Build Helpers

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/ui/ui_runtime.c`
- Key changes:
  Split `ui_system_build()` internals into private helpers for modal state update, frame timing/theme polling, menu/theme request handling, inspector animation/rendering, and canvas-content bounds publishing.
  Kept Nuklear rendering calls, layout fields, theme behavior, context-menu behavior, and published layout semantics unchanged.
  Left helper functions in `ui_runtime.c` for this first pass to reduce risk before moving UI build concerns into separate files.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|command|renderer"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Move UI Frame Build Into Dedicated Module

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `CMakeLists.txt`,
  `src/ui/ui_runtime.c`,
  `src/ui/ui_frame.c`,
  `doc/reference/file-map.md`
- Key changes:
  Moved `ui_system_build()` and its private frame-composition helpers from `ui_runtime.c` into the new `ui_frame.c` module.
  Kept `ui_runtime.c` focused on UI lifecycle, action emission, input forwarding, rendering, and layout query APIs.
  Added `ui_frame.c` to the Nuklear UI target and file map.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "ui_logic|registry|renderer|command"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Introduce Render Scene Descriptor

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `include/render/render_system.h`,
  `src/render/render_system.c`,
  `src/app/application.c`,
  `tests/test_renderer.c`
- Key changes:
  Added `RenderSceneDesc` as the render-system input snapshot for document, selection, canvas, selection preview, and overlay object state.
  Replaced the long `render_system_draw()` parameter list with a single scene descriptor pointer.
  Updated application frame submission and renderer regression tests to pass the descriptor.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "renderer|ui_logic|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-25 - P2: Extract Render Scene Cache Key

- Branch:
  `refactor-editor-architecture-roadmap`
- Modified files:
  `src/render/render_system.c`
- Key changes:
  Replaced the individual cached document/selection/canvas fields with a `RenderSceneCacheKey` value.
  Added helpers to build and compare cache keys from `RenderSceneDesc`.
  Kept draw-list invalidation behavior unchanged while making the cache comparison path easier to audit.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R "renderer|ui_logic|registry"` passed.
  `ctest --test-dir build --output-on-failure` passed with 11/11 tests passing.

## 2026-05-12 21:25:51 CST - Baseline Check

- Build system: CMake (`CMakeLists.txt` at repository root).
- Source layout: `src/`, `include/`, `doc/`, and existing `build/` directory match the proposed plan.
- Baseline validation: `cmake --build build --parallel` completed successfully before refactor work started.

## 2026-05-12 21:29:13 CST - P0: Fix RenderSystem Selection Cache Key

- Modified files:
  `include/model/selection.h`,
  `src/model/selection.c`,
  `src/render/render_system.c`,
  `tests/test_selection.c`,
  `tests/test_renderer.c`
- Key changes:
  Added `SelectionSet.revision` and incremented it only when the selected ID set actually changes.
  Extended `RenderSystem` cache invalidation to depend on both `selection_count` and `selection_revision`.
  Added regression coverage for stable revisions on no-op updates and for render invalidation when the selection content changes without changing selection count.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'selection|renderer'` passed.

## 2026-05-12 21:39:06 CST - P0: Harden Lua Script Runtime Boundary

- Modified files:
  `src/script/script_runtime_lua.c`,
  `scripts/star_wreath.lua`,
  `tests/test_script_runtime.c`,
  `CMakeLists.txt`
- Key changes:
  Replaced per-event `luaL_dofile()` execution with a one-time script load path that caches the compiled script per runtime instance.
  Moved script access onto a whitelist-style `gldraw` table (`document_add_object`, `undo`, `redo`, `selection_get_ids`) inside a dedicated environment instead of exposing command text execution and unrestricted globals.
  Limited the available standard libraries to base helpers plus `math`, `string`, and `table`, and added a scripting-only regression test target that checks safe-library exposure and one-time loading behavior.
- Validation:
  `cmake --build build --parallel` passed for the default configuration (`GLDRAW_ENABLE_SCRIPTING=OFF`).
  `ctest --test-dir build --output-on-failure -R 'selection|renderer'` passed after the P0 changes.
  `cmake -S . -B build-scripting -DGLDRAW_ENABLE_SCRIPTING=ON -DCMAKE_BUILD_TYPE=Release` could not complete on this machine because CMake could not find `LUA_LIBRARIES` / `LUA_INCLUDE_DIR`, so the scripting-enabled branch could not be compiled locally.

## 2026-05-12 21:54:08 CST - P1: Split Public Workspace API From Internal State

- Modified files:
  `include/app/workspace.h`,
  `include/app/workspace_internal.h`,
  `src/app/workspace.c`,
  `src/app/application_internal.h`,
  `src/app/application_file_actions.c`,
  `src/app/editor_viewmodel.c`,
  `src/app/workspace_service.c`,
  `src/app/workspace_actions.c`,
  `src/app/workspace_dialogs.c`,
  `src/app/command_dispatcher.c`,
  `src/app/command_registry.c`,
  `src/input/input_router.c`,
  `src/tools/script_tool_lua.c`,
  `src/tools/tool_select.c`,
  `src/tools/tool_shape.c`,
  `tests/test_registry.c`,
  `tests/test_ui_logic.c`,
  `tests/test_workspace_service.c`,
  `CMakeLists.txt`
- Key changes:
  Moved the concrete `Workspace`/`EditorCore`/`EditorSession`/`EditorServices` layout into `workspace_internal.h` and reduced the public `workspace.h` surface to public enums, dialog/layout types, and opaque-pointer API declarations.
  Replaced the old public inline implementations with compiled functions in `src/app/workspace.c`, including the existing status/layout/dirty/clipboard helpers plus new accessor entry points for core editor services.
  Updated internal runtime modules and tests that need concrete workspace storage or direct field access to include the internal header explicitly, removing accidental dependence on the public header layout.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'workspace|ui_logic|registry|selection|renderer'` passed.

## 2026-05-12 22:09:20 CST - P1: Replace ToolContext Workspace Access With Ports

- Modified files:
  `include/tools/tool.h`,
  `include/script/script_runtime.h`,
  `src/app/workspace.c`,
  `src/tools/tool_select.c`,
  `src/tools/tool_shape.c`,
  `src/tools/script_tool_lua.c`,
  `src/script/script_runtime_lua.c`,
  `tests/test_ui_logic.c`,
  `tests/test_script_runtime.c`
- Key changes:
  Added `ToolPorts` to `ToolContext` and introduced helper wrappers for command execution, undo/redo, selection preview updates, and dirty-flag sync so tools no longer require direct `Workspace*` access for runtime mutations.
  Wired `workspace_tool_context()` to publish concrete port implementations backed by the workspace command executor and session preview/dirty state.
  Updated built-in select and shape tools plus the optional Lua-backed script tool/runtime to use the new port callbacks instead of reaching into `Workspace` internals or `CommandExecutor` directly.
  Updated tests to build contexts through `workspace_tool_context()` and to provide explicit test ports for script runtime execution.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'ui_logic|workspace|registry|selection|renderer|command'` passed.
  The scripting-enabled build still could not be compiled locally because this machine does not have the required Lua development package discoverable by CMake.

## 2026-05-12 22:32:08 CST - P1: Seal Platform Window Leakage Behind Callbacks and Adapters

- Modified files:
  `include/platform/window.h`,
  `src/platform/window.c`,
  `src/platform/window_internal.h`,
  `src/app/application.c`,
  `src/ui/ui_runtime.c`,
  `src/ui/ui_system_internal.h`
- Key changes:
  Added platform-owned window event callback registration APIs plus wrappers for window sizing, close requests, timed waits, and shared time queries so application runtime code no longer calls GLFW window functions directly.
  Moved GLFW callback dispatch into the platform layer and backed it with an internal native-window registry, removing the need for application/user-pointer ownership on `GLFWwindow`.
  Added a small platform-owned Nuklear/GLFW adapter surface so `ui_runtime.c` can initialize, frame, render, and shut down Nuklear without including `window_internal.h` or touching raw `GLFWwindow*`.
  Updated application startup and loop code to register platform callbacks and use platform wrappers for framebuffer polling and close control.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure -R 'ui_logic|workspace|registry|selection|renderer|command'` passed.

## 2026-05-12 22:42:37 CST - P1: Reorganize CMake Targets Around Runtime Boundaries

- Modified files:
  `CMakeLists.txt`
- Key changes:
  Replaced the old coarse library split (`gldraw_model`, `gldraw_render_gl`, `gldraw_commands`, `gldraw_ui_nuklear`, `gldraw_app`) with a layered target graph centered on `editor_model`, `editor_commands`, `editor_tools`, `editor_runtime`, `render_core`, `render_glfw_gl`, `platform_glfw`, `ui_nuklear_glfw`, and `editor_app`.
  Moved command core, tool runtime, render core, GL backend, platform window implementation, UI runtime, and application shell sources into separate targets so their responsibilities match the architecture more closely and command/tool/runtime cycles are broken explicitly in the link graph.
  Updated executable and test link dependencies to target the new layer boundaries, while keeping compatibility aliases for the previous `gldraw_*` library names.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:09:14 CST - P2: Introduce Render Resource Management Layer

- Modified files:
  `include/render/buffer_pool.h`,
  `include/render/shader_manager.h`,
  `src/render/buffer_pool.c`,
  `src/render/render_device_gl.c`,
  `src/render/shader_manager.c`,
  `CMakeLists.txt`
- Key changes:
  Added a `ShaderManager` module that centralizes shader source loading, shader compilation, program linking, uniform lookup, and program destruction instead of keeping that lifecycle embedded in `render_device_gl.c`.
  Added a `BufferPool` module that owns GL vertex stream creation, attribute layout setup, buffer growth, and VAO/VBO teardown so transient backend resources are managed through one resource layer.
  Updated `render_device_gl.c` to consume the new shader and buffer managers, reducing the backend file to frame state, transform logic, clip handling, and draw submission orchestration.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:27:42 CST - P2: Replace Draw-List Hot-Path Heap Churn With Reusable Arena Storage

- Modified files:
  `include/render/render_arena.h`,
  `src/render/render_arena.c`,
  `include/render/canvas_drawlist.h`,
  `src/render/canvas_drawlist.c`,
  `tests/test_renderer.c`,
  `CMakeLists.txt`
- Key changes:
  Added a reusable `RenderArena` allocator module with explicit init/reset/shutdown/reserve/alloc operations so temporary render-build storage lives behind a dedicated abstraction instead of ad hoc per-call heap usage.
  Updated `CanvasDrawList` to own a build-scoped scratch arena and reset it once per build, making temporary object path buffers, grid geometry buffers, and visible-index arrays reuse retained memory across frames.
  Removed the hot-path `malloc/free` pairs from `canvas_drawlist_build()` and its helper paths, while keeping the externally visible draw-list output arrays and submission flow unchanged.
  Added a renderer regression test that verifies the draw-list scratch arena remains allocated and reusable across repeated builds.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:46:18 CST - P2: Raise Render Abstraction Around Pass, Material, and Geometry

- Modified files:
  `include/render/render_device.h`,
  `src/render/render_device.c`,
  `include/render/canvas_drawlist.h`,
  `src/render/canvas_drawlist.c`,
  `src/render/render_device_gl.c`,
  `src/render/canvas_renderer.c`,
  `tests/test_renderer.c`
- Key changes:
  Replaced the old immediate-mode render-device surface (`set_color`, `draw_path`, `draw_rect`, `draw_line`, mutable transform/clip state) with explicit `RenderPass`, `RenderMaterial`, and `RenderGeometry` value types.
  Updated `canvas_renderer_submit()` to build a pass object once per frame submission and then submit each stroke as a geometry/material pair, making the renderer intent explicit and reducing hidden backend state transitions.
  Simplified the GL backend so pass setup owns clip and transform state, while draw submission consumes geometry and material payloads directly instead of relying on prior setter calls.
  Updated renderer tests to validate the new frame/pass/draw submission sequence and geometry metadata capture.
- Validation:
  `cmake --build build --parallel` passed.
  `ctest --test-dir build --output-on-failure` passed.

## 2026-05-12 23:54:03 CST - P3: Future Architecture Evolution Suggestions

- Render graph direction:
  If GLDraw grows beyond a single canvas line-render pass, evolve the current frame/pass model into a render graph so offscreen composition, post-processing, and dependency ordering are explicit instead of being encoded inside backend control flow.
- Backend portability direction:
  Split backend-facing rendering into a `RenderBackend` layer plus a separate context/provider abstraction so OpenGL-specific context creation assumptions stop leaking into the rendering lifecycle and alternate backends can be introduced incrementally.
- Editor ecosystem direction:
  Keep the current static manifest model unless there is a real plugin-distribution goal; if that goal appears later, introduce a narrow plugin ABI centered on registration APIs rather than exposing workspace or renderer internals directly.
