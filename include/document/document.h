/**
 * @file document.h
 * @brief Core in-memory document model for drawable objects and selection.
 *
 * Role in project:
 * - Owns object storage, stable IDs, selection state, and revision tracking.
 * - Acts as the authoritative data model for tools, UI, persistence, and render.
 *
 * Module relationships:
 * - Uses object APIs from `document/object.h`.
 * - Consumed by history, canvas picking, tool operations, and save/load.
 *
 * Concurrency note:
 * - This module is not thread-safe; callers must serialize access externally.
 */
#ifndef GLDRAW_DOCUMENT_DOCUMENT_H
#define GLDRAW_DOCUMENT_DOCUMENT_H

#include <document/object.h>

#define DOCUMENT_MAX_OBJECTS 1024
#define DOCUMENT_MAX_SELECTION 128

/** Bounded selection list of object IDs. */
typedef struct {
    ObjectId ids[DOCUMENT_MAX_SELECTION];
    int count;
} SelectionSet;

typedef struct Document {
    GraphicObject* objects[DOCUMENT_MAX_OBJECTS];
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} Document;

/** Initialize a document to empty state. Complexity: `O(1)`. */
void document_init(Document* document);
/** Destroy all owned objects and clear runtime state. Complexity: `O(n)`. */
void document_shutdown(Document* document);
/** Reset to pristine empty document (`revision=1`, `next_id=1`). Complexity: `O(n)`. */
void document_reset(Document* document);

/** Append object and assign a fresh ID; returns 0 on capacity/null input. Complexity: `O(1)`. */
int document_add_object(Document* document, GraphicObject* object);
/** Append object with explicit ID; fails on duplicate/invalid ID. Complexity: `O(n)`. */
int document_append_object_with_id(Document* document, GraphicObject* object, ObjectId id);
/** Find object by ID; returns `NULL` when not found. Complexity: `O(n)`. */
GraphicObject* document_find_object(const Document* document, ObjectId id);
/** Get object by index; returns `NULL` for invalid index/document. Complexity: `O(1)`. */
GraphicObject* document_get_object_at(const Document* document, int index);
/** Remove object by ID and compact array; returns 1 on success. Complexity: `O(n)`. */
int document_remove_object(Document* document, ObjectId id);
/** Delete all selected objects. Complexity: up to `O(s*n)` (`s`: selection count). */
void document_delete_selection(Document* document);
/** Bump revision for non-structural edits. Complexity: `O(1)`. */
void document_touch(Document* document);
/** Compute maximum existing object ID (0 when empty/invalid). Complexity: `O(n)`. */
ObjectId document_max_id(const Document* document);

/** Clear selection set. Complexity: `O(1)`. */
void document_clear_selection(Document* document);
/** Check if ID is selected. Complexity: `O(s)` (`s`: selection count). */
int document_selection_contains(const Document* document, ObjectId id);
/** Add ID to selection if not already present. Complexity: `O(s)`. */
int document_selection_add(Document* document, ObjectId id);
/** Remove ID from selection if present. Complexity: `O(s)`. */
void document_selection_remove(Document* document, ObjectId id);
/** Toggle selection state for an ID. Complexity: `O(s)`. */
void document_selection_toggle(Document* document, ObjectId id);
/** Get first selected object; returns `NULL` when none. Complexity: `O(n)` (ID lookup). */
GraphicObject* document_primary_selection(const Document* document);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_H */
