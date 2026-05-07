#include "command_internal.h"

#include <stdio.h>
#include <string.h>

static int set_active_layer_apply(Document* document, LayerId layer_id)
{
    if (!document) {
        return 0;
    }

    return document_set_active_layer(document, layer_id);
}

static int set_active_layer_execute(void* cmd, Document* document)
{
    SetActiveLayerCommand* command = (SetActiveLayerCommand*)cmd;
    return set_active_layer_apply(document, command ? command->after_layer_id : 0u);
}

static int set_active_layer_undo(void* cmd, Document* document)
{
    SetActiveLayerCommand* command = (SetActiveLayerCommand*)cmd;
    return set_active_layer_apply(document, command ? command->before_layer_id : 0u);
}

static int set_active_layer_merge(void* cmd, const void* next)
{
    SetActiveLayerCommand* command = (SetActiveLayerCommand*)cmd;
    const SetActiveLayerCommand* next_command = (const SetActiveLayerCommand*)next;

    if (!command || !next_command ||
        next_command->base.vtable != &SET_ACTIVE_LAYER_VTABLE) {
        return 0;
    }

    command->after_layer_id = next_command->after_layer_id;
    return 1;
}

static void set_active_layer_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable SET_ACTIVE_LAYER_VTABLE = {
    set_active_layer_execute, set_active_layer_undo, set_active_layer_execute,
    set_active_layer_merge, set_active_layer_destroy};

static int set_layer_visibility_apply(Document* document,
                                      LayerId layer_id,
                                      int visible)
{
    if (!document) {
        return 0;
    }

    return document_set_layer_visibility(document, layer_id, visible);
}

static int set_layer_visibility_execute(void* cmd, Document* document)
{
    SetLayerVisibilityCommand* command = (SetLayerVisibilityCommand*)cmd;
    return set_layer_visibility_apply(document,
                                      command ? command->layer_id : 0u,
                                      command ? command->after_visible : 0);
}

static int set_layer_visibility_undo(void* cmd, Document* document)
{
    SetLayerVisibilityCommand* command = (SetLayerVisibilityCommand*)cmd;
    return set_layer_visibility_apply(document,
                                      command ? command->layer_id : 0u,
                                      command ? command->before_visible : 0);
}

static int set_layer_visibility_merge(void* cmd, const void* next)
{
    SetLayerVisibilityCommand* command = (SetLayerVisibilityCommand*)cmd;
    const SetLayerVisibilityCommand* next_command = (const SetLayerVisibilityCommand*)next;

    if (!command || !next_command ||
        next_command->base.vtable != &SET_LAYER_VISIBILITY_VTABLE ||
        command->layer_id != next_command->layer_id) {
        return 0;
    }

    command->after_visible = next_command->after_visible;
    return 1;
}

static void set_layer_visibility_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable SET_LAYER_VISIBILITY_VTABLE = {
    set_layer_visibility_execute,
    set_layer_visibility_undo,
    set_layer_visibility_execute,
    set_layer_visibility_merge,
    set_layer_visibility_destroy};

static int set_layer_locked_apply(Document* document, LayerId layer_id, int locked)
{
    if (!document) {
        return 0;
    }

    return document_set_layer_locked(document, layer_id, locked);
}

static int set_layer_locked_execute(void* cmd, Document* document)
{
    SetLayerLockedCommand* command = (SetLayerLockedCommand*)cmd;
    return set_layer_locked_apply(document,
                                  command ? command->layer_id : 0u,
                                  command ? command->after_locked : 0);
}

static int set_layer_locked_undo(void* cmd, Document* document)
{
    SetLayerLockedCommand* command = (SetLayerLockedCommand*)cmd;
    return set_layer_locked_apply(document,
                                  command ? command->layer_id : 0u,
                                  command ? command->before_locked : 0);
}

static int set_layer_locked_merge(void* cmd, const void* next)
{
    SetLayerLockedCommand* command = (SetLayerLockedCommand*)cmd;
    const SetLayerLockedCommand* next_command = (const SetLayerLockedCommand*)next;

    if (!command || !next_command ||
        next_command->base.vtable != &SET_LAYER_LOCKED_VTABLE ||
        command->layer_id != next_command->layer_id) {
        return 0;
    }

    command->after_locked = next_command->after_locked;
    return 1;
}

