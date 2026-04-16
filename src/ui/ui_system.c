#include <ui/ui_system.h>
#include <ui/ui_menubar.h>

#include <app/workspace.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <tools/tool_controller.h>
#include <glad/glad.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"
#include <ui/ui_theme.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define UI_MAX_VERTEX_BUFFER (512 * 1024)
#define UI_MAX_ELEMENT_BUFFER (128 * 1024)
#define UI_MIN_CANVAS_WIDTH 320.0f

struct UiSystem {
    struct nk_glfw glfw;
    struct nk_context* ctx;
    UiMenuBar* menu_bar;
    UiThemeTokens theme;
    RectF appbar_bounds;
    RectF rail_bounds;
    RectF panel_bounds;
    RectF status_bounds;
    float inspector_anim_t;
    int inspector_target_visible;
    int inspector_anim_initialized;
    double last_frame_seconds;
};

static float ui_clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float ui_smoothstep(float t)
{
    float clamped = ui_clampf(t, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

static ToolContext ui_tool_context(Workspace* workspace)
{
    ToolContext context;
    context.workspace = workspace;
    context.document = &workspace->document;
    context.history = &workspace->history;
    context.canvas = &workspace->canvas;
    return context;
}

static void ui_commit_document_change(Workspace* workspace, DocumentSnapshot* before_snapshot)
{
    document_touch(&workspace->document);
    document_history_push(&workspace->history, before_snapshot, &workspace->document);
    workspace_sync_document_dirty(workspace);
}

static void ui_apply_stroke_color(Workspace* workspace, GraphicObject* object, Color color)
{
    DocumentSnapshot before_snapshot;
    document_snapshot_init(&before_snapshot);
    if (!document_snapshot_capture(&before_snapshot, &workspace->document)) {
        return;
    }
    object_set_stroke_color(object, color);
    ui_commit_document_change(workspace, &before_snapshot);
}

static void ui_apply_stroke_width(Workspace* workspace, GraphicObject* object, float stroke_width)
{
    DocumentSnapshot before_snapshot;
    document_snapshot_init(&before_snapshot);
    if (!document_snapshot_capture(&before_snapshot, &workspace->document)) {
        return;
    }
    object_set_stroke_width(object, stroke_width);
    ui_commit_document_change(workspace, &before_snapshot);
}

static void ui_property_apply_float(struct nk_context* ctx,
                                    Workspace* workspace,
                                    GraphicObject* object,
                                    const char* label,
                                    const char* key,
                                    float min_value,
                                    float* value,
                                    float max_value,
                                    float step,
                                    float inc_per_pixel)
{
    float before = *value;
    DocumentSnapshot before_snapshot;

    nk_property_float(ctx, label, min_value, value, max_value, step, inc_per_pixel);
    if (fabsf(*value - before) > 1e-6f) {
        document_snapshot_init(&before_snapshot);
        if (!document_snapshot_capture(&before_snapshot, &workspace->document)) {
            return;
        }
        object_set_scalar(object, key, *value);
        ui_commit_document_change(workspace, &before_snapshot);
    }
}

static int ui_tool_button(UiSystem* ui, const char* label, int active, const char* tooltip)
{
    struct nk_context* ctx = ui->ctx;
    struct nk_rect widget_bounds;
    int pressed = 0;
    int hovered = 0;

    if (!ctx || !label) {
        return 0;
    }

    if (active) {
        struct nk_style_item active_bg = nk_style_item_color(ui->theme.primary);
        nk_style_push_style_item(ctx, &ctx->style.button.normal, active_bg);
        nk_style_push_style_item(ctx, &ctx->style.button.hover, active_bg);
        nk_style_push_style_item(ctx, &ctx->style.button.active, active_bg);
        nk_style_push_color(ctx, &ctx->style.button.text_normal, nk_rgba(255, 255, 255, 255));
        nk_style_push_color(ctx, &ctx->style.button.text_hover, nk_rgba(255, 255, 255, 255));
        nk_style_push_color(ctx, &ctx->style.button.text_active, nk_rgba(255, 255, 255, 255));
    }

    widget_bounds = nk_widget_bounds(ctx);
    pressed = nk_button_label(ctx, label);
    hovered = nk_input_is_mouse_hovering_rect(&ctx->input, widget_bounds);
    if (tooltip && tooltip[0] != '\0' && hovered) {
        nk_tooltip(ctx, tooltip);
    }

    if (active) {
        nk_style_pop_color(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
        nk_style_pop_style_item(ctx);
    }

    return pressed;
}

static void ui_tool_rail(UiSystem* ui, Workspace* workspace, RectF bounds)
{
    struct nk_context* ctx = ui->ctx;
    ToolContext context = ui_tool_context(workspace);
    ToolKind active = workspace->tools.active_kind;

    ui->rail_bounds = bounds;
    if (bounds.w <= 0.0f || bounds.h <= 0.0f) {
        return;
    }

    if (nk_begin(ctx, "Tool Rail",
                 nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, ui->theme.row_height, 1);

        if (ui_tool_button(ui, "Select (V)", active == TOOL_KIND_SELECT, "Select and edit objects")) {
            tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_SELECT);
        }
        if (ui_tool_button(ui, "Hand (H)", active == TOOL_KIND_PAN, "Pan canvas view")) {
            tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_PAN);
        }
        if (ui_tool_button(ui, "Line (L)", active == TOOL_KIND_LINE, "Draw line")) {
            tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_LINE);
        }
        if (ui_tool_button(ui, "Rect (R)", active == TOOL_KIND_RECT, "Draw rectangle")) {
            tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_RECT);
        }
        if (ui_tool_button(ui, "Ellipse (E)", active == TOOL_KIND_ELLIPSE, "Draw ellipse")) {
            tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_ELLIPSE);
        }

        nk_layout_row_dynamic(ctx, ui->theme.row_height * 0.9f, 1);
        nk_label(ctx, "", NK_TEXT_LEFT);
        nk_labelf(ctx, NK_TEXT_LEFT, "Active:");
        nk_label(ctx, tool_controller_active_label(&workspace->tools), NK_TEXT_LEFT);
    }
    nk_end(ctx);
}

