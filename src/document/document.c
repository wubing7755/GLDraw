#include <document/document.h>

#include <base/math2d.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOCUMENT_DEFAULT_LAYER_ID 1u
#define DOCUMENT_DEFAULT_LAYER_NAME "Layer 1"
#define DOCUMENT_DEFAULT_OBJECT_CAPACITY 16
#define DOCUMENT_DEFAULT_LAYER_CAPACITY 4
#define DOCUMENT_SPATIAL_CELL_SIZE 256.0f
#define DOCUMENT_SPATIAL_MAX_AXIS_CELLS 64

static void document_clear_objects(Document *document);
static void document_reset_layers(Document *document);
static int document_reserve_objects(Document *document, int needed);
static int document_reserve_layers(Document *document, int needed);
static void document_spatial_free(Document *document);
static void document_spatial_invalidate(Document *document);
static int document_spatial_add_entry(Document *document,
                                      int cell_index,
                                      int object_index);
static int document_spatial_rebuild(Document *document);
static LayerId document_resolve_layer_id(const Document *document,
                                         LayerId layer_id);
static int rect_intersects(const RectF *a, const RectF *b);
static int document_layer_has_objects(const Document *document, LayerId layer_id);

static void document_clear_objects(Document *document) {
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

static void document_reset_layers(Document *document) {
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

static int document_reserve_objects(Document *document, int needed) {
  GraphicObject **objects = NULL;
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

  objects = (GraphicObject **)realloc(document->objects,
                                      (size_t)capacity * sizeof(document->objects[0]));
  if (!objects) {
    return 0;
  }

  document->objects = objects;
  document->capacity = capacity;
  return 1;
}

static int document_reserve_layers(Document *document, int needed) {
  DocumentLayer *layers = NULL;
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

  layers = (DocumentLayer *)realloc(document->layers,
                                    (size_t)capacity * sizeof(document->layers[0]));
  if (!layers) {
    return 0;
  }

  if (capacity > document->layer_capacity) {
    memset(layers + document->layer_capacity,
           0,
           (size_t)(capacity - document->layer_capacity) * sizeof(layers[0]));
  }
  document->layers = layers;
  document->layer_capacity = capacity;
  return 1;
}

static void document_spatial_free(Document *document) {
  if (!document) {
    return;
  }

  free(document->spatial_heads);
  free(document->spatial_entries);
  free(document->spatial_marks);
  document->spatial_heads = NULL;
  document->spatial_entries = NULL;
  document->spatial_marks = NULL;
  document->spatial_cols = 0;
  document->spatial_rows = 0;
  document->spatial_cell_count = 0;
  document->spatial_entry_count = 0;
  document->spatial_entry_capacity = 0;
  document->spatial_revision = 0u;
  document->spatial_query_token = 0u;
  document->spatial_bounds = (RectF){0.0f, 0.0f, 0.0f, 0.0f};
  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document->spatial_mark_capacity = 0;
}

static void document_spatial_invalidate(Document *document) {
  if (!document) {
    return;
  }

  document->spatial_revision = 0u;
}

static int document_spatial_add_entry(Document *document,
                                      int cell_index,
                                      int object_index) {
  DocumentSpatialEntry *entries = NULL;
  int capacity = 0;

  if (!document || cell_index < 0 || cell_index >= document->spatial_cell_count) {
    return 0;
  }

  if (document->spatial_entry_count >= document->spatial_entry_capacity) {
    capacity =
        (document->spatial_entry_capacity > 0) ? document->spatial_entry_capacity * 2 : 128;
    entries = (DocumentSpatialEntry *)realloc(
        document->spatial_entries, (size_t)capacity * sizeof(document->spatial_entries[0]));
    if (!entries) {
      return 0;
    }
    document->spatial_entries = entries;
    document->spatial_entry_capacity = capacity;
  }

  document->spatial_entries[document->spatial_entry_count].object_index = object_index;
  document->spatial_entries[document->spatial_entry_count].next =
      document->spatial_heads[cell_index];
  document->spatial_heads[cell_index] = document->spatial_entry_count;
  document->spatial_entry_count++;
  return 1;
}

static int document_spatial_rebuild(Document *document) {
  RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};
  int first = 1;
  int i = 0;

  if (!document) {
    return 0;
  }
  if (document->spatial_revision == document->revision) {
    return 1;
  }

  document->spatial_entry_count = 0;
  if (document->count <= 0) {
    if (document->spatial_heads) {
      memset(document->spatial_heads, 0xff,
             (size_t)document->spatial_cell_count * sizeof(document->spatial_heads[0]));
    }
    if (document->spatial_marks) {
      memset(document->spatial_marks, 0,
             (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
    }
    document->spatial_bounds = bounds;
    document->spatial_revision = document->revision;
    return 1;
  }

  for (i = 0; i < document->count; ++i) {
    RectF object_bounds;

    if (!document->objects[i]) {
      continue;
    }

    object_bounds = object_get_bounds(document->objects[i]);
    if (first) {
      bounds = object_bounds;
      first = 0;
    } else {
      float left = bounds.x < object_bounds.x ? bounds.x : object_bounds.x;
      float bottom = bounds.y < object_bounds.y ? bounds.y : object_bounds.y;
      float right = rectf_right(&bounds) > rectf_right(&object_bounds)
                        ? rectf_right(&bounds)
                        : rectf_right(&object_bounds);
      float top = rectf_top(&bounds) > rectf_top(&object_bounds) ? rectf_top(&bounds)
                                                                 : rectf_top(&object_bounds);
      bounds.x = left;
      bounds.y = bottom;
      bounds.w = right - left;
      bounds.h = top - bottom;
    }
  }

  bounds.x -= 1.0f;
  bounds.y -= 1.0f;
  bounds.w += 2.0f;
  bounds.h += 2.0f;
  if (bounds.w < DOCUMENT_SPATIAL_CELL_SIZE) {
    bounds.w = DOCUMENT_SPATIAL_CELL_SIZE;
  }
  if (bounds.h < DOCUMENT_SPATIAL_CELL_SIZE) {
    bounds.h = DOCUMENT_SPATIAL_CELL_SIZE;
  }

  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document->spatial_cols = (int)ceilf(bounds.w / document->spatial_cell_size);
  document->spatial_rows = (int)ceilf(bounds.h / document->spatial_cell_size);
  if (document->spatial_cols < 1) {
    document->spatial_cols = 1;
  }
  if (document->spatial_rows < 1) {
    document->spatial_rows = 1;
  }
  if (document->spatial_cols > DOCUMENT_SPATIAL_MAX_AXIS_CELLS) {
    document->spatial_cols = DOCUMENT_SPATIAL_MAX_AXIS_CELLS;
  }
  if (document->spatial_rows > DOCUMENT_SPATIAL_MAX_AXIS_CELLS) {
    document->spatial_rows = DOCUMENT_SPATIAL_MAX_AXIS_CELLS;
  }
  document->spatial_cell_count = document->spatial_cols * document->spatial_rows;
  document->spatial_bounds = bounds;

  {
    int *heads = (int *)realloc(document->spatial_heads,
                                (size_t)document->spatial_cell_count *
                                    sizeof(document->spatial_heads[0]));
    unsigned int *marks =
        (unsigned int *)realloc(document->spatial_marks,
                                (size_t)document->count *
                                    sizeof(document->spatial_marks[0]));
    if (!heads || !marks) {
      if (heads) {
        document->spatial_heads = heads;
      }
      if (marks) {
        document->spatial_marks = marks;
      }
      document_spatial_free(document);
      return 0;
    }
    document->spatial_heads = heads;
    document->spatial_marks = marks;
    document->spatial_mark_capacity = document->count;
  }

  memset(document->spatial_heads, 0xff,
         (size_t)document->spatial_cell_count * sizeof(document->spatial_heads[0]));
  memset(document->spatial_marks, 0,
         (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
  document->spatial_query_token = 0u;

  for (i = 0; i < document->count; ++i) {
    RectF object_bounds;
    int min_col = 0;
    int max_col = 0;
    int min_row = 0;
    int max_row = 0;
    int row = 0;
    int col = 0;

    if (!document->objects[i]) {
      continue;
    }

    object_bounds = object_get_bounds(document->objects[i]);
    min_col = (int)floorf((object_bounds.x - bounds.x) / document->spatial_cell_size);
    max_col = (int)floorf((rectf_right(&object_bounds) - bounds.x) /
                          document->spatial_cell_size);
    min_row = (int)floorf((object_bounds.y - bounds.y) / document->spatial_cell_size);
    max_row = (int)floorf((rectf_top(&object_bounds) - bounds.y) /
                          document->spatial_cell_size);
    if (min_col < 0) {
      min_col = 0;
    }
    if (min_row < 0) {
      min_row = 0;
    }
    if (max_col >= document->spatial_cols) {
      max_col = document->spatial_cols - 1;
    }
    if (max_row >= document->spatial_rows) {
      max_row = document->spatial_rows - 1;
    }

    for (row = min_row; row <= max_row; ++row) {
      for (col = min_col; col <= max_col; ++col) {
        if (!document_spatial_add_entry(document, row * document->spatial_cols + col, i)) {
          document_spatial_free(document);
          return 0;
        }
      }
    }
  }

  document->spatial_revision = document->revision;
  return 1;
}

static LayerId document_resolve_layer_id(const Document *document, LayerId layer_id) {
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

static int rect_intersects(const RectF *a, const RectF *b) {
  if (!a || !b) {
    return 0;
  }

  return !(rectf_right(a) < b->x || rectf_right(b) < a->x || rectf_top(a) < b->y ||
           rectf_top(b) < a->y);
}

static int document_layer_has_objects(const Document *document, LayerId layer_id) {
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

void document_init(Document *document) {
  if (!document) {
    return;
  }

  memset(document, 0, sizeof(*document));
  document->next_id = 1u;
  document->revision = 1u;
  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document_reset_layers(document);
}

void document_shutdown(Document *document) {
  if (!document) {
    return;
  }

  document_clear_objects(document);
  document_spatial_free(document);
  free(document->layers);
  document->layers = NULL;
  document->layer_count = 0;
  document->layer_capacity = 0;
  document->active_layer_id = 0u;
  document->next_layer_id = 0u;
  document->revision++;
}

void document_reset(Document *document) {
  if (!document) {
    return;
  }

  document_clear_objects(document);
  document_spatial_free(document);
  document->next_id = 1u;
  document->revision = 1u;
  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document_reset_layers(document);
}

int document_add_object_to_layer(Document *document,
                                 GraphicObject *object,
                                 LayerId layer_id) {
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

int document_add_object(Document *document, GraphicObject *object) {
  return document_add_object_to_layer(document, object,
                                      document ? document->active_layer_id : 0u);
}

int document_append_object_with_id_to_layer(Document *document,
                                            GraphicObject *object,
                                            ObjectId id,
                                            LayerId layer_id) {
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

int document_append_object_with_id(Document *document, GraphicObject *object,
                                   ObjectId id) {
  return document_append_object_with_id_to_layer(
      document, object, id, object ? object->layer_id : 0u);
}

GraphicObject *document_find_object(const Document *document, ObjectId id) {
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

GraphicObject *document_get_object_at(const Document *document, int index) {
  if (!document || index < 0 || index >= document->count) {
    return NULL;
  }
  return document->objects[index];
}

int document_remove_object(Document *document, ObjectId id) {
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

void document_touch(Document *document) {
  if (document) {
    document->revision++;
    document_spatial_invalidate(document);
  }
}

ObjectId document_max_id(const Document *document) {
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

int document_layer_count(const Document *document) {
  return document ? document->layer_count : 0;
}

const DocumentLayer *document_layer_at(const Document *document, int index) {
  if (!document || index < 0 || index >= document->layer_count) {
    return NULL;
  }
  return &document->layers[index];
}

int document_layer_index(const Document *document, LayerId layer_id) {
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

DocumentLayer *document_layer_find(Document *document, LayerId layer_id) {
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

const DocumentLayer *document_layer_find_const(const Document *document,
                                               LayerId layer_id) {
  return document_layer_find((Document *)document, layer_id);
}

int document_layer_is_locked(const Document *document, LayerId layer_id) {
  const DocumentLayer *layer = document_layer_find_const(document, layer_id);
  return layer ? layer->locked : 0;
}

int document_object_is_locked(const Document *document, ObjectId object_id) {
  const GraphicObject *object = document_find_object(document, object_id);
  return object ? document_layer_is_locked(document, object->layer_id) : 0;
}

LayerId document_active_layer_id(const Document *document) {
  return document ? document->active_layer_id : 0u;
}

int document_set_active_layer(Document *document, LayerId layer_id) {
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

LayerId document_create_layer(Document *document, const char *name) {
  DocumentLayer *layer = NULL;

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

LayerId document_create_layer_with_id(Document *document,
                                      const char *name,
                                      LayerId layer_id) {
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

int document_insert_layer_at(Document *document,
                             const DocumentLayer *layer,
                             int index) {
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

int document_delete_layer(Document *document, LayerId layer_id) {
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

int document_rename_layer(Document *document, LayerId layer_id, const char *name) {
  DocumentLayer *layer = NULL;
  char normalized_name[sizeof(((DocumentLayer *)0)->name)];

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

int document_move_layer(Document *document, LayerId layer_id, int target_index) {
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

int document_set_layer_visibility(Document *document,
                                  LayerId layer_id,
                                  int visible) {
  DocumentLayer *layer = document_layer_find(document, layer_id);
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

int document_set_layer_locked(Document *document, LayerId layer_id, int locked) {
  DocumentLayer *layer = document_layer_find(document, layer_id);
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

int document_set_layer_blend_mode(Document *document,
                                  LayerId layer_id,
                                  DocumentLayerBlendMode blend_mode) {
  DocumentLayer *layer = document_layer_find(document, layer_id);
  if (!layer) {
    return 0;
  }
  layer->blend_mode = blend_mode;
  document->revision++;
  return 1;
}

int document_insert_object_clone_at(Document *document,
                                    const GraphicObject *snapshot,
                                    int index) {
  GraphicObject *clone = NULL;
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

int document_query_visible_indices(const Document *document,
                                   RectF visible_rect,
                                   int *out_indices,
                                   int max_indices) {
  Document *mutable_document = (Document *)document;
  int count = 0;
  int cell_min_col = 0;
  int cell_max_col = 0;
  int cell_min_row = 0;
  int cell_max_row = 0;
  int row = 0;
  int col = 0;

  if (!document || !out_indices || max_indices <= 0) {
    return 0;
  }

  if (!document_spatial_rebuild(mutable_document) || document->count <= 0 ||
      document->spatial_cell_count <= 0) {
    return 0;
  }

  if (document->spatial_query_token == 0xffffffffu) {
    memset(document->spatial_marks, 0,
           (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
    mutable_document->spatial_query_token = 1u;
  } else {
    mutable_document->spatial_query_token++;
  }

  cell_min_col = (int)floorf((visible_rect.x - document->spatial_bounds.x) /
                             document->spatial_cell_size);
  cell_max_col = (int)floorf((rectf_right(&visible_rect) - document->spatial_bounds.x) /
                             document->spatial_cell_size);
  cell_min_row = (int)floorf((visible_rect.y - document->spatial_bounds.y) /
                             document->spatial_cell_size);
  cell_max_row = (int)floorf((rectf_top(&visible_rect) - document->spatial_bounds.y) /
                             document->spatial_cell_size);
  if (cell_min_col < 0) {
    cell_min_col = 0;
  }
  if (cell_min_row < 0) {
    cell_min_row = 0;
  }
  if (cell_max_col >= document->spatial_cols) {
    cell_max_col = document->spatial_cols - 1;
  }
  if (cell_max_row >= document->spatial_rows) {
    cell_max_row = document->spatial_rows - 1;
  }

  for (row = cell_min_row; row <= cell_max_row; ++row) {
    for (col = cell_min_col; col <= cell_max_col; ++col) {
      int entry_index = document->spatial_heads[row * document->spatial_cols + col];
      while (entry_index >= 0) {
        int object_index = document->spatial_entries[entry_index].object_index;
        const GraphicObject *object = document->objects[object_index];
        const DocumentLayer *layer = object ? document_layer_find_const(document, object->layer_id)
                                            : NULL;

        if (object && document->spatial_marks[object_index] != document->spatial_query_token &&
            layer && layer->visible) {
          RectF object_bounds = object_get_bounds(object);
          if (!rect_intersects(&visible_rect, &object_bounds)) {
            entry_index = document->spatial_entries[entry_index].next;
            continue;
          }
          document->spatial_marks[object_index] = document->spatial_query_token;
          if (count < max_indices) {
            out_indices[count++] = object_index;
          }
        }
        entry_index = document->spatial_entries[entry_index].next;
      }
    }
  }

  return count;
}

int document_query_point_indices(const Document *document,
                                 Vec2 point,
                                 float tolerance,
                                 int *out_indices,
                                 int max_indices) {
  RectF query_rect;

  query_rect.x = point.x - tolerance;
  query_rect.y = point.y - tolerance;
  query_rect.w = tolerance * 2.0f;
  query_rect.h = tolerance * 2.0f;
  return document_query_visible_indices(document, query_rect, out_indices, max_indices);
}
