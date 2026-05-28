/**
 * @file workspace_view_commands.c
 * @brief Canvas view commands for zooming and grid visibility.
 */
#include <app/workspace_view_commands.h>

#include <app/workspace_internal.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>

int workspace_view_zoom_in(Workspace* workspace)
{
    Vec2 center;

    if (!workspace) {
        return 0;
    }

    center = canvas_view_center(&workspace->core.canvas);
    canvas_view_zoom_at_screen_point(&workspace->core.canvas,
                                     1.25f,
                                     canvas_view_world_to_screen(&workspace->core.canvas, center));
    return 1;
}

int workspace_view_zoom_out(Workspace* workspace)
{
    Vec2 center;

    if (!workspace) {
        return 0;
    }

    center = canvas_view_center(&workspace->core.canvas);
    canvas_view_zoom_at_screen_point(&workspace->core.canvas,
                                     0.8f,
                                     canvas_view_world_to_screen(&workspace->core.canvas, center));
    return 1;
}

int workspace_view_zoom_to_fit(Workspace* workspace)
{
    const float padding_ratio = 0.10f;
    const float minimum_world_span = 32.0f;
    Document* doc = NULL;
    CanvasView* canvas = NULL;
    RectF canvas_viewport = {0.0f, 0.0f, 0.0f, 0.0f};
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    float content_w = 0.0f;
    float content_h = 0.0f;
    float pad_x = 0.0f;
    float pad_y = 0.0f;
    float zoom_x = 1.0f;
    float zoom_y = 1.0f;
    float new_zoom = 1.0f;
    int first = 1;
    int i = 0;

    if (!workspace || !workspace->core.canvas.document) {
        return 0;
    }

    doc = &workspace->core.document;
    canvas = &workspace->core.canvas;
    canvas_viewport = canvas_view_viewport(canvas);
    if (canvas_viewport.w <= 1.0f || canvas_viewport.h <= 1.0f) {
        canvas_view_set_center_zoom(canvas, vec2_make(0.0f, 0.0f), 1.0f);
        return 1;
    }

    if (doc->count == 0) {
        canvas_view_set_center_zoom(canvas, vec2_make(0.0f, 0.0f), 1.0f);
        return 1;
    }

    for (i = 0; i < doc->count; ++i) {
        GraphicObject* object = doc->objects[i];
        RectF bounds;

        if (!object) {
            continue;
        }

        bounds = object_get_bounds(object);
        if (first) {
            min_x = bounds.x;
            max_x = bounds.x + bounds.w;
            min_y = bounds.y;
            max_y = bounds.y + bounds.h;
            first = 0;
        } else {
            if (bounds.x < min_x) min_x = bounds.x;
            if (bounds.y < min_y) min_y = bounds.y;
            if (bounds.x + bounds.w > max_x) max_x = bounds.x + bounds.w;
            if (bounds.y + bounds.h > max_y) max_y = bounds.y + bounds.h;
        }
    }

    if (first) {
        canvas_view_set_center_zoom(canvas, vec2_make(0.0f, 0.0f), 1.0f);
        return 1;
    }

    content_w = max_x - min_x;
    content_h = max_y - min_y;
    if (content_w < minimum_world_span) {
        float center_x = (min_x + max_x) * 0.5f;
        content_w = minimum_world_span;
        min_x = center_x - content_w * 0.5f;
        max_x = center_x + content_w * 0.5f;
    }
    if (content_h < minimum_world_span) {
        float center_y = (min_y + max_y) * 0.5f;
        content_h = minimum_world_span;
        min_y = center_y - content_h * 0.5f;
        max_y = center_y + content_h * 0.5f;
    }

    pad_x = content_w * padding_ratio;
    pad_y = content_h * padding_ratio;
    min_x -= pad_x;
    min_y -= pad_y;
    max_x += pad_x;
    max_y += pad_y;
    content_w = max_x - min_x;
    content_h = max_y - min_y;
    zoom_x = canvas_viewport.w / content_w;
    zoom_y = canvas_viewport.h / content_h;
    new_zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    new_zoom = clampf(new_zoom, 0.1f, 12.0f);
    canvas_view_set_center_zoom(canvas,
                                vec2_make((min_x + max_x) * 0.5f,
                                          (min_y + max_y) * 0.5f),
                                new_zoom);

    return 1;
}

int workspace_view_toggle_grid(Workspace* workspace)
{
    if (!workspace) {
        return 0;
    }

    workspace->core.canvas.show_grid = !workspace->core.canvas.show_grid;
    return 1;
}

int workspace_view_toggle_inspector(Workspace* workspace)
{
    return workspace != NULL;
}
