#include <core/selection_manager.h>
#include <string.h>

void sel_init(SelectionManager* sel)
{
    sel->count = 0;
}

void sel_clear(SelectionManager* sel)
{
    sel->count = 0;
}

void sel_add(SelectionManager* sel, Shape* s)
{
    if (sel->count >= SELECTION_MAX) return;
    if (sel_contains(sel, s)) return;
    sel->items[sel->count++] = s;
}

void sel_remove(SelectionManager* sel, Shape* s)
{
    for (int i = 0; i < sel->count; i++) {
        if (sel->items[i] == s) {
            /* Shift remaining items */
            for (int j = i; j < sel->count - 1; j++) {
                sel->items[j] = sel->items[j + 1];
            }
            sel->count--;
            return;
        }
    }
}

void sel_toggle(SelectionManager* sel, Shape* s)
{
    if (sel_contains(sel, s)) {
        sel_remove(sel, s);
    } else {
        sel_add(sel, s);
    }
}

int sel_count(const SelectionManager* sel)
{
    return sel->count;
}

Shape* sel_get(const SelectionManager* sel, int index)
{
    if (index < 0 || index >= sel->count) return NULL;
    return sel->items[index];
}

int sel_contains(const SelectionManager* sel, Shape* s)
{
    for (int i = 0; i < sel->count; i++) {
        if (sel->items[i] == s) return 1;
    }
    return 0;
}
