#include "command_internal.h"

#include <base/log.h>
#include <base/math2d.h>

#include <stdio.h>
#include <string.h>

static GraphicObject* document_find_for_command(Document* document, ObjectId id)
{
    return document_find_object(document, id);
}

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

static int create_object_execute(void* cmd, Document* document)
{
    CreateObjectCommand* command = (CreateObjectCommand*)cmd;
    GraphicObject* clone = NULL;

    if (!command || !document || !command->object_snapshot) {
        return 0;
    }

    clone = object_clone(command->object_snapshot);
    if (!clone) {
        return 0;
    }

    if (command->object_id != 0u) {
        if (!document_append_object_with_id_to_layer(document, clone, command->object_id,
                                                     clone->layer_id)) {
            object_destroy(clone);
            return 0;
        }
    } else {
        if (!document_add_object_to_layer(document, clone, clone->layer_id)) {
            object_destroy(clone);
            return 0;
        }
        command->object_id = clone->id;
        command->object_snapshot->id = clone->id;
    }

    return 1;
}

static int create_object_undo(void* cmd, Document* document)
{
    CreateObjectCommand* command = (CreateObjectCommand*)cmd;

    if (!command || !document || command->object_id == 0u) {
        return 0;
    }

    return document_remove_object(document, command->object_id);
}

static int create_object_merge(void* cmd, const void* next)
{
    (void)cmd;
    (void)next;
    return 0;
}

static void create_object_destroy(void* cmd)
{
    CreateObjectCommand* command = (CreateObjectCommand*)cmd;

    if (!command) {
        return;
    }

    object_destroy(command->object_snapshot);
    g_allocator.free_fn(command);
}

const CommandVTable CREATE_OBJECT_VTABLE = {create_object_execute, create_object_undo,
                                            create_object_execute, create_object_merge,
                                            create_object_destroy};

static int delete_selection_execute(void* cmd, Document* document)
{
    DeleteSelectionCommand* command = (DeleteSelectionCommand*)cmd;
    int i = 0;

    if (!command || !document) {
        return 0;
    }

    for (i = 0; i < command->object_count; ++i) {
        if (command->objects[i]) {
            document_remove_object(document, command->objects[i]->id);
        }
    }
    return 1;
}

static int delete_selection_undo(void* cmd, Document* document)
{
    DeleteSelectionCommand* command = (DeleteSelectionCommand*)cmd;
    int i = 0;

    if (!command || !document) {
        return 0;
    }

    for (i = 0; i < command->object_count; ++i) {
        if (!document_insert_object_clone_at(document, command->objects[i],
                                             command->original_indices[i])) {
            return 0;
        }
    }

    return 1;
}

static int delete_selection_merge(void* cmd, const void* next)
{
    (void)cmd;
    (void)next;
    return 0;
}

static void delete_selection_destroy(void* cmd)
{
    DeleteSelectionCommand* command = (DeleteSelectionCommand*)cmd;
    int i = 0;

    if (!command) {
        return;
    }

    for (i = 0; i < command->object_count; ++i) {
        object_destroy(command->objects[i]);
    }
    g_allocator.free_fn(command->objects);
    g_allocator.free_fn(command->original_indices);
    g_allocator.free_fn(command);
}

const CommandVTable DELETE_SELECTION_VTABLE = {delete_selection_execute, delete_selection_undo,
                                               delete_selection_execute, delete_selection_merge,
                                               delete_selection_destroy};

static int move_objects_apply(Document* document, const MoveObjectsCommand* command,
                              Vec2 delta)
{
    int i = 0;
    int moved = 0;

    if (!document || !command) {
        return 0;
    }

    for (i = 0; i < command->object_count; ++i) {
        GraphicObject* object = document_find_for_command(document, command->object_ids[i]);
        if (object) {
            object_translate(object, delta);
            moved = 1;
        }
    }

    if (moved) {
        document_touch(document);
    }
    return moved;
}

static int move_objects_execute(void* cmd, Document* document)
{
    MoveObjectsCommand* command = (MoveObjectsCommand*)cmd;
    return move_objects_apply(document, command,
                              command ? command->delta : vec2_make(0.0f, 0.0f));
}

static int move_objects_undo(void* cmd, Document* document)
{
    MoveObjectsCommand* command = (MoveObjectsCommand*)cmd;
    Vec2 undo_delta = vec2_make(0.0f, 0.0f);

    if (!command) {
        return 0;
    }

    undo_delta.x = -command->delta.x;
    undo_delta.y = -command->delta.y;
    return move_objects_apply(document, command, undo_delta);
}

