#include "document_internal.h"

#include <stdlib.h>

void document_clear_objects(Document* document)
{
    int i = 0;

    if (!document) {
        return;
    }

    for (i = 0; i < document->count; ++i) {
        object_destroy(document->objects[i]);
    }

    free(document->objects);
    document->objects = NULL;
    document->count = 0;
    document->capacity = 0;
}

int document_reserve_objects(Document* document, int needed)
{
    GraphicObject** objects = NULL;
    int capacity = 0;

    if (!document || needed <= 0) {
        return document != NULL;
    }
    if (needed <= document->capacity) {
        return 1;
    }

    capacity = document->capacity > 0 ? document->capacity : DOCUMENT_DEFAULT_OBJECT_CAPACITY;
    while (capacity < needed) {
        capacity *= 2;
    }

    objects = (GraphicObject**)realloc(document->objects,
                                       (size_t)capacity * sizeof(document->objects[0]));
    if (!objects) {
        return 0;
    }

    document->objects = objects;
    document->capacity = capacity;
    return 1;
}

int document_add_object_to_layer(Document* document,
                                 GraphicObject* object,
                                 LayerId layer_id)
{
    if (!document || !object || !document_reserve_objects(document, document->count + 1)) {
        return 0;
    }

    layer_id = document_resolve_layer_id(document, layer_id);
    if (!document_layer_find_const(document, layer_id)) {
        return 0;
    }

    object->id = document->next_id++;
    object->layer_id = layer_id;
    document->objects[document->count++] = object;
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}

int document_add_object(Document* document, GraphicObject* object)
{
    return document_add_object_to_layer(document, object,
                                        document ? document->active_layer_id : 0u);
}

int document_append_object_with_id_to_layer(Document* document,
                                            GraphicObject* object,
                                            ObjectId id,
                                            LayerId layer_id)
{
    if (!document || !object || id == 0u ||
        !document_reserve_objects(document, document->count + 1)) {
        return 0;
    }
    if (document_find_object(document, id)) {
        return 0;
    }

    layer_id = document_resolve_layer_id(document, layer_id);
    if (!document_layer_find_const(document, layer_id)) {
        return 0;
    }

    object->id = id;
    object->layer_id = layer_id;
    document->objects[document->count++] = object;
    if (document->next_id <= id) {
        document->next_id = id + 1u;
    }
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}

int document_append_object_with_id(Document* document, GraphicObject* object,
                                   ObjectId id)
{
    return document_append_object_with_id_to_layer(
        document, object, id, object ? object->layer_id : 0u);
}

GraphicObject* document_find_object(const Document* document, ObjectId id)
{
    int i = 0;

    if (!document) {
        return NULL;
    }

    for (i = 0; i < document->count; ++i) {
        if (document->objects[i] && document->objects[i]->id == id) {
            return document->objects[i];
        }
    }

    return NULL;
}

GraphicObject* document_get_object_at(const Document* document, int index)
{
    if (!document || index < 0 || index >= document->count) {
        return NULL;
    }

    return document->objects[index];
}

int document_remove_object(Document* document, ObjectId id)
{
    int i = 0;

    if (!document) {
        return 0;
    }

    for (i = 0; i < document->count; ++i) {
        if (document->objects[i] && document->objects[i]->id == id) {
            int j = 0;

            object_destroy(document->objects[i]);
            for (j = i; j < document->count - 1; ++j) {
                document->objects[j] = document->objects[j + 1];
            }
            document->objects[document->count - 1] = NULL;
            document->count--;
            document->revision++;
            document_spatial_invalidate(document);
            return 1;
        }
    }

    return 0;
}

void document_touch(Document* document)
{
    if (document) {
        document->revision++;
        document_spatial_invalidate(document);
    }
}

ObjectId document_max_id(const Document* document)
{
    ObjectId max_id = 0u;
    int i = 0;

    if (!document) {
        return 0u;
    }

    for (i = 0; i < document->count; ++i) {
        if (document->objects[i] && document->objects[i]->id > max_id) {
            max_id = document->objects[i]->id;
        }
    }

    return max_id;
}

int document_insert_object_clone_at(Document* document,
                                    const GraphicObject* snapshot,
                                    int index)
{
    GraphicObject* clone = NULL;
    int i = 0;

    if (!document || !snapshot ||
        !document_reserve_objects(document, document->count + 1) ||
        snapshot->id == 0u || document_find_object(document, snapshot->id)) {
        return 0;
    }

    clone = object_clone(snapshot);
    if (!clone) {
        return 0;
    }

    if (!document_layer_find_const(document, clone->layer_id)) {
        clone->layer_id = document_resolve_layer_id(document, clone->layer_id);
    }

    if (index < 0) {
        index = 0;
    }
    if (index > document->count) {
        index = document->count;
    }

    for (i = document->count; i > index; --i) {
        document->objects[i] = document->objects[i - 1];
    }
    document->objects[index] = clone;
    document->count++;
    if (document->next_id <= clone->id) {
        document->next_id = clone->id + 1u;
    }
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}
