#include <model/selection.h>

#include <stdlib.h>
#include <string.h>

static void selection_set_touch(SelectionSet* selection)
{
    if (selection) {
        selection->revision++;
    }
}

void selection_set_init(SelectionSet* selection)
{
    if (!selection) {
        return;
    }

    memset(selection, 0, sizeof(*selection));
}

void selection_set_shutdown(SelectionSet* selection)
{
    if (!selection) {
        return;
    }

    free(selection->ids);
    selection_set_init(selection);
}

void selection_set_clear(SelectionSet* selection)
{
    if (selection && selection->count > 0) {
        selection->count = 0;
        selection_set_touch(selection);
    }
}

int selection_set_reserve(SelectionSet* selection, int needed)
{
    ObjectId* ids = NULL;
    int capacity = 0;

    if (!selection || needed <= 0) {
        return selection != NULL;
    }
    if (needed <= selection->capacity) {
        return 1;
    }

    capacity = selection->capacity > 0 ? selection->capacity : 8;
    while (capacity < needed) {
        capacity *= 2;
    }

    ids = (ObjectId*)realloc(selection->ids, (size_t)capacity * sizeof(selection->ids[0]));
    if (!ids) {
        return 0;
    }

    selection->ids = ids;
    selection->capacity = capacity;
    return 1;
}

int selection_set_copy(SelectionSet* dst, const SelectionSet* src)
{
    size_t size = 0u;

    if (!dst || !src) {
        return 0;
    }
    if (dst == src) {
        return 1;
    }
    if (src->count <= 0) {
        selection_set_clear(dst);
        return 1;
    }
    if (!selection_set_reserve(dst, src->count)) {
        return 0;
    }

    size = (size_t)src->count * sizeof(src->ids[0]);
    if (dst->count == src->count && memcmp(dst->ids, src->ids, size) == 0) {
        return 1;
    }

    memcpy(dst->ids, src->ids, size);
    dst->count = src->count;
    selection_set_touch(dst);
    return 1;
}

int selection_set_contains(const SelectionSet* selection, ObjectId id)
{
    int i = 0;

    if (!selection || id == 0u) {
        return 0;
    }

    for (i = 0; i < selection->count; ++i) {
        if (selection->ids[i] == id) {
            return 1;
        }
    }

    return 0;
}

int selection_set_add(SelectionSet* selection, ObjectId id)
{
    if (!selection || id == 0u) {
        return 0;
    }
    if (selection_set_contains(selection, id)) {
        return 1;
    }
    if (!selection_set_reserve(selection, selection->count + 1)) {
        return 0;
    }

    selection->ids[selection->count++] = id;
    selection_set_touch(selection);
    return 1;
}

void selection_set_remove(SelectionSet* selection, ObjectId id)
{
    int i = 0;

    if (!selection || id == 0u) {
        return;
    }

    for (i = 0; i < selection->count; ++i) {
        if (selection->ids[i] == id) {
            memmove(&selection->ids[i],
                    &selection->ids[i + 1],
                    (size_t)(selection->count - i - 1) * sizeof(selection->ids[0]));
            selection->count--;
            selection_set_touch(selection);
            return;
        }
    }
}

void selection_set_toggle(SelectionSet* selection, ObjectId id)
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