static int move_objects_merge(void* cmd, const void* next)
{
    MoveObjectsCommand* command = (MoveObjectsCommand*)cmd;
    const MoveObjectsCommand* next_command = (const MoveObjectsCommand*)next;
    float dot = 0.0f;
    int i = 0;

    if (!command || !next_command || next_command->base.vtable != &MOVE_OBJECTS_VTABLE ||
        command->object_count != next_command->object_count) {
        return 0;
    }

    for (i = 0; i < command->object_count; ++i) {
        if (command->object_ids[i] != next_command->object_ids[i]) {
            return 0;
        }
    }

    dot = command->delta.x * next_command->delta.x + command->delta.y * next_command->delta.y;
    if (dot <= 0.0f) {
        return 0;
    }

    command->delta = vec2_add(command->delta, next_command->delta);
    return 1;
}

static void move_objects_destroy(void* cmd)
{
    MoveObjectsCommand* command = (MoveObjectsCommand*)cmd;
    if (!command) {
        return;
    }

    g_allocator.free_fn(command->object_ids);
    g_allocator.free_fn(command);
}

const CommandVTable MOVE_OBJECTS_VTABLE = {move_objects_execute, move_objects_undo,
                                           move_objects_execute, move_objects_merge,
                                           move_objects_destroy};

static int paste_objects_execute(void* cmd, Document* document)
{
    PasteObjectsCommand* command = (PasteObjectsCommand*)cmd;
    int i = 0;

    if (!command || !document) {
        return 0;
    }

    for (i = 0; i < command->object_count; ++i) {
        GraphicObject* clone = object_clone(command->object_snapshots[i]);
        if (!clone) {
            break;
        }

        if (!document_append_object_with_id_to_layer(document,
                                                     clone,
                                                     command->object_ids[i],
                                                     command->layer_id)) {
            object_destroy(clone);
            break;
        }
    }

    if (i == command->object_count) {
        return 1;
    }

    while (i > 0) {
        --i;
        if (!document_remove_object(document, command->object_ids[i])) {
            LOG_WARN("paste_objects_execute rollback: failed to remove object %u",
                     (unsigned)command->object_ids[i]);
        }
    }
    return 0;
}

static int paste_objects_undo(void* cmd, Document* document)
{
    PasteObjectsCommand* command = (PasteObjectsCommand*)cmd;
    int i = 0;

    if (!command || !document) {
        return 0;
    }

    for (i = command->object_count - 1; i >= 0; --i) {
        if (!document_remove_object(document, command->object_ids[i])) {
            return 0;
        }
    }

    return 1;
}

static int paste_objects_merge(void* cmd, const void* next)
{
    (void)cmd;
    (void)next;
    return 0;
}

static void paste_objects_destroy(void* cmd)
{
    PasteObjectsCommand* command = (PasteObjectsCommand*)cmd;
    int i = 0;

    if (!command) {
        return;
    }

    for (i = 0; i < command->object_count; ++i) {
        object_destroy(command->object_snapshots[i]);
    }
    g_allocator.free_fn(command->object_snapshots);
    g_allocator.free_fn(command->object_ids);
    g_allocator.free_fn(command);
}

const CommandVTable PASTE_OBJECTS_VTABLE = {paste_objects_execute, paste_objects_undo,
                                            paste_objects_execute, paste_objects_merge,
                                            paste_objects_destroy};

static int set_property_apply(Document* document, const SetPropertyCommand* command,
                              float value)
{
    GraphicObject* object = NULL;

    if (!document || !command) {
        return 0;
    }

    object = document_find_for_command(document, command->object_id);
    if (!object || !object_set_scalar(object, command->key, value)) {
        return 0;
    }

    document_touch(document);
    return 1;
}

static int set_property_execute(void* cmd, Document* document)
{
    SetPropertyCommand* command = (SetPropertyCommand*)cmd;
    return set_property_apply(document, command, command ? command->after_value : 0.0f);
}

static int set_property_undo(void* cmd, Document* document)
{
    SetPropertyCommand* command = (SetPropertyCommand*)cmd;
    return set_property_apply(document, command, command ? command->before_value : 0.0f);
}

static int set_property_merge(void* cmd, const void* next)
{
    SetPropertyCommand* command = (SetPropertyCommand*)cmd;
    const SetPropertyCommand* next_command = (const SetPropertyCommand*)next;

    if (!command || !next_command || next_command->base.vtable != &SET_PROPERTY_VTABLE ||
        command->object_id != next_command->object_id ||
        strcmp(command->key, next_command->key) != 0) {
        return 0;
    }

    command->after_value = next_command->after_value;
    return 1;
}

static void set_property_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable SET_PROPERTY_VTABLE = {set_property_execute, set_property_undo,
                                           set_property_execute, set_property_merge,
                                           set_property_destroy};

Command* command_create_create_object(GraphicObject* object_snapshot)
{
    CreateObjectCommand* command = NULL;

    if (!object_snapshot) {
        return NULL;
    }

    command = (CreateObjectCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        object_destroy(object_snapshot);
        return NULL;
    }

    command->base.vtable = &CREATE_OBJECT_VTABLE;
    command->object_snapshot = object_snapshot;
    command->object_id = object_snapshot->id;
    return (Command*)command;
}

