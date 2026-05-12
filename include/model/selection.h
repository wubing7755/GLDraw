/**
 * @file selection.h
 * @brief Editor selection set value type and helpers.
 */
#ifndef GLDRAW_MODEL_SELECTION_H
#define GLDRAW_MODEL_SELECTION_H

#include <stdint.h>

#include <base/types.h>

/**
 * @struct SelectionSet
 * @brief Selection set based on stable document object IDs.
 */
typedef struct {
    ObjectId* ids;
    int count;
    int capacity;
    uint64_t revision;
} SelectionSet;

void selection_set_init(SelectionSet* selection);
void selection_set_shutdown(SelectionSet* selection);
void selection_set_clear(SelectionSet* selection);
int selection_set_reserve(SelectionSet* selection, int needed);
int selection_set_copy(SelectionSet* dst, const SelectionSet* src);
int selection_set_contains(const SelectionSet* selection, ObjectId id);
int selection_set_add(SelectionSet* selection, ObjectId id);
void selection_set_remove(SelectionSet* selection, ObjectId id);
void selection_set_toggle(SelectionSet* selection, ObjectId id);

#endif /* GLDRAW_MODEL_SELECTION_H */
