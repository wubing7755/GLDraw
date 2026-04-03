#ifndef TOOL_H
#define TOOL_H

/* =============================================================================
 * Phase 3: Tool abstraction — decouples interaction from shape creation
 *
 * Tool layer sits between input callbacks and ShapeManager.
 * Each tool (DrawTool, SelectTool) implements on_down/on_move/on_up.
 * =============================================================================
 */

#include <core/selection_manager.h>

typedef struct Tool Tool;
typedef struct ToolVTable ToolVTable;

struct ToolVTable {
    const char* (*name)(const Tool*);
    void (*on_down)(Tool*, float x, float y, SelectionManager* sel);
    void (*on_move)(Tool*, float x, float y, SelectionManager* sel);
    void (*on_up)(Tool*, SelectionManager* sel);
};

struct Tool {
    ToolVTable* vtable;
    void* ctx;  /* tool-specific context (DrawToolCtx*, SelectToolCtx*) */
};

const char* tool_name(const Tool* t);

#endif /* TOOL_H */