static void ui_inspector_empty_hint(struct nk_context* ctx)
{
    nk_label(ctx, "No selection", NK_TEXT_LEFT);
    nk_label(ctx, "Shortcuts: V H L R E", NK_TEXT_LEFT);
    nk_label(ctx, "Document: Ctrl+S save, Ctrl+O load", NK_TEXT_LEFT);
    nk_label(ctx, "Wheel zooms at cursor", NK_TEXT_LEFT);
    nk_label(ctx, "Delete removes selection", NK_TEXT_LEFT);
}

static void ui_inspector_overview(struct nk_context* ctx, const Document* document, const GraphicObject* object)
{
    if (!ctx || !document || !object) {
        return;
    }

    nk_layout_row_dynamic(ctx, 20.0f, 1);
    nk_labelf(ctx, NK_TEXT_LEFT, "Type: %s", object_type_name(object->type));
    nk_labelf(ctx, NK_TEXT_LEFT, "Selected: %d", document->selection.count);
}

static void ui_inspector_style(struct nk_context* ctx, Workspace* workspace, GraphicObject* object)
{
    Color stroke;
    float stroke_width;

    if (!ctx || !workspace || !object) {
        return;
    }

    stroke = object->style.stroke_color;
    stroke_width = object->style.stroke_width;

    nk_layout_row_dynamic(ctx, 20.0f, 1);
    nk_label(ctx, "Style", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 22.0f, 2);
    nk_label(ctx, "Stroke R", NK_TEXT_LEFT);
    if (nk_slider_float(ctx, 0.0f, &stroke.r, 1.0f, 0.01f)) ui_apply_stroke_color(workspace, object, stroke);
    nk_label(ctx, "Stroke G", NK_TEXT_LEFT);
    if (nk_slider_float(ctx, 0.0f, &stroke.g, 1.0f, 0.01f)) ui_apply_stroke_color(workspace, object, stroke);
    nk_label(ctx, "Stroke B", NK_TEXT_LEFT);
    if (nk_slider_float(ctx, 0.0f, &stroke.b, 1.0f, 0.01f)) ui_apply_stroke_color(workspace, object, stroke);
    nk_label(ctx, "Stroke A", NK_TEXT_LEFT);
    if (nk_slider_float(ctx, 0.1f, &stroke.a, 1.0f, 0.01f)) ui_apply_stroke_color(workspace, object, stroke);
    nk_label(ctx, "Line Width", NK_TEXT_LEFT);
    if (nk_slider_float(ctx, 1.0f, &stroke_width, 12.0f, 0.1f)) ui_apply_stroke_width(workspace, object, stroke_width);
}

