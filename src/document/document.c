/**
 * @file document.c
 * @brief In-memory document storage.
 *
 * Role in project:
 * - Owns bounded object array mutation logic.
 * - Maintains revision counters used for dirty tracking and history commits.
 *
 * Module relationships:
 * - Uses object create/destroy helpers from `object.c`.
 * - Used by tools, UI, history, persistence, and renderer.
 */
#include <document/document.h>

#include <string.h>

/**
 * @brief Initializes a document.
 * @param document Document to initialize.
 * @return None.
 */
void document_init(Document* document)
{
    if (!document) {
        return;
    }

    memset(document, 0, sizeof(*document));
    document->next_id = 1;
    document->revision = 1;
}

/**
 * @brief Shuts down a document and frees resources.
 * @param document Document to shut down.
 * @return None.
 */
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
    document->revision++;
}

/**
 * @brief Resets a document to initial empty state.
 * @param document Document to reset.
 * @return None.
 */
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
    document->next_id = 1;
    document->revision = 1;
}

/**
 * @brief Adds an object to the document.
 * @param document Document instance.
 * @param object Object to add.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Appends an object with a specific ID.
 * @param document Document instance.
 * @param object Object to append.
 * @param id Desired object ID.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Finds an object by ID.
 * @param document Document instance.
 * @param id Object ID to find.
 * @return Pointer to object or NULL.
 */
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

/**
 * @brief Gets an object at a specific index.
 * @param document Document instance.
 * @param index Object index.
 * @return Pointer to object or NULL.
 */
GraphicObject* document_get_object_at(const Document* document, int index)
{
    if (!document || index < 0 || index >= document->count) {
        return NULL;
    }
    return document->objects[index];
}

/**
 * @brief Removes an object from the document.
 * @param document Document instance.
 * @param id Object ID to remove.
 * @return 1 on success, 0 on failure.
 */
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
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Increments the document revision.
 * @param document Document instance.
 * @return None.
 */
void document_touch(Document* document)
{
    if (document) {
        document->revision++;
    }
}

/**
 * @brief Gets the maximum object ID in the document.
 * @param document Document instance.
 * @return Maximum object ID.
 */
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
