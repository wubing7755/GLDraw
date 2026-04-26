/**
 * @file workspace_dialogs.c
 * @brief Reusable workspace-owned dialog builders and lifecycle helpers.
 */
#include <app/workspace_dialogs.h>

#include <stdio.h>
#include <string.h>

static int workspace_dialog_resolve_confirm_unsaved(Workspace* workspace,
                                                    UiDialogResult result);
static int workspace_dialog_resolve_shortcuts(Workspace* workspace,
                                              UiDialogResult result);
static int workspace_dialog_resolve_info(Workspace* workspace,
                                         UiDialogResult result);

static const UiDialogButtonDefinition UI_DIALOG_BUTTONS_CONFIRM_UNSAVED[] = {
    {"Save", UI_DIALOG_RESULT_PRIMARY, 1},
    {"Don't Save", UI_DIALOG_RESULT_SECONDARY, 0},
    {"Cancel", UI_DIALOG_RESULT_CANCEL, 0}
};

static const UiDialogDefinition UI_DIALOG_DEFINITION_CONFIRM_UNSAVED = {
    {
        UI_DIALOG_CONFIRM_UNSAVED,
        "Unsaved Changes",
        "You have unsaved changes.\nDo you want to save before continuing?",
        360.0f,
        170.0f,
        1
    },
    UI_DIALOG_BUTTONS_CONFIRM_UNSAVED,
    (int)(sizeof(UI_DIALOG_BUTTONS_CONFIRM_UNSAVED) / sizeof(UI_DIALOG_BUTTONS_CONFIRM_UNSAVED[0])),
    workspace_dialog_resolve_confirm_unsaved
};

static const UiDialogButtonDefinition UI_DIALOG_BUTTONS_SHORTCUTS[] = {
    {"Close", UI_DIALOG_RESULT_CANCEL, 1}
};

static const UiDialogDefinition UI_DIALOG_DEFINITION_SHORTCUTS = {
    {
        UI_DIALOG_SHORTCUTS,
        "Keyboard Shortcuts",
        "",
        520.0f,
        420.0f,
        1
    },
    UI_DIALOG_BUTTONS_SHORTCUTS,
    (int)(sizeof(UI_DIALOG_BUTTONS_SHORTCUTS) / sizeof(UI_DIALOG_BUTTONS_SHORTCUTS[0])),
    workspace_dialog_resolve_shortcuts
};

static const UiDialogButtonDefinition UI_DIALOG_BUTTONS_INFO[] = {
    {"OK", UI_DIALOG_RESULT_PRIMARY, 1}
};

static const UiDialogDefinition UI_DIALOG_DEFINITION_INFO = {
    {
        UI_DIALOG_INFO,
        "Information",
        "",
        420.0f,
        190.0f,
        1
    },
    UI_DIALOG_BUTTONS_INFO,
    (int)(sizeof(UI_DIALOG_BUTTONS_INFO) / sizeof(UI_DIALOG_BUTTONS_INFO[0])),
    workspace_dialog_resolve_info
};

static int workspace_dialog_execute_action_now(Workspace* workspace,
                                               WorkspaceActionType action)
{
    if (!workspace || action == WORKSPACE_ACTION_NONE || !workspace->services.execute_action) {
        return 0;
    }

    return workspace->services.execute_action(workspace,
                                              action,
                                              workspace->services.command_user_data);
}

static int workspace_dialog_resolve_confirm_unsaved(Workspace* workspace,
                                                    UiDialogResult result)
{
    WorkspaceActionType action = WORKSPACE_ACTION_NONE;

    if (!workspace) {
        return 0;
    }

    action = (WorkspaceActionType)workspace->session.active_dialog.payload.int_values[0];

    switch (result) {
    case UI_DIALOG_RESULT_PRIMARY:
        if (!workspace->services.save_document ||
            !workspace->services.save_document(workspace, workspace->services.command_user_data)) {
            return 0;
        }
        workspace_dialog_close(workspace);
        return workspace_dialog_execute_action_now(workspace, action);
    case UI_DIALOG_RESULT_SECONDARY:
        workspace_dialog_close(workspace);
        return workspace_dialog_execute_action_now(workspace, action);
    case UI_DIALOG_RESULT_CANCEL:
        workspace_dialog_close(workspace);
        snprintf(workspace->session.status_message,
                 sizeof(workspace->session.status_message),
                 "Action cancelled.");
        return 1;
    case UI_DIALOG_RESULT_NONE:
    default:
        return 0;
    }
}

static int workspace_dialog_resolve_shortcuts(Workspace* workspace,
                                              UiDialogResult result)
{
    if (!workspace) {
        return 0;
    }

    switch (result) {
    case UI_DIALOG_RESULT_PRIMARY:
    case UI_DIALOG_RESULT_CANCEL:
        workspace_dialog_close(workspace);
        return 1;
    case UI_DIALOG_RESULT_NONE:
    case UI_DIALOG_RESULT_SECONDARY:
    default:
        return 0;
    }
}

static int workspace_dialog_resolve_info(Workspace* workspace,
                                         UiDialogResult result)
{
    if (!workspace) {
        return 0;
    }

    switch (result) {
    case UI_DIALOG_RESULT_PRIMARY:
    case UI_DIALOG_RESULT_CANCEL:
        workspace_dialog_close(workspace);
        return 1;
    case UI_DIALOG_RESULT_NONE:
    case UI_DIALOG_RESULT_SECONDARY:
    default:
        return 0;
    }
}