static void set_layer_locked_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable SET_LAYER_LOCKED_VTABLE = {
    set_layer_locked_execute,
    set_layer_locked_undo,
    set_layer_locked_execute,
    set_layer_locked_merge,
    set_layer_locked_destroy};

static int rename_layer_apply(Document* document, LayerId layer_id, const char* name)
{
    if (!document || !name) {
        return 0;
    }

    return document_rename_layer(document, layer_id, name);
}

static int rename_layer_execute(void* cmd, Document* document)
{
    RenameLayerCommand* command = (RenameLayerCommand*)cmd;
    return rename_layer_apply(document,
                              command ? command->layer_id : 0u,
                              command ? command->after_name : NULL);
}

static int rename_layer_undo(void* cmd, Document* document)
{
    RenameLayerCommand* command = (RenameLayerCommand*)cmd;
    return rename_layer_apply(document,
                              command ? command->layer_id : 0u,
                              command ? command->before_name : NULL);
}

static int rename_layer_merge(void* cmd, const void* next)
{
    RenameLayerCommand* command = (RenameLayerCommand*)cmd;
    const RenameLayerCommand* next_command = (const RenameLayerCommand*)next;

    if (!command || !next_command ||
        next_command->base.vtable != &RENAME_LAYER_VTABLE ||
        command->layer_id != next_command->layer_id) {
        return 0;
    }

    snprintf(command->after_name, sizeof(command->after_name), "%s",
             next_command->after_name);
    return 1;
}

static void rename_layer_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable RENAME_LAYER_VTABLE = {rename_layer_execute, rename_layer_undo,
                                           rename_layer_execute, rename_layer_merge,
                                           rename_layer_destroy};

static int move_layer_apply(Document* document, LayerId layer_id, int index)
{
    if (!document) {
        return 0;
    }

    return document_move_layer(document, layer_id, index);
}

static int move_layer_execute(void* cmd, Document* document)
{
    MoveLayerCommand* command = (MoveLayerCommand*)cmd;
    return move_layer_apply(document,
                            command ? command->layer_id : 0u,
                            command ? command->after_index : 0);
}

static int move_layer_undo(void* cmd, Document* document)
{
    MoveLayerCommand* command = (MoveLayerCommand*)cmd;
    return move_layer_apply(document,
                            command ? command->layer_id : 0u,
                            command ? command->before_index : 0);
}

static int move_layer_merge(void* cmd, const void* next)
{
    MoveLayerCommand* command = (MoveLayerCommand*)cmd;
    const MoveLayerCommand* next_command = (const MoveLayerCommand*)next;

    if (!command || !next_command || next_command->base.vtable != &MOVE_LAYER_VTABLE ||
        command->layer_id != next_command->layer_id) {
        return 0;
    }

    command->after_index = next_command->after_index;
    return 1;
}

static void move_layer_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable MOVE_LAYER_VTABLE = {move_layer_execute, move_layer_undo,
                                         move_layer_execute, move_layer_merge,
                                         move_layer_destroy};

static int create_layer_execute(void* cmd, Document* document)
{
    CreateLayerCommand* command = (CreateLayerCommand*)cmd;
    DocumentLayer* layer = NULL;

    if (!command || !document) {
        return 0;
    }

    if (!command->created_once) {
        command->previous_active_layer_id = document_active_layer_id(document);
        command->layer_index = document_layer_count(document);
        command->layer.id = document_create_layer(document, command->layer.name);
        if (command->layer.id == 0u) {
            return 0;
        }
        layer = document_layer_find(document, command->layer.id);
        if (!layer) {
            return 0;
        }
        command->layer = *layer;
        command->created_once = 1;
        return document_set_active_layer(document, command->layer.id);
    }

    if (!document_insert_layer_at(document, &command->layer, command->layer_index)) {
        return 0;
    }
    return document_set_active_layer(document, command->layer.id);
}

static int create_layer_undo(void* cmd, Document* document)
{
    CreateLayerCommand* command = (CreateLayerCommand*)cmd;

    if (!command || !document || command->layer.id == 0u) {
        return 0;
    }

    if (!document_delete_layer(document, command->layer.id)) {
        return 0;
    }
    if (command->previous_active_layer_id != 0u) {
        return document_set_active_layer(document, command->previous_active_layer_id);
    }
    return 1;
}

static int create_layer_merge(void* cmd, const void* next)
{
    (void)cmd;
    (void)next;
    return 0;
}

