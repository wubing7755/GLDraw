#include <core/draw_tool.h>
#include <core/shape.h>
#include <core/shape_manager.h>
#include <core/shape_impl.h>
#include <core/shape_registry.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

struct DrawToolCtx {
    float p1[2];
    float p2[2];
    Shape* preview;
    char shape_type[16];
};

static const char* draw_tool_name(const Tool* t)
{
    (void)t;
    return "DRAW";
}

static void draw_tool_on_down(Tool* t, float x, float y, SelectionManager* sel)
{
    (void)sel;
    DrawToolCtx* ctx = (DrawToolCtx*)t->ctx;
    ctx->p1[0] = x; ctx->p1[1] = y;
    ctx->p2[0] = x; ctx->p2[1] = y;
    ctx->preview = NULL;

    /* Create initial preview */
    Shape* preview = shape_create(ctx->shape_type, 1.0f, 1.0f, 1.0f, 0.7f, 2.0f);
    if (!preview) return;

    if (strcmp(ctx->shape_type, "LINE") == 0) {
        LineImpl* line = (LineImpl*)preview->impl;
        line->p1[0] = x; line->p1[1] = y;
        line->p2[0] = x; line->p2[1] = y;
    } else if (strcmp(ctx->shape_type, "CIRCLE") == 0) {
        CircleImpl* c = (CircleImpl*)preview->impl;
        c->center[0] = x; c->center[1] = y;
        c->radius = 0.01f;
    } else if (strcmp(ctx->shape_type, "RECT") == 0) {
        RectImpl* r = (RectImpl*)preview->impl;
        r->min[0] = x; r->min[1] = y;
        r->max[0] = x; r->max[1] = y;
    }

    ctx->preview = preview;
    sm_add(preview);
}

static void draw_tool_on_move(Tool* t, float x, float y, SelectionManager* sel)
{
    (void)sel;
    DrawToolCtx* ctx = (DrawToolCtx*)t->ctx;
    ctx->p2[0] = x; ctx->p2[1] = y;

    if (!ctx->preview) return;

    /* Remove old preview from ShapeManager */
    if (sm_count() > 0) {
        sm_remove_last();
    }

    /* Create new preview */
    Shape* preview = shape_create(ctx->shape_type, 1.0f, 1.0f, 1.0f, 0.7f, 2.0f);
    if (!preview) return;

    if (strcmp(ctx->shape_type, "LINE") == 0) {
        LineImpl* line = (LineImpl*)preview->impl;
        line->p1[0] = ctx->p1[0]; line->p1[1] = ctx->p1[1];
        line->p2[0] = x; line->p2[1] = y;
    } else if (strcmp(ctx->shape_type, "CIRCLE") == 0) {
        CircleImpl* c = (CircleImpl*)preview->impl;
        c->center[0] = ctx->p1[0]; c->center[1] = ctx->p1[1];
        float dx = x - ctx->p1[0];
        float dy = y - ctx->p1[1];
        c->radius = (dx*dx + dy*dy > 0.0f) ? sqrtf(dx*dx + dy*dy) : 0.01f;
    } else if (strcmp(ctx->shape_type, "RECT") == 0) {
        RectImpl* r = (RectImpl*)preview->impl;
        r->min[0] = ctx->p1[0]; r->min[1] = ctx->p1[1];
        r->max[0] = x; r->max[1] = y;
    }

    ctx->preview = preview;
    sm_add(preview);
}

static void draw_tool_on_up(Tool* t, SelectionManager* sel)
{
    (void)sel;
    DrawToolCtx* ctx = (DrawToolCtx*)t->ctx;

    if (!ctx->preview || sm_count() == 0) {
        ctx->preview = NULL;
        return;
    }

    /* Remove preview from ShapeManager */
    sm_remove_last();
    ctx->preview = NULL;

    /* Create final shape */
    Shape* final = shape_create(ctx->shape_type, 0.0f, 0.8f, 1.0f, 1.0f, 2.0f);
    if (!final) return;

    if (strcmp(ctx->shape_type, "LINE") == 0) {
        LineImpl* line = (LineImpl*)final->impl;
        line->p1[0] = ctx->p1[0]; line->p1[1] = ctx->p1[1];
        line->p2[0] = ctx->p2[0]; line->p2[1] = ctx->p2[1];
    } else if (strcmp(ctx->shape_type, "CIRCLE") == 0) {
        CircleImpl* c = (CircleImpl*)final->impl;
        c->center[0] = ctx->p1[0]; c->center[1] = ctx->p1[1];
        float dx = ctx->p2[0] - ctx->p1[0];
        float dy = ctx->p2[1] - ctx->p1[1];
        c->radius = (dx*dx + dy*dy > 0.0f) ? sqrtf(dx*dx + dy*dy) : 0.01f;
    } else if (strcmp(ctx->shape_type, "RECT") == 0) {
        RectImpl* r = (RectImpl*)final->impl;
        r->min[0] = ctx->p1[0]; r->min[1] = ctx->p1[1];
        r->max[0] = ctx->p2[0]; r->max[1] = ctx->p2[1];
    }

    sm_add(final);
}

static ToolVTable draw_tool_vtable = {
    .name = draw_tool_name,
    .on_down = draw_tool_on_down,
    .on_move = draw_tool_on_move,
    .on_up = draw_tool_on_up,
};

Tool* draw_tool_create(const char* shape_type)
{
    Tool* t = (Tool*)malloc(sizeof(Tool));
    if (!t) return NULL;

    DrawToolCtx* ctx = (DrawToolCtx*)malloc(sizeof(DrawToolCtx));
    if (!ctx) { free(t); return NULL; }

    ctx->p1[0] = ctx->p1[1] = 0.0f;
    ctx->p2[0] = ctx->p2[1] = 0.0f;
    ctx->preview = NULL;
    strncpy(ctx->shape_type, shape_type, sizeof(ctx->shape_type) - 1);
    ctx->shape_type[sizeof(ctx->shape_type) - 1] = '\0';

    t->vtable = &draw_tool_vtable;
    t->ctx = ctx;
    return t;
}

void draw_tool_destroy(Tool* t)
{
    if (!t) return;
    free(t->ctx);
    free(t);
}

const char* draw_tool_get_type(const Tool* t)
{
    DrawToolCtx* ctx = (DrawToolCtx*)t->ctx;
    return ctx->shape_type;
}

void draw_tool_set_type(Tool* t, const char* shape_type)
{
    DrawToolCtx* ctx = (DrawToolCtx*)t->ctx;
    strncpy(ctx->shape_type, shape_type, sizeof(ctx->shape_type) - 1);
    ctx->shape_type[sizeof(ctx->shape_type) - 1] = '\0';
}
