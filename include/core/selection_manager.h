#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <core/shape.h>

/* =============================================================================
 * Phase 3: SelectionManager — manages selected shapes without using IDs
 *
 * Uses Shape* pointer array. No IDs — shapes are selected by pointer.
 * =============================================================================
 */

#define SELECTION_MAX 64

typedef struct {
    Shape* items[SELECTION_MAX];
    int count;
} SelectionManager;

void sel_init(SelectionManager* sel);
void sel_clear(SelectionManager* sel);
void sel_add(SelectionManager* sel, Shape* s);
void sel_remove(SelectionManager* sel, Shape* s);
void sel_toggle(SelectionManager* sel, Shape* s);
int sel_count(const SelectionManager* sel);
Shape* sel_get(const SelectionManager* sel, int index);
int sel_contains(const SelectionManager* sel, Shape* s);

#endif /* SELECTION_MANAGER_H */
