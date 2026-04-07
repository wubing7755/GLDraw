#include <core/select_tool.h>
#include <core/shape.h>
#include <core/shape_manager.h>
#include <stdlib.h>

struct SelectToolCtx {
    int dragging;        /* currently dragging? */
    float drag_start[2]; /* mouse pos where drag started */
    float shape_start[2];/* shape pos where drag started (for one shape) */
};

static const char* select_tool_name(const Tool* t)
{
    (void)t;
    return "SELECT";
}

static Shape* find_shape_at(float x, float y)
{
    /* Iterate shapes in reverse (top shapes first) */
    int count = sm_count();
    for (int i = count - 1; i >= 0; i--) {
        Shape* s = sm_get(i);
        if (shape_hit_test(s, x, y, 0.05f)) {
            return s;
        }
    }
    return NULL;
}

static void select_tool_on_down(Tool* t, float x, float y, SelectionManager* sel, int shift_held)
{
    SelectToolCtx* ctx = (SelectToolCtx*)t->ctx;

    Shape* hit = find_shape_at(x, y);

    if (hit) {
        if (sel_contains(sel, hit)) {
            /* Already selected — start drag */
            ctx->dragging = 1;
            ctx->drag_start[0] = x;
            ctx->drag_start[1] = y;
            ctx->shape_start[0] = x;
            ctx->shape_start[1] = y;
        } else {
            /* Not selected yet */
            if (shift_held) {
                /* Shift + click: toggle multi-select */
                sel_toggle(sel, hit);
            } else {
                /* Normal click: clear and select only this */
                sel_clear(sel);
                sel_add(sel, hit);
            }
            ctx->dragging = 1;
            ctx->drag_start[0] = x;
            ctx->drag_start[1] = y;
            ctx->shape_start[0] = x;
            ctx->shape_start[1] = y;
        }
    } else {
        /* Clicked empty space — clear selection if shift not held */
        if (!shift_held) {
            sel_clear(sel);
        }
        ctx->dragging = 0;
    }
}

static void select_tool_on_move(Tool* t, float x, float y, SelectionManager* sel)
{
    (void)sel;
    SelectToolCtx* ctx = (SelectToolCtx*)t->ctx;

    if (!ctx->dragging) return;

    float dx = x - ctx->shape_start[0];
    float dy = y - ctx->shape_start[1];

    /* Translate all selected shapes */
    int count = sel_count(sel);
    for (int i = 0; i < count; i++) {
        Shape* s = sel_get(sel, i);
        shape_translate(s, dx, dy);
    }

    ctx->shape_start[0] = x;
    ctx->shape_start[1] = y;
}

static void select_tool_on_up(Tool* t, SelectionManager* sel)
{
    (void)t;
    (void)sel;
    SelectToolCtx* ctx = (SelectToolCtx*)t->ctx;
    ctx->dragging = 0;
}

static ToolVTable select_tool_vtable = {
    .name = select_tool_name,
    .on_down = select_tool_on_down,
    .on_move = select_tool_on_move,
    .on_up = select_tool_on_up,
};

Tool* select_tool_create(void)
{
    Tool* t = (Tool*)malloc(sizeof(Tool));
    if (!t) return NULL;

    SelectToolCtx* ctx = (SelectToolCtx*)malloc(sizeof(SelectToolCtx));
    if (!ctx) { free(t); return NULL; }

    ctx->dragging = 0;
    ctx->drag_start[0] = ctx->drag_start[1] = 0.0f;
    ctx->shape_start[0] = ctx->shape_start[1] = 0.0f;

    t->vtable = &select_tool_vtable;
    t->ctx = ctx;
    return t;
}

void select_tool_destroy(Tool* t)
{
    if (!t) return;
    free(t->ctx);
    free(t);
}