Command* command_create_delete_selection(const Document* document,
                                         const SelectionSet* selection)
{
    DeleteSelectionCommand* command = NULL;
    int i = 0;
    int count = 0;

    if (!document || !selection || selection->count <= 0) {
        return NULL;
    }

    command = (DeleteSelectionCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }
    command->base.vtable = &DELETE_SELECTION_VTABLE;
    command->objects = (GraphicObject**)g_allocator.calloc_fn((size_t)selection->count,
                                                              sizeof(command->objects[0]));
    command->original_indices = (int*)g_allocator.calloc_fn((size_t)selection->count,
                                                            sizeof(command->original_indices[0]));
    if (!command->objects || !command->original_indices) {
        delete_selection_destroy(command);
        return NULL;
    }

    for (i = 0; i < document->count; ++i) {
        const GraphicObject* object = document->objects[i];
        if (object && selection_set_contains(selection, object->id)) {
            command->objects[count] = object_clone(object);
            if (!command->objects[count]) {
                delete_selection_destroy(command);
                return NULL;
            }
            command->original_indices[count] = i;
            count++;
        }
    }

    if (count <= 0) {
        delete_selection_destroy(command);
        return NULL;
    }

    command->object_count = count;
    return (Command*)command;
}

Command* command_create_move_objects(const ObjectId* object_ids, int object_count,
                                     Vec2 delta)
{
    MoveObjectsCommand* command = NULL;

    if (!object_ids || object_count <= 0 || vec2_length_sq(delta) <= 1e-6f) {
        return NULL;
    }

    command = (MoveObjectsCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->object_ids =
        (ObjectId*)g_allocator.calloc_fn((size_t)object_count, sizeof(command->object_ids[0]));
    if (!command->object_ids) {
        move_objects_destroy(command);
        return NULL;
    }

    command->base.vtable = &MOVE_OBJECTS_VTABLE;
    memcpy(command->object_ids, object_ids, (size_t)object_count * sizeof(object_ids[0]));
    command->object_count = object_count;
    command->delta = delta;
    return (Command*)command;
}

Command* command_create_paste_objects(const Document* document,
                                      GraphicObject* const* object_snapshots,
                                      int object_count,
                                      Vec2 delta,
                                      LayerId layer_id,
                                      SelectionSet* out_selection)
{
    PasteObjectsCommand* command = NULL;
    int i = 0;

    if (!document || !object_snapshots || object_count <= 0 || layer_id == 0u) {
        return NULL;
    }

    command = (PasteObjectsCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->object_snapshots =
        (GraphicObject**)g_allocator.calloc_fn((size_t)object_count,
                                               sizeof(command->object_snapshots[0]));
    command->object_ids =
        (ObjectId*)g_allocator.calloc_fn((size_t)object_count, sizeof(command->object_ids[0]));
    if (!command->object_snapshots || !command->object_ids) {
        paste_objects_destroy(command);
        return NULL;
    }

    command->base.vtable = &PASTE_OBJECTS_VTABLE;
    command->object_count = object_count;
    command->layer_id = layer_id;
    if (out_selection) {
        selection_set_clear(out_selection);
        if (!selection_set_reserve(out_selection, object_count)) {
            paste_objects_destroy(command);
            return NULL;
        }
    }

    for (i = 0; i < object_count; ++i) {
        command->object_snapshots[i] = object_clone(object_snapshots[i]);
        if (!command->object_snapshots[i]) {
            paste_objects_destroy(command);
            return NULL;
        }

        object_translate(command->object_snapshots[i], delta);
        command->object_snapshots[i]->layer_id = layer_id;
        command->object_snapshots[i]->id = document->next_id + (ObjectId)i;
        command->object_ids[i] = command->object_snapshots[i]->id;

        if (out_selection) {
            out_selection->ids[i] = command->object_ids[i];
            out_selection->count++;
        }
    }

    return (Command*)command;
}

Command* command_create_set_property(ObjectId object_id, const char* key,
                                     float before_value, float after_value)
{
    SetPropertyCommand* command = NULL;

    if (object_id == 0u || !key || key[0] == '\0' ||
        strlen(key) >= sizeof(((SetPropertyCommand*)0)->key)) {
        return NULL;
    }

    command = (SetPropertyCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &SET_PROPERTY_VTABLE;
    command->object_id = object_id;
    snprintf(command->key, sizeof(command->key), "%s", key);
    command->before_value = before_value;
    command->after_value = after_value;
    return (Command*)command;
}

Command* command_create_set_property_from_document(const Document* document,
                                                   ObjectId object_id,
                                                   const char* key,
                                                   float after_value)
{
    GraphicObject* object = NULL;
    float before_value = 0.0f;

    if (!document || object_id == 0u || !key || key[0] == '\0') {
        return NULL;
    }

    object = document_find_object(document, object_id);
    if (!object || !object_get_scalar(object, key, &before_value)) {
        return NULL;
    }

    return command_create_set_property(object_id, key, before_value, after_value);
}
