#include "document_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int document_layer_has_objects(const Document* document, LayerId layer_id)
{
    int i = 0;

    if (!document || layer_id == 0u) {
        return 0;
    }

    for (i = 0; i < document->count; ++i) {
        if (document->objects[i] && document->objects[i]->layer_id == layer_id) {
            return 1;
        }
    }

    return 0;
}

void document_reset_layers(Document* document)
{
    if (!document) {
        return;
    }

    free(document->layers);
    document->layers = NULL;
    document->layer_capacity = 0;
    document->layer_count = 1;
    if (!document_reserve_layers(document, 1)) {
        document->layer_count = 0;
        document->active_layer_id = 0u;
        document->next_layer_id = DOCUMENT_DEFAULT_LAYER_ID;
        return;
    }

    memset(document->layers, 0, (size_t)document->layer_capacity * sizeof(document->layers[0]));
    document->layers[0].id = DOCUMENT_DEFAULT_LAYER_ID;
    snprintf(document->layers[0].name, sizeof(document->layers[0].name), "%s",
             DOCUMENT_DEFAULT_LAYER_NAME);
    document->layers[0].visible = 1;
    document->layers[0].locked = 0;
    document->layers[0].blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
    document->active_layer_id = DOCUMENT_DEFAULT_LAYER_ID;
    document->next_layer_id = DOCUMENT_DEFAULT_LAYER_ID + 1u;
}

int document_reserve_layers(Document* document, int needed)
{
    DocumentLayer* layers = NULL;
    int capacity = 0;

    if (!document || needed <= 0) {
        return document != NULL;
    }
    if (needed <= document->layer_capacity) {
        return 1;
    }

    capacity =
        document->layer_capacity > 0 ? document->layer_capacity : DOCUMENT_DEFAULT_LAYER_CAPACITY;
    while (capacity < needed) {
        capacity *= 2;
    }

    layers = (DocumentLayer*)realloc(document->layers,
                                     (size_t)capacity * sizeof(document->layers[0]));
    if (!layers) {
        return 0;
    }

    if (capacity > document->layer_capacity) {
        memset(layers + document->layer_capacity, 0,
               (size_t)(capacity - document->layer_capacity) * sizeof(layers[0]));
    }
    document->layers = layers;
    document->layer_capacity = capacity;
    return 1;
}

LayerId document_resolve_layer_id(const Document* document, LayerId layer_id)
{
    if (!document) {
        return DOCUMENT_DEFAULT_LAYER_ID;
    }
    if (layer_id != 0u && document_layer_find_const(document, layer_id)) {
        return layer_id;
    }
    if (document_layer_find_const(document, document->active_layer_id)) {
        return document->active_layer_id;
    }
    return DOCUMENT_DEFAULT_LAYER_ID;
}

int document_layer_count(const Document* document)
{
    return document ? document->layer_count : 0;
}

const DocumentLayer* document_layer_at(const Document* document, int index)
{
    if (!document || index < 0 || index >= document->layer_count) {
        return NULL;
    }

    return &document->layers[index];
}

int document_layer_index(const Document* document, LayerId layer_id)
{
    int i = 0;

    if (!document || layer_id == 0u) {
        return -1;
    }

    for (i = 0; i < document->layer_count; ++i) {
        if (document->layers[i].id == layer_id) {
            return i;
        }
    }

    return -1;
}

DocumentLayer* document_layer_find(Document* document, LayerId layer_id)
{
    int i = 0;

    if (!document || layer_id == 0u) {
        return NULL;
    }

    for (i = 0; i < document->layer_count; ++i) {
        if (document->layers[i].id == layer_id) {
            return &document->layers[i];
        }
    }

    return NULL;
}

const DocumentLayer* document_layer_find_const(const Document* document,
                                               LayerId layer_id)
{
    int i = 0;

    if (!document || layer_id == 0u) {
        return NULL;
    }

    for (i = 0; i < document->layer_count; ++i) {
        if (document->layers[i].id == layer_id) {
            return &document->layers[i];
        }
    }

    return NULL;
}

int document_layer_is_locked(const Document* document, LayerId layer_id)
{
    const DocumentLayer* layer = document_layer_find_const(document, layer_id);
    return layer ? layer->locked : 0;
}

int document_object_is_locked(const Document* document, ObjectId object_id)
{
    const GraphicObject* object = document_find_object(document, object_id);
    return object ? document_layer_is_locked(document, object->layer_id) : 0;
}

LayerId document_active_layer_id(const Document* document)
{
    return document ? document->active_layer_id : 0u;
}

int document_set_active_layer(Document* document, LayerId layer_id)
{
    if (!document || !document_layer_find(document, layer_id)) {
        return 0;
    }
    if (document->active_layer_id == layer_id) {
        return 1;
    }

    document->active_layer_id = layer_id;
    document->revision++;
    return 1;
}

LayerId document_create_layer(Document* document, const char* name)
{
    DocumentLayer* layer = NULL;

    if (!document || !document_reserve_layers(document, document->layer_count + 1)) {
        return 0u;
    }

    layer = &document->layers[document->layer_count++];
    memset(layer, 0, sizeof(*layer));
    layer->id = document->next_layer_id++;
    layer->visible = 1;
    layer->locked = 0;
    layer->blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
    snprintf(layer->name, sizeof(layer->name), "%s",
             (name && name[0] != '\0') ? name : "Layer");
    document->revision++;
    document_spatial_invalidate(document);
    return layer->id;
}

