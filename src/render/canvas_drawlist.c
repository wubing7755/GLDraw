#include <render/canvas_drawlist.h>

#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int canvas_drawlist_reserve_points(CanvasDrawList* draw_list, size_t needed)
{
    Vec2* points = NULL;
    size_t capacity = 0u;

    if (!draw_list) {
        return 0;
    }
    if (needed <= draw_list->point_capacity) {
        return 1;
    }

    capacity = draw_list->point_capacity > 0u ? draw_list->point_capacity : 64u;
    while (capacity < needed) {
        capacity *= 2u;
    }

    points = (Vec2*)realloc(draw_list->points, capacity * sizeof(draw_list->points[0]));
    if (!points) {
        return 0;
    }

    draw_list->points = points;
    draw_list->point_capacity = capacity;
    return 1;
}

static int canvas_drawlist_reserve_strokes(CanvasDrawList* draw_list, size_t needed)
{
    CanvasStrokeCommand* strokes = NULL;
    size_t capacity = 0u;

    if (!draw_list) {
        return 0;
    }
    if (needed <= draw_list->stroke_capacity) {
        return 1;
    }

    capacity = draw_list->stroke_capacity > 0u ? draw_list->stroke_capacity : 16u;
    while (capacity < needed) {
        capacity *= 2u;
    }

    strokes = (CanvasStrokeCommand*)realloc(draw_list->strokes,
                                            capacity * sizeof(draw_list->strokes[0]));
    if (!strokes) {
        return 0;
    }

    draw_list->strokes = strokes;
    draw_list->stroke_capacity = capacity;
    return 1;
}

static int canvas_drawlist_append_stroke(CanvasDrawList* draw_list,
                                         const Vec2* points,
                                         int point_count,
                                         Color color,
                                         float line_width,
                                         RenderPrimitiveType primitive)
{
    CanvasStrokeCommand* stroke = NULL;

    if (!draw_list || !points || point_count <= 1) {
        return 0;
    }
    if (!canvas_drawlist_reserve_points(draw_list,
                                        draw_list->point_count + (size_t)point_count) ||
        !canvas_drawlist_reserve_strokes(draw_list, draw_list->stroke_count + 1u)) {
        return 0;
    }

    memcpy(draw_list->points + draw_list->point_count,
           points,
           (size_t)point_count * sizeof(points[0]));

    stroke = &draw_list->strokes[draw_list->stroke_count++];
    stroke->color = color;
    stroke->line_width = line_width;
    stroke->primitive = primitive;
    stroke->point_offset = draw_list->point_count;
    stroke->point_count = point_count;
    draw_list->point_count += (size_t)point_count;
    return 1;
}

static int count_grid_lines(float start, float end, float spacing)
{
    int count = 0;
    float value = start;
    float limit = end + spacing * 0.5f;

    if (spacing <= 0.0f) {
        return 0;
    }

    while (value <= limit) {
        ++count;
        value += spacing;
    }

    return count;
}

static int canvas_drawlist_append_grid(CanvasDrawList* draw_list, const CanvasView* canvas)
{
    RectF visible = canvas_view_visible_world_rect(canvas);
    Color grid_color = {0.20f, 0.22f, 0.25f, 1.0f};
    Color axis_color = {0.32f, 0.35f, 0.40f, 1.0f};
    float spacing = 100.0f;
    float start_x = floorf(visible.x / spacing) * spacing;
    float end_x = rectf_right(&visible);
    float start_y = floorf(visible.y / spacing) * spacing;
    float end_y = rectf_top(&visible);
    int vertical_count = count_grid_lines(start_x, end_x, spacing);
    int horizontal_count = count_grid_lines(start_y, end_y, spacing);
    int point_count = (vertical_count + horizontal_count) * 2;
    Vec2 axis_points[4];
    Vec2* points = NULL;
    int index = 0;

    if (point_count > 0) {
        points = (Vec2*)render_arena_alloc(&draw_list->scratch_arena,
                                           (size_t)point_count * sizeof(points[0]),
                                           sizeof(points[0]));
        if (!points) {
            return 0;
        }

        for (float x = start_x; x <= end_x + spacing * 0.5f; x += spacing) {
            points[index++] = canvas_view_world_to_screen(canvas, vec2_make(x, visible.y));
            points[index++] = canvas_view_world_to_screen(canvas,
                                                          vec2_make(x, visible.y + visible.h));
        }
        for (float y = start_y; y <= end_y + spacing * 0.5f; y += spacing) {
            points[index++] = canvas_view_world_to_screen(canvas, vec2_make(visible.x, y));
            points[index++] = canvas_view_world_to_screen(canvas,
                                                          vec2_make(visible.x + visible.w, y));
        }

        if (!canvas_drawlist_append_stroke(draw_list,
                                           points,
                                           point_count,
                                           grid_color,
                                           1.0f,
                                           RENDER_PRIMITIVE_LINES)) {
            return 0;
        }
    }

    axis_points[0] = canvas_view_world_to_screen(canvas, vec2_make(visible.x, 0.0f));
    axis_points[1] =
        canvas_view_world_to_screen(canvas, vec2_make(visible.x + visible.w, 0.0f));
    axis_points[2] = canvas_view_world_to_screen(canvas, vec2_make(0.0f, visible.y));
    axis_points[3] =
        canvas_view_world_to_screen(canvas, vec2_make(0.0f, visible.y + visible.h));

    return canvas_drawlist_append_stroke(draw_list,
                                         axis_points,
                                         4,
                                         axis_color,
                                         1.5f,
                                         RENDER_PRIMITIVE_LINES);
}