static void create_layer_destroy(void* cmd)
{
    g_allocator.free_fn(cmd);
}

const CommandVTable CREATE_LAYER_VTABLE = {
    create_layer_execute, create_layer_undo, create_layer_execute,
    create_layer_merge, create_layer_destroy};

Command* command_create_set_active_layer(const Document* document,
                                         LayerId after_layer_id)
{
    SetActiveLayerCommand* command = NULL;

    if (!document || after_layer_id == 0u ||
        !document_layer_find_const(document, after_layer_id) ||
        document_active_layer_id(document) == after_layer_id) {
        return NULL;
    }

    command = (SetActiveLayerCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &SET_ACTIVE_LAYER_VTABLE;
    command->before_layer_id = document_active_layer_id(document);
    command->after_layer_id = after_layer_id;
    return (Command*)command;
}

Command* command_create_set_layer_visibility(const Document* document,
                                             LayerId layer_id,
                                             int visible)
{
    const DocumentLayer* layer = NULL;
    SetLayerVisibilityCommand* command = NULL;

    if (!document || layer_id == 0u) {
        return NULL;
    }

    layer = document_layer_find_const(document, layer_id);
    if (!layer || layer->visible == (visible ? 1 : 0)) {
        return NULL;
    }

    command = (SetLayerVisibilityCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &SET_LAYER_VISIBILITY_VTABLE;
    command->layer_id = layer_id;
    command->before_visible = layer->visible;
    command->after_visible = visible ? 1 : 0;
    return (Command*)command;
}

Command* command_create_set_layer_locked(const Document* document,
                                         LayerId layer_id,
                                         int locked)
{
    const DocumentLayer* layer = NULL;
    SetLayerLockedCommand* command = NULL;

    if (!document || layer_id == 0u) {
        return NULL;
    }

    layer = document_layer_find_const(document, layer_id);
    locked = locked ? 1 : 0;
    if (!layer || layer->locked == locked) {
        return NULL;
    }

    command = (SetLayerLockedCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &SET_LAYER_LOCKED_VTABLE;
    command->layer_id = layer_id;
    command->before_locked = layer->locked;
    command->after_locked = locked;
    return (Command*)command;
}

Command* command_create_rename_layer(const Document* document,
                                     LayerId layer_id,
                                     const char* name)
{
    const DocumentLayer* layer = NULL;
    RenameLayerCommand* command = NULL;

    if (!document || layer_id == 0u || !name || name[0] == '\0') {
        return NULL;
    }

    layer = document_layer_find_const(document, layer_id);
    if (!layer || strcmp(layer->name, name) == 0) {
        return NULL;
    }

    command = (RenameLayerCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &RENAME_LAYER_VTABLE;
    command->layer_id = layer_id;
    snprintf(command->before_name, sizeof(command->before_name), "%s", layer->name);
    snprintf(command->after_name, sizeof(command->after_name), "%s", name);
    return (Command*)command;
}

Command* command_create_move_layer(const Document* document,
                                   LayerId layer_id,
                                   int target_index)
{
    MoveLayerCommand* command = NULL;
    int current_index = 0;

    if (!document || layer_id == 0u) {
        return NULL;
    }

    current_index = document_layer_index(document, layer_id);
    if (current_index < 0) {
        return NULL;
    }

    if (target_index < 0) {
        target_index = 0;
    }
    if (target_index >= document_layer_count(document)) {
        target_index = document_layer_count(document) - 1;
    }
    if (current_index == target_index) {
        return NULL;
    }

    command = (MoveLayerCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &MOVE_LAYER_VTABLE;
    command->layer_id = layer_id;
    command->before_index = current_index;
    command->after_index = target_index;
    return (Command*)command;
}

Command* command_create_create_layer(const char* name)
{
    CreateLayerCommand* command = NULL;

    command = (CreateLayerCommand*)g_allocator.calloc_fn(1u, sizeof(*command));
    if (!command) {
        return NULL;
    }

    command->base.vtable = &CREATE_LAYER_VTABLE;
    snprintf(command->layer.name, sizeof(command->layer.name), "%s",
             (name && name[0] != '\0') ? name : "Layer");
    command->layer.visible = 1;
    command->layer.locked = 0;
    command->layer.blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
    command->layer_index = -1;
    command->previous_active_layer_id = 0u;
    command->created_once = 0;
    return (Command*)command;
}
