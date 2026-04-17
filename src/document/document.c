/**
 * @file document.c
 * @brief In-memory document storage and selection operations.
 *
 * Role in project:
 * - Owns bounded object array and selection list mutation logic.
 * - Maintains revision counters used for dirty tracking and history commits.
 *
 * Module relationships:
 * - Uses object create/destroy helpers from `object.c`.
 * - Used by tools, UI, history, persistence, and renderer.
 */
#include <document/document.h>

#include <string.h>

/** Initialize document fields to empty defaults. Complexity: `O(1)`. */
void document_init(Document* document)
{
    if (!document) {
        return;
    }

    memset(document, 0, sizeof(*document));
    document->next_id = 1;
    document->revision = 1;
}

/** Destroy all objects and clear selection while preserving struct storage. Complexity: `O(n)`. */
void document_shutdown(Document* document)
{
    int i = 0;

    if (!document) {
        return;
    }

    for (i = 0; i < document->count; ++i) {
        object_destroy(document->objects[i]);
        document->objects[i] = NULL;
    }

    document->count = 0;
    document->selection.count = 0;
    document->revision++;
}

/** Reset document to pristine empty state. Complexity: `O(n)`. */
void document_reset(Document* document)
{
    int i = 0;

    if (!document) {
        return;
    }

    for (i = 0; i < document->count; ++i) {
        object_destroy(document->objects[i]);
        document->objects[i] = NULL;
    }

    document->count = 0;
    document->selection.count = 0;
    document->next_id = 1;
    document->revision = 1;
}

/** Append object with auto-assigned ID. Complexity: `O(1)`. */
int document_add_object(Document* document, GraphicObject* object)
{
    if (!document || !object || document->count >= DOCUMENT_MAX_OBJECTS) {
        return 0;
    }

    object->id = document->next_id++;
    document->objects[document->count++] = object;
    document->revision++;
    return 1;
}

/** Append object with explicit ID and duplicate check. Complexity: `O(n)`. */
int document_append_object_with_id(Document* document, GraphicObject* object, ObjectId id)
{
    if (!document || !object || id == 0 || document->count >= DOCUMENT_MAX_OBJECTS) {
        return 0;
    }

    if (document_find_object(document, id)) {
        return 0;
    }

    object->id = id;
    document->objects[document->count++] = object;
    if (document->next_id <= id) {
        document->next_id = id + 1;
    }
    document->revision++;
    return 1;
}

/** Find object by ID by linear scan. Complexity: `O(n)`. */
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

/** Access object by index. Complexity: `O(1)`. */
GraphicObject* document_get_object_at(const Document* document, int index)
{
    if (!document || index < 0 || index >= document->count) {
        return NULL;
    }
    return document->objects[index];
}

/** Check whether ID is in selection set. Complexity: `O(s)`. */
int document_selection_contains(const Document* document, ObjectId id)
{
    int i = 0;

    if (!document) {
        return 0;
    }

    for (i = 0; i < document->selection.count; ++i) {
        if (document->selection.ids[i] == id) {
            return 1;
        }
    }

    return 0;
}

/** Clear selection count only (IDs remain irrelevant beyond count). Complexity: `O(1)`. */
void document_clear_selection(Document* document)
{
    if (document) {
        document->selection.count = 0;
    }
}

/** Add ID to selection if missing and capacity permits. Complexity: `O(s)`. */
int document_selection_add(Document* document, ObjectId id)
{
    if (!document || id == 0) {
        return 0;
    }

    if (document_selection_contains(document, id)) {
        return 1;
    }

    if (document->selection.count >= DOCUMENT_MAX_SELECTION) {
        return 0;
    }

    document->selection.ids[document->selection.count++] = id;
    return 1;
}

/** Remove one ID from selection and compact array. Complexity: `O(s)`. */
void document_selection_remove(Document* document, ObjectId id)
{
    int i = 0;

    if (!document) {
        return;
    }

    for (i = 0; i < document->selection.count; ++i) {
        if (document->selection.ids[i] == id) {
            int j = 0;
            for (j = i; j < document->selection.count - 1; ++j) {
                document->selection.ids[j] = document->selection.ids[j + 1];
            }
            document->selection.count--;
            return;
        }
    }
}

/** Toggle selection membership for ID. Complexity: `O(s)`. */
void document_selection_toggle(Document* document, ObjectId id)
{
    if (!document) {
        return;
    }

    if (document_selection_contains(document, id)) {
        document_selection_remove(document, id);
    } else {
        document_selection_add(document, id);
    }
}

/** Resolve first selected object. Complexity: `O(n)` due to ID lookup. */
GraphicObject* document_primary_selection(const Document* document)
{
    if (!document || document->selection.count <= 0) {
        return NULL;
    }
    return document_find_object(document, document->selection.ids[0]);
}

/** Remove object by ID, destroy it, and compact object array. Complexity: `O(n)`. */
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
            document_selection_remove(document, id);
            document->revision++;
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Delete all currently selected objects.
 *
 * Why copy IDs first:
 * - Removing objects mutates selection/order, so direct iteration over
 *   `selection.ids` while deleting would skip or misread entries.
 *
 * Complexity: up to `O(s*n)`.
 */
void document_delete_selection(Document* document)
{
    ObjectId ids[DOCUMENT_MAX_SELECTION];
    int count = 0;
    int i = 0;

    if (!document) {
        return;
    }

    count = document->selection.count;
    for (i = 0; i < count; ++i) {
        ids[i] = document->selection.ids[i];
    }

    for (i = 0; i < count; ++i) {
        document_remove_object(document, ids[i]);
    }

    document_clear_selection(document);
}

/** Increment document revision marker. Complexity: `O(1)`. */
void document_touch(Document* document)
{
    if (document) {
        document->revision++;
    }
}

/** Return max object ID currently present. Complexity: `O(n)`. */
ObjectId document_max_id(const Document* document)
{
    ObjectId max_id = 0;
    int i = 0;

    if (!document) {
        return 0;
    }

    for (i = 0; i < document->count; ++i) {
        if (document->objects[i] && document->objects[i]->id > max_id) {
            max_id = document->objects[i]->id;
        }
    }

    return max_id;
}
