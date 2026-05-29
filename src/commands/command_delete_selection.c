#include "command_internal.h"

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

Command* command_create_delete_selection(const Document* document,
                                         const SelectionSet* selection)
{
    DeleteSelectionCommand* command = NULL;
    int i = 0;
    int count = 0;
    int object_count = 0;

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

    object_count = document_object_count(document);
    for (i = 0; i < object_count; ++i) {
        const GraphicObject* object = document_get_object_at_const(document, i);
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
