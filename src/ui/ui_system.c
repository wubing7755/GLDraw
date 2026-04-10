#include <ui/ui_system.h>

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

#include <stdio.h>
#include <stdlib.h>

#define UI_MAX_VERTEX_BUFFER (512 * 1024)
#define UI_MAX_ELEMENT_BUFFER (128 * 1024)

struct UiSystem {
    struct nk_glfw glfw;
    struct nk_context* ctx;
    RectF toolbar_bounds;
    RectF panel_bounds;
    RectF status_bounds;
};

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

static void ui_run_workspace_command(Workspace* workspace, WorkspaceCommandFn command)
{
    if (!workspace || !command) {
        return;
    }

    command(workspace, workspace->command_user_data);
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

static void ui_toolbar(UiSystem* ui, Workspace* workspace, int window_width)
{
    struct nk_context* ctx = ui->ctx;
    ToolContext context = ui_tool_context(workspace);

    ui->toolbar_bounds.x = 12.0f;
    ui->toolbar_bounds.y = 12.0f;
    ui->toolbar_bounds.w = (float)(window_width - 24);
    ui->toolbar_bounds.h = 54.0f;

    if (nk_begin(ctx, "Toolbar",
                 nk_rect(ui->toolbar_bounds.x, ui->toolbar_bounds.y, ui->toolbar_bounds.w, ui->toolbar_bounds.h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 28.0f, 11);
        if (nk_button_label(ctx, "Save")) ui_run_workspace_command(workspace, workspace->save_document);
        if (nk_button_label(ctx, "Load")) ui_run_workspace_command(workspace, workspace->load_document);
        if (nk_button_label(ctx, "Undo") && document_history_undo(&workspace->history, &workspace->document)) workspace_sync_document_dirty(workspace);
        if (nk_button_label(ctx, "Redo") && document_history_redo(&workspace->history, &workspace->document)) workspace_sync_document_dirty(workspace);
        if (nk_button_label(ctx, "Select")) tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_SELECT);
        if (nk_button_label(ctx, "Hand")) tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_PAN);
        if (nk_button_label(ctx, "Line")) tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_LINE);
        if (nk_button_label(ctx, "Rect")) tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_RECT);
        if (nk_button_label(ctx, "Ellipse")) tool_controller_set_active(&workspace->tools, &context, TOOL_KIND_ELLIPSE);
        nk_label(ctx, tool_controller_active_label(&workspace->tools), NK_TEXT_LEFT);
        {
            char zoom_text[32];
            snprintf(zoom_text, sizeof(zoom_text), "%.0f%%", workspace->canvas.zoom * 100.0f);
            nk_label(ctx, zoom_text, NK_TEXT_RIGHT);
        }
    }
    nk_end(ctx);
}

