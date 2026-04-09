# Selection System

## Overview

The selection system tracks which shapes are currently selected, supporting both single and multi-selection.

## SelectionManager

```c
typedef struct {
    Shape* shapes[64];   // Selected shapes
    size_t count;        // Number selected
} SelectionManager;
```

**Maximum selections**: 64 shapes

## API

```c
// Initialize
void sel_init(SelectionManager* sel);

// Add shape to selection
void sel_add(SelectionManager* sel, Shape* shape);

// Remove shape from selection
void sel_remove(SelectionManager* sel, Shape* shape);

// Toggle shape selection (add if not present, remove if present)
void sel_toggle(SelectionManager* sel, Shape* shape);

// Clear all selections
void sel_clear(SelectionManager* sel);

// Check if shape is selected
bool sel_contains(const SelectionManager* sel, const Shape* shape);

// Get selected shape at index
Shape* sel_get(const SelectionManager* sel, size_t index);
```

## Multi-Selection

- **Shift+Click** with SelectTool toggles individual shapes
- **Click on empty** clears all selections
- **Drag selected shape** moves all selected shapes together

## Selection Highlighting

During rendering, selected shapes are drawn with a yellow outline (indicating they can be moved/deleted). The renderer checks `sel_contains()` for each shape.

## Key Files

- `include/core/selection_manager.h` - Public API
- `src/selection_manager.c` - Implementation
