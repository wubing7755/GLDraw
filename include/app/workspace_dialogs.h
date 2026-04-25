/**
 * @file workspace_dialogs.h
 * @brief Reusable workspace-owned dialog builders and lifecycle helpers.
 *
 * Role in project:
 * - Centralizes generic dialog-state construction and workspace dialog ownership.
 * - Provides reusable presets so business modules do not handcraft `UiDialogState`.
 *
 * Module relationships:
 * - Called by workspace action flows and future business modules that need dialogs.
 * - Consumed by UI rendering through `Workspace.active_dialog`.
 */
#ifndef GLDRAW_APP_WORKSPACE_DIALOGS_H
#define GLDRAW_APP_WORKSPACE_DIALOGS_H

#include <app/workspace.h>

/** Immutable dialog shell configuration used by generic builders. */
typedef struct UiDialogTemplate {
    UiDialogKind kind;
    const char* title;
    const char* message;
    float width;
    float height;
    int modal;
} UiDialogTemplate;

/** Immutable button definition used when assembling dialog presets. */
typedef struct UiDialogButtonDefinition {
    const char* label;
    UiDialogResult result;
    int is_default;
} UiDialogButtonDefinition;

struct Workspace;
/** Dialog-specific result handler signature used by registered definitions. */
typedef int (*UiDialogResolveFn)(struct Workspace* workspace, UiDialogResult result);

/** Complete registered dialog definition: template, buttons, and resolver. */
typedef struct UiDialogDefinition {
    UiDialogTemplate dialog_template;
    const UiDialogButtonDefinition* buttons;
    int button_count;
    UiDialogResolveFn resolve;
} UiDialogDefinition;

/** Reset one dialog state to an inactive default value. */
void workspace_dialog_reset(UiDialogState* dialog);
/** Apply a generic template to a dialog state. */
void workspace_dialog_apply_template(UiDialogState* dialog, const UiDialogTemplate* dialog_template);
/** Append one button definition when capacity allows. Returns non-zero on success. */
int workspace_dialog_add_button(UiDialogState* dialog, const UiDialogButtonDefinition* button);
/** Look up one registered dialog definition by kind. */
const UiDialogDefinition* workspace_dialog_definition(UiDialogKind kind);
/** Open one dialog from a template, payload, and button definitions. */
int workspace_dialog_open_from_template(Workspace* workspace,
                                        const UiDialogTemplate* dialog_template,
                                        const UiDialogPayload* payload,
                                        const UiDialogButtonDefinition* buttons,
                                        int button_count);
/** Open one registered dialog definition with the supplied payload. */
int workspace_dialog_open_definition(Workspace* workspace,
                                     const UiDialogDefinition* definition,
                                     const UiDialogPayload* payload);
/** Open one fully prepared dialog as the workspace-owned active request. */
int workspace_dialog_open(Workspace* workspace, const UiDialogState* dialog);
/** Close the workspace-owned active dialog request. */
void workspace_dialog_close(Workspace* workspace);
/** Resolve the active dialog according to its kind and the returned result. */
int workspace_dialog_resolve(Workspace* workspace, UiDialogResult result);

/** Open the standard unsaved-changes confirmation dialog. */
int workspace_dialog_open_confirm_unsaved(Workspace* workspace, WorkspaceActionType pending_action);
/** Open the keyboard shortcuts help dialog using preformatted content text. */
int workspace_dialog_open_shortcuts(Workspace* workspace, const char* content_text);

#endif /* GLDRAW_APP_WORKSPACE_DIALOGS_H */
