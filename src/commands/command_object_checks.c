#include "command_internal.h"

static CommandExecuteCheck create_object_check(const Document* document,
                                               const CreateObjectCommand* command)
{
    LayerId layer_id = 0u;

    if (!document || !command || !command->object_snapshot) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }

    layer_id = command->object_snapshot->layer_id;
    if (layer_id == 0u) {
        layer_id = document_active_layer_id(document);
    }

    if (document_layer_is_locked(document, layer_id)) {
        return COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED;
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck delete_selection_check(const Document* document,
                                                  const DeleteSelectionCommand* command)
{
    int i = 0;

    if (!document || !command || command->object_count <= 0) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }

    for (i = 0; i < command->object_count; ++i) {
        const GraphicObject* object = command->objects[i];
        if (!object) {
            return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
        }
        if (!document_find_object(document, object->id)) {
            return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
        }
        if (document_object_is_locked(document, object->id)) {
            return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
        }
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck move_objects_check(const Document* document,
                                              const MoveObjectsCommand* command)
{
    int i = 0;

    if (!document || !command || command->object_count <= 0) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }

    for (i = 0; i < command->object_count; ++i) {
        if (!document_find_object(document, command->object_ids[i])) {
            return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
        }
        if (document_object_is_locked(document, command->object_ids[i])) {
            return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
        }
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck paste_objects_check(const Document* document,
                                               const PasteObjectsCommand* command)
{
    int i = 0;

    if (!document || !command || command->object_count <= 0 || command->layer_id == 0u) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }
    if (document_layer_is_locked(document, command->layer_id)) {
        return COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED;
    }

    for (i = 0; i < command->object_count; ++i) {
        if (!command->object_snapshots[i] || command->object_ids[i] == 0u) {
            return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
        }
        if (document_find_object(document, command->object_ids[i])) {
            return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
        }
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck set_property_check(const Document* document,
                                              const SetPropertyCommand* command)
{
    if (!document || !command) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }

    if (!document_find_object(document, command->object_id)) {
        return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
    }
    if (document_object_is_locked(document, command->object_id)) {
        return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

CommandExecuteCheck command_check_execute(const Command* command,
                                          const Document* document)
{
    if (!command || !command->vtable || !document) {
        return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }

    if (command->vtable == &CREATE_OBJECT_VTABLE) {
        return create_object_check(document, (const CreateObjectCommand*)command);
    }
    if (command->vtable == &DELETE_SELECTION_VTABLE) {
        return delete_selection_check(document, (const DeleteSelectionCommand*)command);
    }
    if (command->vtable == &MOVE_OBJECTS_VTABLE) {
        return move_objects_check(document, (const MoveObjectsCommand*)command);
    }
    if (command->vtable == &PASTE_OBJECTS_VTABLE) {
        return paste_objects_check(document, (const PasteObjectsCommand*)command);
    }
    if (command->vtable == &SET_PROPERTY_VTABLE) {
        return set_property_check(document, (const SetPropertyCommand*)command);
    }

    return COMMAND_EXECUTE_CHECK_OK;
}

int command_can_execute(const Command* command, const Document* document)
{
    return command_check_execute(command, document) == COMMAND_EXECUTE_CHECK_OK;
}

const char* command_execute_check_message(CommandExecuteCheck check)
{
    switch (check) {
    case COMMAND_EXECUTE_CHECK_OK:
        return "Command can execute.";
    case COMMAND_EXECUTE_CHECK_TARGET_MISSING:
        return "Target object is missing.";
    case COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED:
        return "Target object is on a locked layer.";
    case COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED:
        return "Active layer is locked.";
    case COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT:
    default:
        return "Command is invalid in the current context.";
    }
}
