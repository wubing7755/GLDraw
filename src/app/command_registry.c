/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <base/log.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <tools/tool_controller.h>
#include <ui/ui_menu_def.h>

#include <string.h>

static const CommandDescriptor g_commands[] = {
    {EDITOR_COMMAND_FILE_NEW, "file.new", "New", KEY_SCOPE_GLOBAL, MENU_ID_FILE_NEW},
    {EDITOR_COMMAND_FILE_OPEN, "file.open", "Open", KEY_SCOPE_GLOBAL, MENU_ID_FILE_OPEN},
    {EDITOR_COMMAND_FILE_SAVE, "file.save", "Save", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE},
    {EDITOR_COMMAND_FILE_SAVE_AS, "file.save_as", "Save As", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE_AS},
    {EDITOR_COMMAND_FILE_EXIT, "file.exit", "Exit", KEY_SCOPE_GLOBAL, MENU_ID_FILE_EXIT},
    {EDITOR_COMMAND_EDIT_UNDO, "edit.undo", "Undo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_UNDO},
    {EDITOR_COMMAND_EDIT_REDO, "edit.redo", "Redo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_REDO},
    {EDITOR_COMMAND_EDIT_DELETE, "edit.delete", "Delete", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_DELETE},
    {EDITOR_COMMAND_EDIT_SELECT_ALL, "edit.select_all", "Select All", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_SELECT_ALL},
    {EDITOR_COMMAND_VIEW_ZOOM_IN, "view.zoom_in", "Zoom In", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_IN},
    {EDITOR_COMMAND_VIEW_ZOOM_OUT, "view.zoom_out", "Zoom Out", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_OUT},
    {EDITOR_COMMAND_VIEW_ZOOM_FIT, "view.zoom_fit", "Zoom to Fit", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_ZOOM_FIT},
    {EDITOR_COMMAND_VIEW_TOGGLE_GRID, "view.toggle_grid", "Toggle Grid", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_TOGGLE_GRID},
    {EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR, "view.toggle_inspector", "Toggle Inspector", KEY_SCOPE_GLOBAL, MENU_ID_VIEW_TOGGLE_INSPECTOR},
    {EDITOR_COMMAND_TOOL_SELECT, "tool.select", "Select Tool", KEY_SCOPE_GLOBAL, MENU_ID_EDIT},
    {EDITOR_COMMAND_TOOL_PAN, "tool.pan", "Pan Tool", KEY_SCOPE_GLOBAL, MENU_ID_EDIT},
    {EDITOR_COMMAND_TOOL_LINE, "tool.line", "Line Tool", KEY_SCOPE_GLOBAL, MENU_ID_EDIT},
    {EDITOR_COMMAND_TOOL_RECT, "tool.rect", "Rectangle Tool", KEY_SCOPE_GLOBAL, MENU_ID_EDIT},
    {EDITOR_COMMAND_TOOL_ELLIPSE, "tool.ellipse", "Ellipse Tool", KEY_SCOPE_GLOBAL, MENU_ID_EDIT},
    {EDITOR_COMMAND_HELP_SHORTCUTS, "help.shortcuts", "Keyboard Shortcuts", KEY_SCOPE_GLOBAL, MENU_ID_HELP_SHORTCUTS},
    {EDITOR_COMMAND_HELP_ABOUT, "help.about", "About", KEY_SCOPE_GLOBAL, MENU_ID_HELP_ABOUT},
    {EDITOR_COMMAND_MODAL_CONFIRM, "modal.confirm", "Confirm", KEY_SCOPE_MODAL, MENU_ID_HELP},
    {EDITOR_COMMAND_MODAL_CANCEL, "modal.cancel", "Cancel", KEY_SCOPE_MODAL, MENU_ID_HELP}
};

static int command_registry_zoom_to_fit(Workspace* workspace)
{
    Document* doc = NULL;
    CanvasView* canvas = NULL;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    int first = 1;
    int i = 0;

    if (!workspace || !workspace->canvas.document) {
        return 0;
    }

    doc = &workspace->document;
    canvas = &workspace->canvas;
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

    if (!first) {
        float pad = 50.0f;
        float content_w = 0.0f;
        float content_h = 0.0f;
        RectF canvas_viewport = canvas_view_viewport(canvas);
        float zoom_x = 1.0f;
        float zoom_y = 1.0f;
        float new_zoom = 1.0f;

        min_x -= pad;
        min_y -= pad;
        max_x += pad;
        max_y += pad;
        content_w = max_x - min_x;
        content_h = max_y - min_y;
        zoom_x = canvas_viewport.w / content_w;
        zoom_y = canvas_viewport.h / content_h;
        new_zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
        new_zoom = (new_zoom < 0.1f) ? 0.1f : (new_zoom > 12.0f) ? 12.0f : new_zoom;
        canvas_view_set_center_zoom(canvas,
                                    vec2_make((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f),
                                    new_zoom);
    }

    return 1;
}

static void command_registry_delete_selection(Workspace* workspace)
{
    DocumentSnapshot before_snapshot;

    if (!workspace || workspace->document.selection.count <= 0) {
        return;
    }

    document_snapshot_init(&before_snapshot);
    document_snapshot_capture(&before_snapshot, &workspace->document);
    document_delete_selection(&workspace->document);
    if (!document_history_push(&workspace->history, &before_snapshot, &workspace->document)) {
        document_snapshot_free(&before_snapshot);
    }
    workspace_sync_document_dirty(workspace);
}

static void command_registry_select_all(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    document_clear_selection(&workspace->document);
    for (i = 0; i < workspace->document.count; ++i) {
        document_selection_add(&workspace->document,
                               workspace->document.objects[i]->id);
    }
}

const CommandDescriptor* command_registry_find_by_id(const char* command_id)
{
    size_t i = 0u;

    if (!command_id) {
        return NULL;
    }

    for (i = 0u; i < sizeof(g_commands) / sizeof(g_commands[0]); ++i) {
        if (strcmp(g_commands[i].id, command_id) == 0) {
            return &g_commands[i];
        }
    }

    return NULL;
}

const CommandDescriptor* command_registry_find_by_menu_id(int id)
{
    size_t i = 0u;

    for (i = 0u; i < sizeof(g_commands) / sizeof(g_commands[0]); ++i) {
        if (g_commands[i].menu_id == id) {
            return &g_commands[i];
        }
    }

    return NULL;
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    if (!workspace) {
        return 0;
    }

    switch (command) {
    case EDITOR_COMMAND_FILE_NEW:
        return workspace_request_action(workspace, WORKSPACE_ACTION_NEW_DOCUMENT);
    case EDITOR_COMMAND_FILE_OPEN:
        return workspace_request_action(workspace, WORKSPACE_ACTION_OPEN_DOCUMENT);
    case EDITOR_COMMAND_FILE_SAVE:
        return workspace->save_document ? workspace->save_document(workspace, workspace->command_user_data) : 0;
    case EDITOR_COMMAND_FILE_SAVE_AS:
        return workspace->save_document ? workspace->save_document(workspace, workspace->command_user_data) : 0;
    case EDITOR_COMMAND_FILE_EXIT:
        return workspace_request_action(workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
    case EDITOR_COMMAND_EDIT_UNDO:
        if (document_history_undo(&workspace->history, &workspace->document)) {
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    case EDITOR_COMMAND_EDIT_REDO:
        if (document_history_redo(&workspace->history, &workspace->document)) {
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    case EDITOR_COMMAND_EDIT_DELETE:
        command_registry_delete_selection(workspace);
        return 1;
    case EDITOR_COMMAND_EDIT_SELECT_ALL:
        command_registry_select_all(workspace);
        return 1;
    case EDITOR_COMMAND_VIEW_ZOOM_IN:
    {
        Vec2 center = canvas_view_center(&workspace->canvas);
        canvas_view_zoom_at_screen_point(&workspace->canvas, 1.25f,
                                         canvas_view_world_to_screen(&workspace->canvas, center));
        return 1;
    }
    case EDITOR_COMMAND_VIEW_ZOOM_OUT:
    {
        Vec2 center = canvas_view_center(&workspace->canvas);
        canvas_view_zoom_at_screen_point(&workspace->canvas, 0.8f,
                                         canvas_view_world_to_screen(&workspace->canvas, center));
        return 1;
    }
    case EDITOR_COMMAND_VIEW_ZOOM_FIT:
        return command_registry_zoom_to_fit(workspace);
    case EDITOR_COMMAND_VIEW_TOGGLE_GRID:
        workspace->canvas.show_grid = !workspace->canvas.show_grid;
        return 1;
    case EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR:
        return 1;
    case EDITOR_COMMAND_TOOL_SELECT:
        if (tool_context) tool_controller_set_active(&workspace->tools, tool_context, TOOL_KIND_SELECT);
        return 1;
    case EDITOR_COMMAND_TOOL_PAN:
        if (tool_context) tool_controller_set_active(&workspace->tools, tool_context, TOOL_KIND_PAN);
        return 1;
    case EDITOR_COMMAND_TOOL_LINE:
        if (tool_context) tool_controller_set_active(&workspace->tools, tool_context, TOOL_KIND_LINE);
        return 1;
    case EDITOR_COMMAND_TOOL_RECT:
        if (tool_context) tool_controller_set_active(&workspace->tools, tool_context, TOOL_KIND_RECT);
        return 1;
    case EDITOR_COMMAND_TOOL_ELLIPSE:
        if (tool_context) tool_controller_set_active(&workspace->tools, tool_context, TOOL_KIND_ELLIPSE);
        return 1;
    case EDITOR_COMMAND_HELP_SHORTCUTS:
        LOG_INFO("%s", "Keyboard shortcuts dialog requested");
        return 1;
    case EDITOR_COMMAND_HELP_ABOUT:
        LOG_INFO("%s", "About dialog requested");
        return 1;
    case EDITOR_COMMAND_MODAL_CONFIRM:
        return workspace_confirm_pending_action_save(workspace);
    case EDITOR_COMMAND_MODAL_CANCEL:
        workspace_confirm_pending_action_cancel(workspace);
        return 1;
    case EDITOR_COMMAND_NONE:
    default:
        return 0;
    }
}

void command_registry_format_menu_shortcut(const Workspace* workspace,
                                           int id,
                                           char* buffer,
                                           size_t buffer_size)
{
    const CommandDescriptor* descriptor = NULL;

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!workspace) {
        return;
    }

    descriptor = command_registry_find_by_menu_id(id);
    if (!descriptor) {
        return;
    }

    keymap_format_command_shortcut(&workspace->keymap,
                                   descriptor->id,
                                   descriptor->scope,
                                   buffer,
                                   buffer_size);
}