static void ui_inspector_geometry(struct nk_context* ctx, Workspace* workspace, GraphicObject* object)
{
    if (!ctx || !workspace || !object) {
        return;
    }

    if (object->type == GRAPHIC_OBJECT_LINE) {
        float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
        object_get_scalar(object, "x1", &x1);
        object_get_scalar(object, "y1", &y1);
        object_get_scalar(object, "x2", &x2);
        object_get_scalar(object, "y2", &y2);
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label(ctx, "Geometry", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 24.0f, 1);
        ui_property_apply_float(ctx, workspace, object, "#X1", "x1", -5000.0f, &x1, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#Y1", "y1", -5000.0f, &y1, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#X2", "x2", -5000.0f, &x2, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#Y2", "y2", -5000.0f, &y2, 5000.0f, 1.0f, 0.5f);
    } else {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
        object_get_scalar(object, "x", &x);
        object_get_scalar(object, "y", &y);
        object_get_scalar(object, "width", &width);
        object_get_scalar(object, "height", &height);
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label(ctx, "Bounds", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 24.0f, 1);
        ui_property_apply_float(ctx, workspace, object, "#X", "x", -5000.0f, &x, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#Y", "y", -5000.0f, &y, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#W", "width", 1.0f, &width, 5000.0f, 1.0f, 0.5f);
        ui_property_apply_float(ctx, workspace, object, "#H", "height", 1.0f, &height, 5000.0f, 1.0f, 0.5f);
    }
}

static void ui_selection_panel(UiSystem* ui, Workspace* workspace, RectF bounds)
{
    struct nk_context* ctx = ui->ctx;
    Document* document = &workspace->document;
    GraphicObject* object = document_primary_selection(document);

    ui->panel_bounds = bounds;
    if (bounds.w <= 0.0f || bounds.h <= 0.0f) {
        return;
    }

    if (nk_begin(ctx, "Inspector",
                 nk_rect(bounds.x, bounds.y, bounds.w, bounds.h),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCROLL_AUTO_HIDE)) {
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        if (!object) {
            ui_inspector_empty_hint(ctx);
        } else {
            ui_inspector_overview(ctx, document, object);
            ui_inspector_style(ctx, workspace, object);
            ui_inspector_geometry(ctx, workspace, object);
        }
    }
    nk_end(ctx);
}

static void ui_status_bar(UiSystem* ui, Workspace* workspace, int window_width, int window_height)
{
    struct nk_context* ctx = ui->ctx;
    const float margin = ui->theme.margin;
    const float status_h = ui->theme.status_height;
    const char* status_text = workspace->status_message[0] ? workspace->status_message : "Ready";
    float status_row_h = ui->theme.row_height * 0.65f;
    char zoom_text[24];

    ui->status_bounds.x = margin;
    ui->status_bounds.w = (float)window_width - (margin * 2.0f);
    ui->status_bounds.h = status_h;
    ui->status_bounds.y = (float)window_height - margin - status_h;

    if (ui->status_bounds.w <= 32.0f || ui->status_bounds.h <= 14.0f) {
        return;
    }

    snprintf(zoom_text, sizeof(zoom_text), "Zoom: %.0f%%", workspace->canvas.zoom * 100.0f);

    if (nk_begin(ctx, "Status",
                 nk_rect(ui->status_bounds.x, ui->status_bounds.y, ui->status_bounds.w, ui->status_bounds.h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, status_row_h, 5);
        nk_layout_row_push(ctx, 0.12f);
        nk_labelf(ctx, NK_TEXT_LEFT, "Objects: %d", workspace->document.count);
        nk_layout_row_push(ctx, 0.13f);
        nk_label(ctx, zoom_text, NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 0.35f);
        nk_labelf(ctx, NK_TEXT_LEFT, "File: %s%s",
                  workspace->current_document_path[0] ? workspace->current_document_path : "(default)",
                  workspace->document_dirty ? " *" : "");
        nk_layout_row_push(ctx, 0.18f);
        nk_labelf(ctx, NK_TEXT_LEFT, "Undo:%d Redo:%d", workspace->history.undo_count, workspace->history.redo_count);
        nk_layout_row_push(ctx, 0.22f);
        nk_label(ctx, status_text, NK_TEXT_RIGHT);
        nk_layout_row_end(ctx);
    }
    nk_end(ctx);
}

UiSystem* ui_system_create(PlatformWindow* window)
{
    UiSystem* ui = (UiSystem*)calloc(1, sizeof(*ui));
    struct nk_font_atlas* atlas = NULL;

    if (!ui) {
        return NULL;
    }

    ui->ctx = nk_glfw3_init(&ui->glfw, window->handle, 0);
    if (!ui->ctx) {
        free(ui);
        return NULL;
    }

    nk_glfw3_font_stash_begin(&ui->glfw, &atlas);
    nk_glfw3_font_stash_end(&ui->glfw);

    ui->theme = ui_theme_default_tokens();
    ui_theme_apply(ui->ctx, &ui->theme);

    ui->menu_bar = ui_menubar_create(ui->ctx);
    if (ui->menu_bar) {
        ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
    }

    ui->inspector_anim_t = 1.0f;
    ui->inspector_target_visible = 1;
    ui->inspector_anim_initialized = 0;
    ui->last_frame_seconds = glfwGetTime();

    return ui;
}

void ui_system_destroy(UiSystem* ui)
{
    if (!ui) {
        return;
    }
    if (ui->menu_bar) {
        ui_menubar_destroy(ui->menu_bar);
        ui->menu_bar = NULL;
    }
    nk_glfw3_shutdown(&ui->glfw);
    free(ui);
}

void ui_system_begin_frame(UiSystem* ui)
{
    if (ui) {
        nk_glfw3_new_frame(&ui->glfw);
    }
}

void ui_system_build(UiSystem* ui, Workspace* workspace)
{
    int width = 0;
    int height = 0;
    float content_top = 0.0f;
    float content_bottom = 0.0f;
    float content_height = 0.0f;
    float needed_width = 0.0f;
    int inspector_requested = 0;
    int inspector_target_visible = 0;
    int inspector_render_visible = 0;
    RectF rail_bounds = {0};
    RectF inspector_bounds = {0};
    double now_seconds;
    float dt_seconds;
    float anim_step;
    float inspector_eased_t;
    float inspector_hidden_x;
    float inspector_shown_x;

    if (!ui || !workspace) {
        return;
    }

    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    now_seconds = glfwGetTime();
    dt_seconds = (float)(now_seconds - ui->last_frame_seconds);
    ui->last_frame_seconds = now_seconds;
    dt_seconds = ui_clampf(dt_seconds, 0.0f, 0.10f);

    if (ui->menu_bar) {
        ui_menubar_set_height(ui->menu_bar, ui->theme.menu_height);
        ui_menubar_build(ui->menu_bar, workspace, width);
    }

    ui->appbar_bounds.x = 0.0f;
    ui->appbar_bounds.y = 0.0f;
    ui->appbar_bounds.w = (float)width;
    ui->appbar_bounds.h = ui_menubar_height(ui->menu_bar);

    content_top = ui->appbar_bounds.h + ui->theme.margin;
    content_bottom = (float)height - ui->theme.margin - ui->theme.status_height;
    content_height = content_bottom - content_top;
    if (content_height < 80.0f) {
        content_height = 80.0f;
    }

    rail_bounds.x = ui->theme.margin;
    rail_bounds.y = content_top;
    rail_bounds.w = ui->theme.tool_rail_width;
    rail_bounds.h = content_height;
    ui_tool_rail(ui, workspace, rail_bounds);

    inspector_requested = ui_menubar_inspector_visible(ui->menu_bar);
    needed_width = ui->theme.margin * 2.0f +
                   ui->theme.tool_rail_width +
                   ui->theme.gap +
                   UI_MIN_CANVAS_WIDTH;
    if (inspector_requested) {
        needed_width += ui->theme.gap + ui->theme.panel_width;
    }
    inspector_target_visible = inspector_requested && ((float)width >= needed_width);
    ui->inspector_target_visible = inspector_target_visible;

    if (!ui->inspector_anim_initialized) {
        ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
        ui->inspector_anim_initialized = 1;
    } else if (!ui->theme.enable_transitions || ui->theme.transition_duration <= 1e-4f) {
        ui->inspector_anim_t = inspector_target_visible ? 1.0f : 0.0f;
    } else {
        anim_step = dt_seconds / ui->theme.transition_duration;
        if (ui->inspector_target_visible) {
            ui->inspector_anim_t = ui_clampf(ui->inspector_anim_t + anim_step, 0.0f, 1.0f);
        } else {
            ui->inspector_anim_t = ui_clampf(ui->inspector_anim_t - anim_step, 0.0f, 1.0f);
        }
    }

    inspector_render_visible = (ui->inspector_anim_t > 1e-3f);
    if (inspector_render_visible) {
        inspector_eased_t = ui_smoothstep(ui->inspector_anim_t);
        inspector_bounds.w = ui->theme.panel_width;
        inspector_bounds.h = content_height;
        inspector_hidden_x = (float)width + ui->theme.gap;
        inspector_shown_x = (float)width - ui->theme.margin - inspector_bounds.w;
        inspector_bounds.x = inspector_hidden_x + (inspector_shown_x - inspector_hidden_x) * inspector_eased_t;
        inspector_bounds.y = content_top;
        ui_selection_panel(ui, workspace, inspector_bounds);
    } else {
        ui->panel_bounds.x = 0.0f;
        ui->panel_bounds.y = 0.0f;
        ui->panel_bounds.w = 0.0f;
        ui->panel_bounds.h = 0.0f;
    }

    ui_status_bar(ui, workspace, width, height);
}

void ui_system_render(UiSystem* ui)
{
    if (!ui) {
        return;
    }
    nk_glfw3_render(&ui->glfw, NK_ANTI_ALIASING_ON, UI_MAX_VERTEX_BUFFER, UI_MAX_ELEMENT_BUFFER);
}

int ui_system_blocks_pointer(const UiSystem* ui, Vec2 screen_pos)
{
    if (!ui || !ui->ctx) {
        return 0;
    }

    if (nk_item_is_any_active(ui->ctx)) {
        return 1;
    }

    return rectf_contains_point(&ui->appbar_bounds, screen_pos) ||
           rectf_contains_point(&ui->rail_bounds, screen_pos) ||
           rectf_contains_point(&ui->panel_bounds, screen_pos) ||
           rectf_contains_point(&ui->status_bounds, screen_pos);
}
