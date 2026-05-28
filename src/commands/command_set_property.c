#include "command_internal.h"

#include <stdio.h>
#include <string.h>

static GraphicObject* document_find_for_command(Document* document, ObjectId id)
{
    return document_find_object(document, id);
}

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
