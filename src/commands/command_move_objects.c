#include "command_internal.h"

#include <base/math2d.h>

#include <string.h>

static GraphicObject* document_find_for_command(Document* document, ObjectId id)
{
    return document_find_object(document, id);
}

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