static int canvas_drawlist_append_object(CanvasDrawList* draw_list,
                                         const CanvasView* canvas,
                                         const GraphicObject* object,
                                         int selected,
                                         Vec2 preview_delta)
{
    int point_count = 0;
    Vec2* points = NULL;
    int i = 0;
    Color highlight = {0.98f, 0.86f, 0.24f, 1.0f};

    if (!draw_list || !canvas || !object) {
        return 0;
    }

    point_count = object_get_path_point_count(object);
    if (point_count <= 1) {
        return 1;
    }

    points = (Vec2*)render_arena_alloc(&draw_list->scratch_arena,
                                       (size_t)point_count * sizeof(points[0]),
                                       sizeof(points[0]));
    if (!points) {
        return 0;
    }

    object_write_path_points(object, points);
    for (i = 0; i < point_count; ++i) {
        points[i] = vec2_add(points[i], preview_delta);
        points[i] = canvas_view_world_to_screen(canvas, points[i]);
    }

    if (selected &&
        !canvas_drawlist_append_stroke(draw_list,
                                       points,
                                       point_count,
                                       highlight,
                                       object->style.stroke_width + 3.0f,
                                       RENDER_PRIMITIVE_LINE_STRIP)) {
        return 0;
    }

    if (!canvas_drawlist_append_stroke(draw_list,
                                       points,
                                       point_count,
                                       object->style.stroke_color,
                                       object->style.stroke_width,
                                       RENDER_PRIMITIVE_LINE_STRIP)) {
        return 0;
    }

    return 1;
}

void canvas_drawlist_init(CanvasDrawList* draw_list)
{
    if (!draw_list) {
        return;
    }

    memset(draw_list, 0, sizeof(*draw_list));
    render_arena_init(&draw_list->scratch_arena);
}

void canvas_drawlist_reset(CanvasDrawList* draw_list)
{
    if (!draw_list) {
        return;
    }

    draw_list->point_count = 0u;
    draw_list->stroke_count = 0u;
    render_arena_reset(&draw_list->scratch_arena);
    draw_list->clip_rect = (RectF){0.0f, 0.0f, 0.0f, 0.0f};
    draw_list->clear_color = (Color){0.0f, 0.0f, 0.0f, 1.0f};
}

void canvas_drawlist_shutdown(CanvasDrawList* draw_list)
{
    if (!draw_list) {
        return;
    }

    free(draw_list->points);
    free(draw_list->strokes);
    render_arena_shutdown(&draw_list->scratch_arena);
    canvas_drawlist_init(draw_list);
}

int canvas_drawlist_build(CanvasDrawList* draw_list,
                          const Document* document,
                          const SelectionSet* selection,
                          const CanvasView* canvas,
                          int selection_preview_active,
                          Vec2 selection_preview_delta,
                          const GraphicObject* overlay_object)
{
    RectF visible_rect;
    int* visible_indices = NULL;
    int visible_count = 0;
    int layer_index = 0;
    int success = 0;

    if (!draw_list || !document || !canvas) {
        return 0;
    }

    canvas_drawlist_reset(draw_list);
    draw_list->clip_rect = canvas_view_viewport(canvas);
    draw_list->clear_color = canvas->background;

    if (canvas->show_grid && !canvas_drawlist_append_grid(draw_list, canvas)) {
        goto cleanup;
    }

    if (document->count > 0) {
        visible_indices = (int*)malloc((size_t)document->count * sizeof(visible_indices[0]));
        if (!visible_indices) {
            goto cleanup;
        }
    }

    visible_rect = canvas_view_visible_world_rect(canvas);
    visible_count = document_query_visible_indices(document,
                                                   visible_rect,
                                                   visible_indices,
                                                   document->count);

    if (visible_count <= 0 && document->count > 0) {
        int i = 0;
        visible_count = document->count;
        for (i = 0; i < visible_count; ++i) {
            visible_indices[i] = i;
        }
    }

    for (layer_index = 0; layer_index < document_layer_count(document); ++layer_index) {
        const DocumentLayer* layer = document_layer_at(document, layer_index);
        int i = 0;

        if (!layer || !layer->visible) {
            continue;
        }

        for (i = 0; i < visible_count; ++i) {
            const GraphicObject* object = document->objects[visible_indices[i]];
            int selected = object && selection && selection_set_contains(selection, object->id);
            Vec2 preview_delta = (selection_preview_active && selected)
                                     ? selection_preview_delta
                                     : vec2_make(0.0f, 0.0f);

            if (!object || object->layer_id != layer->id) {
                continue;
            }
            if (!canvas_drawlist_append_object(draw_list,
                                               canvas,
                                               object,
                                               selected,
                                               preview_delta)) {
                goto cleanup;
            }
        }
    }

    if (overlay_object &&
        !canvas_drawlist_append_object(draw_list,
                                       canvas,
                                       overlay_object,
                                       0,
                                       vec2_make(0.0f, 0.0f))) {
        goto cleanup;
    }

    success = 1;

cleanup:
    free(visible_indices);
    return success;
}
