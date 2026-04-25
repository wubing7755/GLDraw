/**
 * @file document.h
 * @brief Document object container and selection set operation interface.
 */
#ifndef GLDRAW_DOCUMENT_DOCUMENT_H
#define GLDRAW_DOCUMENT_DOCUMENT_H

#include <document/object.h>

#define DOCUMENT_MAX_OBJECTS 1024
#define DOCUMENT_MAX_SELECTION 128

/**
 * @struct SelectionSet
 * @brief Document selection set (based on object IDs).
 *
 * @member ids Selected object ID list.
 * @member count Current selection count.
 */
typedef struct {
  ObjectId ids[DOCUMENT_MAX_SELECTION];
  int count;
} SelectionSet;

/**
 * @struct Document
 * @brief Document main data structure.
 *
 * @member objects Object pointer array (document owns the lifetime).
 * @member count Current object count.
 * @member revision Document revision number (incremented after edits).
 * @member next_id Auto-assigned ID for new objects.
 * @member selection Current editor-session selection set (not serialized).
 */
typedef struct Document {
  GraphicObject *objects[DOCUMENT_MAX_OBJECTS];
  int count;
  unsigned int revision;
  ObjectId next_id;
  SelectionSet selection;
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

/**
 * @brief Append an object to the document with a specified ID.
 * @param document Target document.
 * @param object Object to insert.
 * @param id Specified object ID.
 * @return Non-zero on success; zero on ID conflict, invalid parameters, or capacity error.
 */
int document_append_object_with_id(Document *document, GraphicObject *object,
                                   ObjectId id);

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
 * @brief Delete all objects in the current selection set.
 * @param document Target document.
 * @return No return value.
 */
void document_delete_selection(Document *document);

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

/**
 * @brief Clear the selection set.
 * @param document Target document.
 * @return No return value.
 */
void document_clear_selection(Document *document);

/**
 * @brief Check whether an object ID is in the selection set.
 * @param document Document.
 * @param id Object ID.
 * @return Non-zero if in the selection set, zero otherwise.
 */
int document_selection_contains(const Document *document, ObjectId id);

/**
 * @brief Add an object ID to the selection set.
 * @param document Target document.
 * @param id Object ID.
 * @return Non-zero on success or if already present; zero on invalid parameters or capacity error.
 */
int document_selection_add(Document *document, ObjectId id);

/**
 * @brief Remove an object ID from the selection set.
 * @param document Target document.
 * @param id Object ID.
 * @return No return value.
 */
void document_selection_remove(Document *document, ObjectId id);

/**
 * @brief Toggle the selection state of an object ID.
 * @param document Target document.
 * @param id Object ID.
 * @return No return value.
 */
void document_selection_toggle(Document *document, ObjectId id);

/**
 * @brief Get the "primary selected object" (object corresponding to the first element of the selection set).
 * @param document Document.
 * @return Primary selected object; returns `NULL` if no selection.
 */
GraphicObject *document_primary_selection(const Document *document);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_H */
