/**
 * @file editor_session.h
 * @brief Editor-session-only state helpers.
 */
#ifndef GLDRAW_APP_EDITOR_SESSION_H
#define GLDRAW_APP_EDITOR_SESSION_H

#include <document/document.h>

#include <string.h>

/**
 * @brief Clear a selection set.
 * @param selection Selection set to clear.
 * @return No return value.
 */
static inline void selection_set_clear(SelectionSet* selection)
{
    if (selection) {
        selection->count = 0;
    }
}

/**
 * @brief Check whether an object ID is selected.
 * @param selection Selection set.
 * @param id Object ID.
 * @return Non-zero if selected, zero otherwise.
 */
static inline int selection_set_contains(const SelectionSet* selection, ObjectId id)
{
    int i = 0;

    if (!selection || id == 0) {
        return 0;
    }

    for (i = 0; i < selection->count; ++i) {
        if (selection->ids[i] == id) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Add an object ID to a selection set.
 * @param selection Selection set.
 * @param id Object ID.
 * @return Non-zero on success, zero on invalid parameters or capacity overflow.
 */
static inline int selection_set_add(SelectionSet* selection, ObjectId id)
{
    if (!selection || id == 0) {
        return 0;
    }

    if (selection_set_contains(selection, id)) {
        return 1;
    }

    if (selection->count >= DOCUMENT_MAX_SELECTION) {
        return 0;
    }

    selection->ids[selection->count++] = id;
    return 1;
}

/**
 * @brief Remove an object ID from a selection set.
 * @param selection Selection set.
 * @param id Object ID.
 * @return No return value.
 */
static inline void selection_set_remove(SelectionSet* selection, ObjectId id)
{
    int i = 0;

    if (!selection || id == 0) {
        return;
    }

    for (i = 0; i < selection->count; ++i) {
        if (selection->ids[i] == id) {
            memmove(&selection->ids[i],
                    &selection->ids[i + 1],
                    (size_t)(selection->count - i - 1) * sizeof(selection->ids[0]));
            selection->count--;
            return;
        }
    }
}

/**
 * @brief Toggle an object ID in a selection set.
 * @param selection Selection set.
 * @param id Object ID.
 * @return No return value.
 */
static inline void selection_set_toggle(SelectionSet* selection, ObjectId id)
{
    if (!selection) {
        return;
    }

    if (selection_set_contains(selection, id)) {
        selection_set_remove(selection, id);
    } else {
        selection_set_add(selection, id);
    }
}

/**
 * @brief Resolve the primary selected object from a document.
 * @param selection Selection set.
 * @param document Document to search.
 * @return Selected object or `NULL`.
 */
static inline GraphicObject* selection_set_primary_object(const SelectionSet* selection,
                                                          const Document* document)
{
    if (!selection || !document || selection->count <= 0) {
        return NULL;
    }

    return document_find_object(document, selection->ids[0]);
}

#endif /* GLDRAW_APP_EDITOR_SESSION_H */