LayerId document_create_layer_with_id(Document* document,
                                      const char* name,
                                      LayerId layer_id)
{
    DocumentLayer layer;

    if (!document || layer_id == 0u ||
        !document_reserve_layers(document, document->layer_count + 1) ||
        document_layer_find(document, layer_id)) {
        return 0u;
    }

    memset(&layer, 0, sizeof(layer));
    layer.id = layer_id;
    layer.visible = 1;
    layer.locked = 0;
    layer.blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
    snprintf(layer.name, sizeof(layer.name), "%s",
             (name && name[0] != '\0') ? name : "Layer");

    if (!document_insert_layer_at(document, &layer, document->layer_count)) {
        return 0u;
    }

    return layer_id;
}

int document_insert_layer_at(Document* document,
                             const DocumentLayer* layer,
                             int index)
{
    int i = 0;

    if (!document || !layer || layer->id == 0u ||
        !document_reserve_layers(document, document->layer_count + 1) ||
        document_layer_find(document, layer->id)) {
        return 0;
    }

    if (index < 0) {
        index = 0;
    }
    if (index > document->layer_count) {
        index = document->layer_count;
    }

    for (i = document->layer_count; i > index; --i) {
        document->layers[i] = document->layers[i - 1];
    }
    document->layers[index] = *layer;
    document->layer_count++;
    if (document->next_layer_id <= layer->id) {
        document->next_layer_id = layer->id + 1u;
    }
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}

int document_delete_layer(Document* document, LayerId layer_id)
{
    int index = -1;
    int i = 0;

    if (!document || layer_id == 0u || document->layer_count <= 1) {
        return 0;
    }
    if (document_layer_has_objects(document, layer_id)) {
        return 0;
    }

    index = document_layer_index(document, layer_id);
    if (index < 0) {
        return 0;
    }

    for (i = index; i < document->layer_count - 1; ++i) {
        document->layers[i] = document->layers[i + 1];
    }
    memset(&document->layers[document->layer_count - 1], 0,
           sizeof(document->layers[document->layer_count - 1]));
    document->layer_count--;
    if (document->active_layer_id == layer_id) {
        document->active_layer_id = document->layers[0].id;
    }
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}

int document_rename_layer(Document* document, LayerId layer_id, const char* name)
{
    DocumentLayer* layer = NULL;
    char normalized_name[sizeof(((DocumentLayer*)0)->name)];

    if (!document || layer_id == 0u || !name || name[0] == '\0') {
        return 0;
    }

    layer = document_layer_find(document, layer_id);
    if (!layer) {
        return 0;
    }

    snprintf(normalized_name, sizeof(normalized_name), "%s", name);
    if (strcmp(layer->name, normalized_name) == 0) {
        return 1;
    }

    snprintf(layer->name, sizeof(layer->name), "%s", normalized_name);
    document->revision++;
    return 1;
}

int document_move_layer(Document* document, LayerId layer_id, int target_index)
{
    DocumentLayer moved_layer;
    int from_index = 0;
    int i = 0;

    if (!document || layer_id == 0u || document->layer_count <= 1) {
        return 0;
    }

    from_index = document_layer_index(document, layer_id);
    if (from_index < 0) {
        return 0;
    }

    if (target_index < 0) {
        target_index = 0;
    }
    if (target_index >= document->layer_count) {
        target_index = document->layer_count - 1;
    }
    if (from_index == target_index) {
        return 1;
    }

    moved_layer = document->layers[from_index];
    if (from_index < target_index) {
        for (i = from_index; i < target_index; ++i) {
            document->layers[i] = document->layers[i + 1];
        }
    } else {
        for (i = from_index; i > target_index; --i) {
            document->layers[i] = document->layers[i - 1];
        }
    }
    document->layers[target_index] = moved_layer;
    document->revision++;
    return 1;
}

int document_set_layer_visibility(Document* document,
                                  LayerId layer_id,
                                  int visible)
{
    DocumentLayer* layer = document_layer_find(document, layer_id);

    if (!layer) {
        return 0;
    }

    visible = visible ? 1 : 0;
    if (layer->visible == visible) {
        return 1;
    }

    layer->visible = visible;
    document->revision++;
    document_spatial_invalidate(document);
    return 1;
}

int document_set_layer_locked(Document* document, LayerId layer_id, int locked)
{
    DocumentLayer* layer = document_layer_find(document, layer_id);

    if (!layer) {
        return 0;
    }

    locked = locked ? 1 : 0;
    if (layer->locked == locked) {
        return 1;
    }

    layer->locked = locked;
    document->revision++;
    return 1;
}

int document_set_layer_blend_mode(Document* document,
                                  LayerId layer_id,
                                  DocumentLayerBlendMode blend_mode)
{
    DocumentLayer* layer = document_layer_find(document, layer_id);

    if (!layer) {
        return 0;
    }

    layer->blend_mode = blend_mode;
    document->revision++;
    return 1;
}
