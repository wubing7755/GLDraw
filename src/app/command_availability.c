/**
 * @file command_availability.c
 * @brief Command availability rules for workspace state.
 */
#include <app/command_availability.h>

#include <app/command_catalog.h>
#include <app/workspace.h>
#include <commands/command.h>
#include <document/document.h>

#include "command_definitions.h"

typedef struct CommandAvailabilityDef {
    EditorCommand command;
    CommandAvailabilityRule rule;
    WorkspaceServiceType service;
} CommandAvailabilityDef;

static const CommandAvailabilityDef COMMAND_AVAILABILITY_DEFS[] = {
#define GLDRAW_COMMAND_AVAILABILITY(command, id, label, scope, menu_id, tool_id, availability, service, execute) \
    {command, availability, service},
    GLDRAW_STABLE_COMMANDS(GLDRAW_COMMAND_AVAILABILITY)
#undef GLDRAW_COMMAND_AVAILABILITY
};

static const CommandAvailabilityDef* command_availability_find_def(
    EditorCommand command)
{
    int i = 0;

    for (i = 0; i < (int)(sizeof(COMMAND_AVAILABILITY_DEFS) /
                          sizeof(COMMAND_AVAILABILITY_DEFS[0])); ++i) {
        if (COMMAND_AVAILABILITY_DEFS[i].command == command) {
            return &COMMAND_AVAILABILITY_DEFS[i];
        }
    }

    return NULL;
}

static int command_availability_editable_selection_count(const Workspace* workspace)
{
    const Document* document = workspace_get_document_const(workspace);
    const SelectionSet* selection = workspace_get_selection_const(workspace);
    int editable_count = 0;
    int i = 0;

    if (!document || !selection) {
        return 0;
    }

    for (i = 0; i < selection->count; ++i) {
        ObjectId id = selection->ids[i];
        if (document_find_object(document, id) &&
            !document_object_is_locked(document, id)) {
            editable_count++;
        }
    }

    return editable_count;
}

static int command_availability_active_layer_editable(const Workspace* workspace)
{
    const Document* document = workspace_get_document_const(workspace);

    if (!document) {
        return 0;
    }

    return !document_layer_is_locked(document, document_active_layer_id(document));
}

static int command_availability_tool_is_available(const Workspace* workspace,
                                                  EditorCommand command)
{
    const ToolDescriptor* tool = command_catalog_tool_for_command(command, NULL);

    if (!tool || !tool->id) {
        return 0;
    }
    if (!tool->requires_editable_layer) {
        return workspace != NULL;
    }

    return workspace && command_availability_active_layer_editable(workspace);
}

static const char* command_availability_selection_unavailable_reason(const Workspace* workspace)
{
    const SelectionSet* selection = workspace_get_selection_const(workspace);

    if (!selection || selection->count <= 0) {
        return "Selection is empty.";
    }

    if (command_availability_editable_selection_count(workspace) <= 0) {
        return "Selection is on locked layers.";
    }

    return "";
}

int command_availability_is_available(const Workspace* workspace,
                                      EditorCommand command)
{
    const CommandAvailabilityDef* def = command_availability_find_def(command);
    const CommandExecutor* commands = workspace_get_command_executor_const(workspace);
    const SelectionSet* selection = workspace_get_selection_const(workspace);

    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        return command_availability_tool_is_available(workspace, command);
    }
    if (!def) {
        return 0;
    }

    switch (def->rule) {
    case COMMAND_AVAILABILITY_ALWAYS:
        return 1;
    case COMMAND_AVAILABILITY_WORKSPACE:
        return workspace != NULL;
    case COMMAND_AVAILABILITY_SERVICE:
        return workspace_service_available(workspace, def->service);
    case COMMAND_AVAILABILITY_UNDO:
        return commands && command_executor_can_undo(commands);
    case COMMAND_AVAILABILITY_REDO:
        return commands && command_executor_can_redo(commands);
    case COMMAND_AVAILABILITY_EDITABLE_SELECTION:
        return workspace && command_availability_editable_selection_count(workspace) > 0;
    case COMMAND_AVAILABILITY_SELECTION:
        return selection && selection->count > 0;
    case COMMAND_AVAILABILITY_PASTE:
        return workspace &&
               workspace_clipboard_count(workspace) > 0 &&
               command_availability_active_layer_editable(workspace);
    case COMMAND_AVAILABILITY_NEVER:
    default:
        return 0;
    }
}

const char* command_availability_unavailable_reason(const Workspace* workspace,
                                                    EditorCommand command)
{
    const SelectionSet* selection = workspace_get_selection_const(workspace);

    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        const ToolDescriptor* tool = command_catalog_tool_for_command(command, NULL);
        if (!tool) {
            return "";
        }
        if (!command_availability_tool_is_available(workspace, command) &&
            tool->requires_editable_layer) {
            return "Active layer is locked.";
        }
        return "";
    }

    if (command_availability_is_available(workspace, command)) {
        return "";
    }

    switch (command) {
    case EDITOR_COMMAND_EDIT_CUT:
    case EDITOR_COMMAND_EDIT_DELETE:
        return command_availability_selection_unavailable_reason(workspace);
    case EDITOR_COMMAND_EDIT_COPY:
        return (!selection || selection->count <= 0)
                   ? "Selection is empty."
                   : "";
    case EDITOR_COMMAND_EDIT_PASTE:
        if (!workspace) {
            return "Workspace is unavailable.";
        }
        if (!command_availability_active_layer_editable(workspace)) {
            return "Active layer is locked.";
        }
        return "";
    case EDITOR_COMMAND_EDIT_UNDO:
        return "Nothing to undo.";
    case EDITOR_COMMAND_EDIT_REDO:
        return "Nothing to redo.";
    default:
        return "";
    }
}

int command_availability_is_menu_action_available(const Workspace* workspace, int id)
{
    const CommandDescriptor* descriptor = command_catalog_find_by_menu_id(id);

    if (!descriptor) {
        return 0;
    }

    return command_availability_is_available(workspace, descriptor->command);
}
