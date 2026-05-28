#include "command_internal.h"

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