const UiDialogDefinition* workspace_dialog_definition(UiDialogKind kind)
{
    switch (kind) {
    case UI_DIALOG_CONFIRM_UNSAVED:
        return &UI_DIALOG_DEFINITION_CONFIRM_UNSAVED;
    case UI_DIALOG_SHORTCUTS:
        return &UI_DIALOG_DEFINITION_SHORTCUTS;
    case UI_DIALOG_INFO:
        return &UI_DIALOG_DEFINITION_INFO;
    case UI_DIALOG_NONE:
    default:
        return NULL;
    }
}

void workspace_dialog_reset(UiDialogState* dialog)
{
    if (!dialog) {
        return;
    }

    memset(dialog, 0, sizeof(*dialog));
    dialog->kind = UI_DIALOG_NONE;
}

void workspace_dialog_apply_template(UiDialogState* dialog, const UiDialogTemplate* dialog_template)
{
    if (!dialog || !dialog_template) {
        return;
    }

    workspace_dialog_reset(dialog);
    dialog->kind = dialog_template->kind;
    dialog->width = dialog_template->width;
    dialog->height = dialog_template->height;
    dialog->modal = dialog_template->modal;

    if (dialog_template->title) {
        snprintf(dialog->title, sizeof(dialog->title), "%s", dialog_template->title);
    }
    if (dialog_template->message) {
        snprintf(dialog->message, sizeof(dialog->message), "%s", dialog_template->message);
    }
}

int workspace_dialog_add_button(UiDialogState* dialog, const UiDialogButtonDefinition* button)
{
    int index = 0;

    if (!dialog || !button || !button->label) {
        return 0;
    }

    index = dialog->button_count;
    if (index < 0 || index >= (int)(sizeof(dialog->buttons) / sizeof(dialog->buttons[0]))) {
        return 0;
    }

    snprintf(dialog->buttons[index].label,
             sizeof(dialog->buttons[index].label),
             "%s",
             button->label);
    dialog->buttons[index].result = button->result;
    dialog->buttons[index].is_default = button->is_default;
    dialog->button_count += 1;
    return 1;
}

int workspace_dialog_open_from_template(Workspace* workspace,
                                        const UiDialogTemplate* dialog_template,
                                        const UiDialogPayload* payload,
                                        const UiDialogButtonDefinition* buttons,
                                        int button_count)
{
    UiDialogState dialog;
    int i = 0;

    if (!workspace || !dialog_template || !buttons || button_count <= 0) {
        return 0;
    }

    workspace_dialog_apply_template(&dialog, dialog_template);
    if (payload) {
        dialog.payload = *payload;
    }

    for (i = 0; i < button_count; ++i) {
        if (!workspace_dialog_add_button(&dialog, &buttons[i])) {
            return 0;
        }
    }

    return workspace_dialog_open(workspace, &dialog);
}

int workspace_dialog_open_definition(Workspace* workspace,
                                     const UiDialogDefinition* definition,
                                     const UiDialogPayload* payload)
{
    if (!definition) {
        return 0;
    }

    return workspace_dialog_open_from_template(workspace,
                                               &definition->dialog_template,
                                               payload,
                                               definition->buttons,
                                               definition->button_count);
}

int workspace_dialog_open(Workspace* workspace, const UiDialogState* dialog)
{
    if (!workspace || !dialog || dialog->kind == UI_DIALOG_NONE) {
        return 0;
    }

    workspace->session.active_request_type = UI_REQUEST_DIALOG;
    workspace->session.active_dialog = *dialog;
    return 1;
}

void workspace_dialog_close(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->session.active_request_type = UI_REQUEST_NONE;
    workspace_dialog_reset(&workspace->session.active_dialog);
}

int workspace_dialog_resolve(Workspace* workspace, UiDialogResult result)
{
    const UiDialogDefinition* definition = NULL;

    if (!workspace || workspace->session.active_request_type != UI_REQUEST_DIALOG) {
        return 0;
    }

    definition = workspace_dialog_definition(workspace->session.active_dialog.kind);
    if (!definition || !definition->resolve) {
        return 0;
    }

    return definition->resolve(workspace, result);
}

int workspace_dialog_open_confirm_unsaved(Workspace* workspace, WorkspaceActionType pending_action)
{
    UiDialogPayload payload;

    if (!workspace || pending_action == WORKSPACE_ACTION_NONE) {
        return 0;
    }

    memset(&payload, 0, sizeof(payload));
    payload.int_values[0] = (int)pending_action;
    return workspace_dialog_open_definition(workspace,
                                            &UI_DIALOG_DEFINITION_CONFIRM_UNSAVED,
                                            &payload);
}

int workspace_dialog_open_shortcuts(Workspace* workspace, const char* content_text)
{
    UiDialogState dialog;

    if (!workspace || !content_text) {
        return 0;
    }

    workspace_dialog_apply_template(&dialog,
                                    &UI_DIALOG_DEFINITION_SHORTCUTS.dialog_template);
    snprintf(dialog.message, sizeof(dialog.message), "%s", content_text);
    return workspace_dialog_add_button(&dialog, &UI_DIALOG_BUTTONS_SHORTCUTS[0]) &&
           workspace_dialog_open(workspace, &dialog);
}

int workspace_dialog_open_info(Workspace* workspace,
                               const char* title,
                               const char* message)
{
    UiDialogState dialog;

    if (!workspace || !title || !message) {
        return 0;
    }

    workspace_dialog_apply_template(&dialog,
                                    &UI_DIALOG_DEFINITION_INFO.dialog_template);
    snprintf(dialog.title, sizeof(dialog.title), "%s", title);
    snprintf(dialog.message, sizeof(dialog.message), "%s", message);
    return workspace_dialog_add_button(&dialog, &UI_DIALOG_BUTTONS_INFO[0]) &&
           workspace_dialog_open(workspace, &dialog);
}
