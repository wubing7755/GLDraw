#include "command_internal.h"

#include <base/log.h>

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
