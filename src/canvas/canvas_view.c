#include <canvas/canvas_view.h>

#include <base/math2d.h>

#include <stddef.h>

void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport)
{
    if (!canvas) {
        return;
    }

    canvas->document = document;
    canvas->viewport = viewport;
    canvas->center = vec2_make(0.0f, 0.0f);
    canvas->zoom = 1.0f;
    canvas->show_grid = 1;
    canvas->background.r = 0.11f;
    canvas->background.g = 0.12f;
    canvas->background.b = 0.14f;
    canvas->background.a = 1.0f;
}

void canvas_view_set_document(CanvasView* canvas, Document* document)
{
    if (canvas) {
        canvas->document = document;
    }
}

void canvas_view_set_viewport(CanvasView* canvas, RectF viewport)
{
    if (canvas) {
        canvas->viewport = viewport;
    }
}

Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world)
{
    Vec2 screen = {0.0f, 0.0f};
    if (!canvas) {
        return screen;
    }

    screen.x = canvas->viewport.x + (canvas->viewport.w * 0.5f) + (world.x - canvas->center.x) * canvas->zoom;
    screen.y = canvas->viewport.y + (canvas->viewport.h * 0.5f) - (world.y - canvas->center.y) * canvas->zoom;
    return screen;
}

Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen)
{
    Vec2 world = {0.0f, 0.0f};
    if (!canvas || canvas->zoom <= 0.0f) {
        return world;
    }

    world.x = canvas->center.x + (screen.x - (canvas->viewport.x + canvas->viewport.w * 0.5f)) / canvas->zoom;
    world.y = canvas->center.y - (screen.y - (canvas->viewport.y + canvas->viewport.h * 0.5f)) / canvas->zoom;
    return world;
}

void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen)
{
    if (!canvas || canvas->zoom <= 0.0f) {
        return;
    }

    canvas->center.x -= delta_screen.x / canvas->zoom;
    canvas->center.y += delta_screen.y / canvas->zoom;
}

void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor)
{
    Vec2 before = {0.0f, 0.0f};
    Vec2 after = {0.0f, 0.0f};

    if (!canvas || factor <= 0.0f) {
        return;
    }

    before = canvas_view_screen_to_world(canvas, screen_anchor);
    canvas->zoom = clampf(canvas->zoom * factor, 0.1f, 12.0f);
    after = canvas_view_screen_to_world(canvas, screen_anchor);
    canvas->center = vec2_add(canvas->center, vec2_sub(before, after));
}

float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels)
{
    if (!canvas || canvas->zoom <= 0.0f) {
        return pixels;
    }
    return pixels / canvas->zoom;
}

RectF canvas_view_visible_world_rect(const CanvasView* canvas)
{
    RectF rect = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!canvas || canvas->zoom <= 0.0f) {
        return rect;
    }

    rect.w = canvas->viewport.w / canvas->zoom;
    rect.h = canvas->viewport.h / canvas->zoom;
    rect.x = canvas->center.x - rect.w * 0.5f;
    rect.y = canvas->center.y - rect.h * 0.5f;
    return rect;
}

GraphicObject* canvas_view_pick_object(const CanvasView* canvas, Vec2 screen_point, float tolerance_pixels)
{
    Vec2 world_point = {0.0f, 0.0f};
    float tolerance_world = 0.0f;
    int i = 0;

    if (!canvas || !canvas->document) {
        return NULL;
    }

    world_point = canvas_view_screen_to_world(canvas, screen_point);
    tolerance_world = canvas_view_world_tolerance_for_pixels(canvas, tolerance_pixels);

    for (i = canvas->document->count - 1; i >= 0; --i) {
        GraphicObject* object = canvas->document->objects[i];
        if (object && object_hit_test(object, world_point, tolerance_world)) {
            return object;
        }
    }

    return NULL;
}
