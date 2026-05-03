/**
 * @file selection.h
 * @brief Editor selection set value type and helpers.
 */
#ifndef GLDRAW_MODEL_SELECTION_H
#define GLDRAW_MODEL_SELECTION_H

#include <base/types.h>

#include <string.h>

#define DOCUMENT_MAX_SELECTION 128

/**
 * @struct SelectionSet
 * @brief Selection set based on stable document object IDs.
 */
typedef struct {
    ObjectId ids[DOCUMENT_MAX_SELECTION];
    int count;
} SelectionSet;

static inline void selection_set_clear(SelectionSet* selection)
{
    if (selection) {
        selection->count = 0;
    }
}

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

#endif /* GLDRAW_MODEL_SELECTION_H */