static void ui_selection_panel(UiSystem* ui, Workspace* workspace, int window_width, int window_height)
{
    struct nk_context* ctx = ui->ctx;
    Document* document = &workspace->document;
    GraphicObject* object = document_primary_selection(document);
    Color stroke = {0.0f, 0.0f, 0.0f, 0.0f};
    float stroke_width = 0.0f;

    ui->panel_bounds.w = 280.0f;
    ui->panel_bounds.h = (float)(window_height - 146);
    ui->panel_bounds.x = (float)(window_width - 292);
    ui->panel_bounds.y = 78.0f;

    if (nk_begin(ctx, "Inspector",
                 nk_rect(ui->panel_bounds.x, ui->panel_bounds.y, ui->panel_bounds.w, ui->panel_bounds.h),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCROLL_AUTO_HIDE)) {
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        if (!object) {
            nk_label(ctx, "No selection", NK_TEXT_LEFT);
            nk_label(ctx, "Shortcuts: V H L R E", NK_TEXT_LEFT);
            nk_label(ctx, "Document: Ctrl+S save, Ctrl+O load", NK_TEXT_LEFT);
            nk_label(ctx, "Wheel zooms at cursor", NK_TEXT_LEFT);
            nk_label(ctx, "Delete removes selection", NK_TEXT_LEFT);
        } else {
            nk_labelf(ctx, NK_TEXT_LEFT, "Type: %s", object_type_name(object->type));
            nk_labelf(ctx, NK_TEXT_LEFT, "Selected: %d", document->selection.count);

            stroke = object->style.stroke_color;
            stroke_width = object->style.stroke_width;

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

            if (object->type == GRAPHIC_OBJECT_LINE) {
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                object_get_scalar(object, "x1", &x1);
                object_get_scalar(object, "y1", &y1);
                object_get_scalar(object, "x2", &x2);
                object_get_scalar(object, "y2", &y2);
                nk_layout_row_dynamic(ctx, 24.0f, 1);
                nk_label(ctx, "Geometry", NK_TEXT_LEFT);
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
                nk_layout_row_dynamic(ctx, 24.0f, 1);
                nk_label(ctx, "Bounds", NK_TEXT_LEFT);
                ui_property_apply_float(ctx, workspace, object, "#X", "x", -5000.0f, &x, 5000.0f, 1.0f, 0.5f);
                ui_property_apply_float(ctx, workspace, object, "#Y", "y", -5000.0f, &y, 5000.0f, 1.0f, 0.5f);
                ui_property_apply_float(ctx, workspace, object, "#W", "width", 1.0f, &width, 5000.0f, 1.0f, 0.5f);
                ui_property_apply_float(ctx, workspace, object, "#H", "height", 1.0f, &height, 5000.0f, 1.0f, 0.5f);
            }
        }
    }
    nk_end(ctx);
}

static void ui_status_bar(UiSystem* ui, Workspace* workspace, int window_width, int window_height)
{
    struct nk_context* ctx = ui->ctx;
    RectF visible = canvas_view_visible_world_rect(&workspace->canvas);
    const char* status_text = workspace->status_message[0] ? workspace->status_message : "Ready";

    ui->status_bounds.x = 12.0f;
    ui->status_bounds.w = (float)(window_width - 24);
    ui->status_bounds.h = 36.0f;
    ui->status_bounds.y = (float)(window_height - 48);

    if (nk_begin(ctx, "Status",
                 nk_rect(ui->status_bounds.x, ui->status_bounds.y, ui->status_bounds.w, ui->status_bounds.h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, 18.0f, 6);
        nk_layout_row_push(ctx, 0.10f);
        nk_labelf(ctx, NK_TEXT_LEFT, "Objects: %d", workspace->document.count);
        nk_layout_row_push(ctx, 0.15f);
        nk_labelf(ctx, NK_TEXT_LEFT, "Center: %.1f, %.1f", workspace->canvas.center.x, workspace->canvas.center.y);
        nk_layout_row_push(ctx, 0.12f);
        nk_labelf(ctx, NK_TEXT_LEFT, "Visible: %.0f x %.0f", visible.w, visible.h);
        nk_layout_row_push(ctx, 0.18f);
        nk_labelf(ctx, NK_TEXT_LEFT, "File: %s%s",
                  workspace->current_document_path[0] ? workspace->current_document_path : "(default)",
                  workspace->document_dirty ? " *" : "");
        nk_layout_row_push(ctx, 0.28f);
        nk_label(ctx, status_text, NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 0.17f);
        nk_labelf(ctx, NK_TEXT_RIGHT, "Undo:%d Redo:%d", workspace->history.undo_count, workspace->history.redo_count);
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
    return ui;
}

void ui_system_destroy(UiSystem* ui)
{
    if (!ui) {
        return;
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

    if (!ui || !workspace) {
        return;
    }

    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    ui_toolbar(ui, workspace, width);
    ui_selection_panel(ui, workspace, width, height);
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

    return rectf_contains_point(&ui->toolbar_bounds, screen_pos) ||
           rectf_contains_point(&ui->panel_bounds, screen_pos) ||
           rectf_contains_point(&ui->status_bounds, screen_pos);
}
