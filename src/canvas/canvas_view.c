/**
 * @file canvas_view.c
 * @brief Canvas camera state and coordinate conversion implementation.
 *
 * Role in project:
 * - Implements viewport/center/zoom updates and world/screen transforms.
 * - Provides object picking against document geometry.
 *
 * Module relationships:
 * - Uses `document` for object iteration and hit-testing.
 * - Consumed by tools, renderer, and application input routing.
 */
#include <canvas/canvas_view.h>

#include <base/math2d.h>

#include <stddef.h>

/**
 * @brief canvas_view_state 函数。
 *
 * @param canvas 参数 `canvas`。
 * @return 函数返回值。
 */
static CanvasViewportState canvas_view_state(const CanvasView* canvas)
{
    CanvasViewportState state;
    state.viewport.x = 0.0f;
    state.viewport.y = 0.0f;
    state.viewport.w = 0.0f;
    state.viewport.h = 0.0f;
    state.center = vec2_make(0.0f, 0.0f);
    state.zoom = 1.0f;

    if (!canvas) {
        return state;
    }

    state = canvas->viewport_state;
    if (state.zoom <= 0.0f) {
        state.zoom = 1.0f;
    }
    return state;
}

/**
 * @brief canvas_viewport_world_to_screen 函数。
 *
 * @param state 参数 `state`。
 * @param world 参数 `world`。
 * @return 函数返回值。
 */
static Vec2 canvas_viewport_world_to_screen(const CanvasViewportState* state, Vec2 world)
{
    Vec2 screen = {0.0f, 0.0f};

    if (!state) {
        return screen;
    }

    screen.x = state->viewport.x + (state->viewport.w * 0.5f) + (world.x - state->center.x) * state->zoom;
    screen.y = state->viewport.y + (state->viewport.h * 0.5f) - (world.y - state->center.y) * state->zoom;
    return screen;
}

/**
 * @brief canvas_viewport_screen_to_world 函数。
 *
 * @param state 参数 `state`。
 * @param screen 参数 `screen`。
 * @return 函数返回值。
 */
static Vec2 canvas_viewport_screen_to_world(const CanvasViewportState* state, Vec2 screen)
{
    Vec2 world = {0.0f, 0.0f};

    if (!state || state->zoom <= 0.0f) {
        return world;
    }

    world.x = state->center.x + (screen.x - (state->viewport.x + state->viewport.w * 0.5f)) / state->zoom;
    world.y = state->center.y - (screen.y - (state->viewport.y + state->viewport.h * 0.5f)) / state->zoom;
    return world;
}

/**
 * @brief canvas_viewport_pan_screen_delta 函数。
 *
 * @param state 参数 `state`。
 * @param delta_screen 参数 `delta_screen`。
 * @return 无。
 */
static void canvas_viewport_pan_screen_delta(CanvasViewportState* state, Vec2 delta_screen)
{
    if (!state || state->zoom <= 0.0f) {
        return;
    }

    state->center.x -= delta_screen.x / state->zoom;
    state->center.y += delta_screen.y / state->zoom;
}

/**
 * @brief Zoom around a screen anchor while keeping anchor's world point fixed.
 * @param state [in,out] View state.
 * @param factor [in] Multiplicative zoom factor.
 * @param screen_anchor [in] Anchor in screen coordinates.
 * @return None.
 *
 * Why:
 * - Recomputes world point before/after zoom and shifts center by the delta.
 *   This gives "zoom toward cursor" behavior instead of zooming to viewport center.
 *
 * Complexity: `O(1)`.
 */
static void canvas_viewport_zoom_at_screen_point(CanvasViewportState* state, float factor, Vec2 screen_anchor)
{
    Vec2 before = {0.0f, 0.0f};
    Vec2 after = {0.0f, 0.0f};

    if (!state || factor <= 0.0f) {
        return;
    }

    before = canvas_viewport_screen_to_world(state, screen_anchor);
    state->zoom = clampf(state->zoom * factor, 0.1f, 12.0f);
    after = canvas_viewport_screen_to_world(state, screen_anchor);
    state->center = vec2_add(state->center, vec2_sub(before, after));
}

/**
 * @brief canvas_viewport_visible_world_rect 函数。
 *
 * @param state 参数 `state`。
 * @return 函数返回值。
 */
static RectF canvas_viewport_visible_world_rect(const CanvasViewportState* state)
{
    RectF rect = {0.0f, 0.0f, 0.0f, 0.0f};

    if (!state || state->zoom <= 0.0f) {
        return rect;
    }

    rect.w = state->viewport.w / state->zoom;
    rect.h = state->viewport.h / state->zoom;
    rect.x = state->center.x - rect.w * 0.5f;
    rect.y = state->center.y - rect.h * 0.5f;
    return rect;
}

/**
 * @brief canvas_view_init 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param document 参数 `document`。
 * @param viewport 参数 `viewport`。
 * @return 无。
 */
void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport)
{
    if (!canvas) {
        return;
    }

    canvas->document = document;
    canvas->viewport_state.viewport = viewport;
    canvas->viewport_state.center = vec2_make(0.0f, 0.0f);
    canvas->viewport_state.zoom = 1.0f;
    canvas->show_grid = 1;
    canvas->background.r = 218.0f / 255.0f;
    canvas->background.g = 226.0f / 255.0f;
    canvas->background.b = 236.0f / 255.0f;
    canvas->background.a = 1.0f;
}

