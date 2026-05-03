/**
 * @file document.h
 * @brief Document object container interface.
 */
#ifndef GLDRAW_DOCUMENT_DOCUMENT_H
#define GLDRAW_DOCUMENT_DOCUMENT_H

#include <document/object.h>
#include <model/selection.h>

typedef enum {
  DOCUMENT_LAYER_BLEND_NORMAL = 0,
  DOCUMENT_LAYER_BLEND_MULTIPLY,
  DOCUMENT_LAYER_BLEND_SCREEN
} DocumentLayerBlendMode;

typedef struct DocumentLayer {
  LayerId id;
  char name[32];
  int visible;
  int locked;
  DocumentLayerBlendMode blend_mode;
} DocumentLayer;

typedef struct DocumentSpatialEntry {
  int object_index;
  int next;
} DocumentSpatialEntry;

/**
 * @struct Document
 * @brief Document main data structure.
 *
 * @member objects Object pointer array (document owns the lifetime).
 * @member count Current object count.
 * @member revision Document revision number (incremented after edits).
 * @member next_id Auto-assigned ID for new objects.
 */
typedef struct Document {
  GraphicObject **objects;
  int count;
  int capacity;
  unsigned int revision;
  ObjectId next_id;
  DocumentLayer *layers;
  int layer_count;
  int layer_capacity;
  LayerId active_layer_id;
  LayerId next_layer_id;
  RectF spatial_bounds;
  float spatial_cell_size;
  int spatial_cols;
  int spatial_rows;
  int spatial_cell_count;
  int *spatial_heads;
  DocumentSpatialEntry *spatial_entries;
  int spatial_entry_count;
  int spatial_entry_capacity;
  unsigned int spatial_revision;
  unsigned int spatial_query_token;
  unsigned int *spatial_marks;
  int spatial_mark_capacity;
} Document;

/**
 * @brief Initialize the document to a fresh empty state.
 * @param document Document to initialize.
 * @return No return value.
 */
void document_init(Document *document);

/**
 * @brief Release all objects owned by the document and clear runtime state.
 * @param document Target document.
 * @return No return value.
 */
void document_shutdown(Document *document);

/**
 * @brief Reset the document to a "new document" state.
 * @param document Target document.
 * @return No return value.
 * @note Unlike `document_shutdown`, this function resets `revision` and `next_id` to initial values.
 */
void document_reset(Document *document);

/**
 * @brief Append an object to the document and auto-assign an ID.
 * @param document Target document.
 * @param object Object to insert.
 * @return Non-zero on success; zero on capacity error or invalid parameters.
 */
int document_add_object(Document *document, GraphicObject *object);

int document_add_object_to_layer(Document *document,
                                 GraphicObject *object,
                                 LayerId layer_id);

/**
 * @brief Append an object to the document with a specified ID.
 * @param document Target document.
 * @param object Object to insert.
 * @param id Specified object ID.
 * @return Non-zero on success; zero on ID conflict, invalid parameters, or capacity error.
 */
int document_append_object_with_id(Document *document, GraphicObject *object,
                                   ObjectId id);

int document_append_object_with_id_to_layer(Document *document,
                                            GraphicObject *object,
                                            ObjectId id,
                                            LayerId layer_id);

/**
 * @brief Find an object by its ID.
 * @param document Document.
 * @param id Object ID.
 * @return Object pointer if found, `NULL` otherwise.
 */
GraphicObject *document_find_object(const Document *document, ObjectId id);

/**
 * @brief Get an object by array index.
 * @param document Document.
 * @param index Object index.
 * @return Object pointer if index is valid, `NULL` otherwise.
 */
GraphicObject *document_get_object_at(const Document *document, int index);

/**
 * @brief Remove an object by ID and compact the object array.
 * @param document Target document.
 * @param id Object ID to remove.
 * @return Non-zero on successful deletion, zero otherwise.
 */
int document_remove_object(Document *document, ObjectId id);

/**
 * @brief Mark a non-structural modification (increments revision).
 * @param document Target document.
 * @return No return value.
 */
void document_touch(Document *document);

/**
 * @brief Compute the maximum object ID in the document.
 * @param document Document.
 * @return Maximum ID; returns `0` if the document is empty or parameters are invalid.
 */
ObjectId document_max_id(const Document *document);

int document_layer_count(const Document *document);
const DocumentLayer *document_layer_at(const Document *document, int index);
int document_layer_index(const Document *document, LayerId layer_id);
DocumentLayer *document_layer_find(Document *document, LayerId layer_id);
const DocumentLayer *document_layer_find_const(const Document *document,
                                               LayerId layer_id);
int document_layer_is_locked(const Document *document, LayerId layer_id);
int document_object_is_locked(const Document *document, ObjectId object_id);
LayerId document_active_layer_id(const Document *document);
int document_set_active_layer(Document *document, LayerId layer_id);
LayerId document_create_layer(Document *document, const char *name);
LayerId document_create_layer_with_id(Document *document,
                                      const char *name,
                                      LayerId layer_id);
int document_insert_layer_at(Document *document,
                             const DocumentLayer *layer,
                             int index);
int document_delete_layer(Document *document, LayerId layer_id);
int document_rename_layer(Document *document, LayerId layer_id, const char *name);
int document_move_layer(Document *document, LayerId layer_id, int target_index);
int document_set_layer_visibility(Document *document,
                                  LayerId layer_id,
                                  int visible);
int document_set_layer_locked(Document *document, LayerId layer_id, int locked);
int document_set_layer_blend_mode(Document *document,
                                  LayerId layer_id,
                                  DocumentLayerBlendMode blend_mode);
int document_insert_object_clone_at(Document *document,
                                    const GraphicObject *snapshot,
                                    int index);
int document_query_visible_indices(const Document *document,
                                   RectF visible_rect,
                                   int *out_indices,
                                   int max_indices);
int document_query_point_indices(const Document *document,
                                 Vec2 point,
                                 float tolerance,
                                 int *out_indices,
                                 int max_indices);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_H */