/**
 * @brief canvas_view_set_document 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param document 参数 `document`。
 * @return 无。
 */
void canvas_view_set_document(CanvasView* canvas, Document* document)
{
    if (canvas) {
        canvas->document = document;
    }
}

/**
 * @brief canvas_view_set_viewport 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param viewport 参数 `viewport`。
 * @return 无。
 */
void canvas_view_set_viewport(CanvasView* canvas, RectF viewport)
{
    if (!canvas) {
        return;
    }

    canvas->viewport_state.viewport = viewport;
}

/**
 * @brief canvas_view_set_center 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param center 参数 `center`。
 * @return 无。
 */
void canvas_view_set_center(CanvasView* canvas, Vec2 center)
{
    if (!canvas) {
        return;
    }

    canvas->viewport_state.center = center;
}

/**
 * @brief canvas_view_set_zoom 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param zoom 参数 `zoom`。
 * @return 无。
 */
void canvas_view_set_zoom(CanvasView* canvas, float zoom)
{
    if (!canvas) {
        return;
    }

    canvas->viewport_state.zoom = clampf(zoom, 0.1f, 12.0f);
}

/**
 * @brief canvas_view_set_center_zoom 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param center 参数 `center`。
 * @param zoom 参数 `zoom`。
 * @return 无。
 */
void canvas_view_set_center_zoom(CanvasView* canvas, Vec2 center, float zoom)
{
    if (!canvas) {
        return;
    }

    canvas->viewport_state.center = center;
    canvas->viewport_state.zoom = clampf(zoom, 0.1f, 12.0f);
}

/**
 * @brief canvas_view_viewport 函数。
 *
 * @param canvas 参数 `canvas`。
 * @return 函数返回值。
 */
RectF canvas_view_viewport(const CanvasView* canvas)
{
    return canvas_view_state(canvas).viewport;
}

/**
 * @brief canvas_view_center 函数。
 *
 * @param canvas 参数 `canvas`。
 * @return 函数返回值。
 */
Vec2 canvas_view_center(const CanvasView* canvas)
{
    return canvas_view_state(canvas).center;
}

/**
 * @brief canvas_view_zoom 函数。
 *
 * @param canvas 参数 `canvas`。
 * @return 函数返回值。
 */
float canvas_view_zoom(const CanvasView* canvas)
{
    return canvas_view_state(canvas).zoom;
}

/**
 * @brief canvas_view_world_to_screen 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param world 参数 `world`。
 * @return 函数返回值。
 */
Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world)
{
    CanvasViewportState state = canvas_view_state(canvas);
    return canvas_viewport_world_to_screen(&state, world);
}

/**
 * @brief canvas_view_screen_to_world 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param screen 参数 `screen`。
 * @return 函数返回值。
 */
Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen)
{
    CanvasViewportState state = canvas_view_state(canvas);
    return canvas_viewport_screen_to_world(&state, screen);
}

/**
 * @brief canvas_view_pan_screen_delta 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param delta_screen 参数 `delta_screen`。
 * @return 无。
 */
void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen)
{
    CanvasViewportState state;

    if (!canvas) {
        return;
    }

    state = canvas_view_state(canvas);
    canvas_viewport_pan_screen_delta(&state, delta_screen);
    canvas->viewport_state = state;
}

/**
 * @brief canvas_view_zoom_at_screen_point 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param factor 参数 `factor`。
 * @param screen_anchor 参数 `screen_anchor`。
 * @return 无。
 */
void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor)
{
    CanvasViewportState state;

    if (!canvas) {
        return;
    }

    state = canvas_view_state(canvas);
    canvas_viewport_zoom_at_screen_point(&state, factor, screen_anchor);
    canvas->viewport_state = state;
}

/**
 * @brief canvas_view_world_tolerance_for_pixels 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param pixels 参数 `pixels`。
 * @return 函数返回值。
 */
float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels)
{
    float zoom = canvas_view_zoom(canvas);

    if (zoom <= 0.0f) {
        return pixels;
    }

    return pixels / zoom;
}

/**
 * @brief canvas_view_visible_world_rect 函数。
 *
 * @param canvas 参数 `canvas`。
 * @return 函数返回值。
 */
RectF canvas_view_visible_world_rect(const CanvasView* canvas)
{
    CanvasViewportState state = canvas_view_state(canvas);
    return canvas_viewport_visible_world_rect(&state);
}

/**
 * @brief Pick top-most object under a screen point.
 * @return Pointer to hit object or `NULL`.
 *
 * Why reverse loop:
 * - Newer objects are drawn later and visually on top, so picking scans from end.
 *
 * Time complexity: `O(n)` where `n` is document object count.
 */

/**
 * @brief canvas_view_pick_object 函数。
 *
 * @param canvas 参数 `canvas`。
 * @param screen_point 参数 `screen_point`。
 * @param tolerance_pixels 参数 `tolerance_pixels`。
 * @return 函数返回值。
 */
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
