/**
 * @file command_registry.c
 * @brief Stable command definitions and execution.
 */
#include <app/command_registry.h>

#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <app/workspace_dialogs.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <tools/tool_controller.h>
#include <ui/ui_menu_def.h>

#include <stdio.h>
#include <string.h>

#define CLIPBOARD_PASTE_OFFSET_PIXELS 10.0f

static const char* command_registry_path_basename(const char* path)
{
    const char* basename = path;
    const char* cursor = NULL;

    if (!path || path[0] == '\0') {
        return "document.json";
    }

    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            basename = cursor + 1;
        }
    }

    return (basename && basename[0] != '\0') ? basename : "document.json";
}

static void command_registry_format_path_directory(const char* path,
                                                   char* buffer,
                                                   size_t buffer_size)
{
    const char* cursor = NULL;
    const char* last_separator = NULL;
    size_t length = 0u;

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!path || path[0] == '\0') {
        snprintf(buffer, buffer_size, ".");
        return;
    }

    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            last_separator = cursor;
        }
    }

    if (!last_separator) {
        snprintf(buffer, buffer_size, ".");
        return;
    }

    length = (size_t)(last_separator - path);
    if (length == 0u) {
        length = 1u;
    } else if (length == 2u && path[1] == ':') {
        length = 3u;
    }
    if (length >= buffer_size) {
        length = buffer_size - 1u;
    }

    memcpy(buffer, path, length);
    buffer[length] = '\0';
}

static int command_registry_open_save_as_dialog(Workspace* workspace)
{
    char directory[260];
    const char* path = NULL;

    if (!workspace) {
        return 0;
    }
    if (workspace_modal_is_active(workspace)) {
        return 0;
    }

    path = workspace->session.current_document_path[0] != '\0'
               ? workspace->session.current_document_path
               : "document.json";
    command_registry_format_path_directory(path, directory, sizeof(directory));
    return workspace_dialog_open_save_as(workspace,
                                         command_registry_path_basename(path),
                                         directory);
}

static const CommandDescriptor g_commands[] = {
    {EDITOR_COMMAND_FILE_NEW, "file.new", "New", KEY_SCOPE_GLOBAL, MENU_ID_FILE_NEW},
    {EDITOR_COMMAND_FILE_OPEN, "file.open", "Open", KEY_SCOPE_GLOBAL, MENU_ID_FILE_OPEN},
    {EDITOR_COMMAND_FILE_SAVE, "file.save", "Save", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE},
    {EDITOR_COMMAND_FILE_SAVE_AS, "file.save_as", "Save As", KEY_SCOPE_GLOBAL, MENU_ID_FILE_SAVE_AS},
    {EDITOR_COMMAND_FILE_EXPORT_PNG, "file.export_png", "Export as PNG", KEY_SCOPE_GLOBAL, MENU_ID_FILE_EXPORT_PNG},
    {EDITOR_COMMAND_FILE_EXIT, "file.exit", "Exit", KEY_SCOPE_GLOBAL, MENU_ID_FILE_EXIT},
    {EDITOR_COMMAND_EDIT_UNDO, "edit.undo", "Undo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_UNDO},
    {EDITOR_COMMAND_EDIT_REDO, "edit.redo", "Redo", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_REDO},
    {EDITOR_COMMAND_EDIT_CUT, "edit.cut", "Cut", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_CUT},
    {EDITOR_COMMAND_EDIT_COPY, "edit.copy", "Copy", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_COPY},
    {EDITOR_COMMAND_EDIT_PASTE, "edit.paste", "Paste", KEY_SCOPE_GLOBAL, MENU_ID_EDIT_PASTE},
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

static int command_registry_copy_selection(Workspace* workspace)
{
    GraphicObject* clipboard_objects[DOCUMENT_MAX_SELECTION];
    int selection_count = 0;
    int i = 0;

    if (!workspace) {
        return 0;
    }

    selection_count = workspace->session.selection.count;
    if (selection_count <= 0) {
        return 0;
    }

    memset(clipboard_objects, 0, sizeof(clipboard_objects));
    for (i = 0; i < selection_count; ++i) {
        GraphicObject* source = document_find_object(&workspace->core.document,
                                                     workspace->session.selection.ids[i]);

        if (!source) {
            break;
        }

        clipboard_objects[i] = object_clone(source);
        if (!clipboard_objects[i]) {
            break;
        }
    }

    if (i != selection_count) {
        int j = 0;
        for (j = 0; j < selection_count; ++j) {
            object_destroy(clipboard_objects[j]);
        }
        return 0;
    }

    workspace_clear_clipboard(workspace);
    for (i = 0; i < selection_count; ++i) {
        workspace->session.clipboard_objects[i] = clipboard_objects[i];
    }
    workspace->session.clipboard_count = selection_count;
    workspace->session.clipboard_paste_serial = 0;
    return 1;
}

static int command_registry_paste_clipboard(Workspace* workspace)
{
    DocumentSnapshot before_snapshot;
    SelectionSet before_selection;
    GraphicObject* pasted_objects[DOCUMENT_MAX_SELECTION];
    Vec2 paste_delta = vec2_make(0.0f, 0.0f);
    float world_offset = 0.0f;
    int paste_count = 0;
    int i = 0;

    if (!workspace) {
        return 0;
    }

    paste_count = workspace->session.clipboard_count;
    if (paste_count <= 0) {
        return 0;
    }
    if (workspace->core.document.count + paste_count > DOCUMENT_MAX_OBJECTS) {
        return 0;
    }

    memset(pasted_objects, 0, sizeof(pasted_objects));
    world_offset = canvas_view_world_tolerance_for_pixels(&workspace->core.canvas,
                                                          CLIPBOARD_PASTE_OFFSET_PIXELS);
    paste_delta = vec2_make(world_offset * (float)(workspace->session.clipboard_paste_serial + 1u),
                            -world_offset * (float)(workspace->session.clipboard_paste_serial + 1u));

    for (i = 0; i < paste_count; ++i) {
        pasted_objects[i] = object_clone(workspace->session.clipboard_objects[i]);
        if (!pasted_objects[i]) {
            break;
        }

        object_translate(pasted_objects[i], paste_delta);
    }

    if (i != paste_count) {
        int j = 0;
        for (j = 0; j < paste_count; ++j) {
            object_destroy(pasted_objects[j]);
        }
        return 0;
    }

    document_snapshot_init(&before_snapshot);
    before_selection = workspace->session.selection;
    if (!document_snapshot_capture(&before_snapshot, &workspace->core.document)) {
        for (i = 0; i < paste_count; ++i) {
            object_destroy(pasted_objects[i]);
        }
        return 0;
    }

    selection_set_clear(&workspace->session.selection);
    for (i = 0; i < paste_count; ++i) {
        if (!document_add_object(&workspace->core.document, pasted_objects[i])) {
            break;
        }

        selection_set_add(&workspace->session.selection, pasted_objects[i]->id);
    }

    if (i != paste_count) {
        document_snapshot_free(&before_snapshot);
        return 0;
    }

    if (!document_history_push(&workspace->core.history,
                               &before_snapshot,
                               &before_selection,
                               &workspace->core.document,
                               &workspace->session.selection)) {
        document_snapshot_free(&before_snapshot);
    }

    workspace->session.clipboard_paste_serial++;
    workspace_sync_document_dirty(workspace);
    return 1;
}

static void command_registry_append_shortcut_line(char* buffer,
                                                  size_t buffer_size,
                                                  const Workspace* workspace,
                                                  const char* command_id,
                                                  KeyScope scope,
                                                  const char* label)
{
    char shortcut[64];

    if (!buffer || buffer_size == 0u || !workspace || !command_id || !label) {
        return;
    }

    shortcut[0] = '\0';
    keymap_format_command_shortcut(&workspace->session.keymap,
                                   command_id,
                                   scope,
                                   shortcut,
                                   sizeof(shortcut));
    if (shortcut[0] == '\0') {
        return;
    }

    snprintf(buffer + strlen(buffer),
             buffer_size - strlen(buffer),
             "  %-16s %s\n",
             shortcut,
             label);
}

static int command_registry_toggle_shortcuts_dialog(Workspace* workspace)
{
    char content[1024];

    if (!workspace) {
        return 0;
    }

    if (workspace_active_dialog_kind(workspace) == UI_DIALOG_SHORTCUTS) {
        workspace_confirm_pending_action_cancel(workspace);
        return 1;
    }
    if (workspace_modal_is_active(workspace)) {
        return 0;
    }

    content[0] = '\0';
    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "Tools\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "tool.select", KEY_SCOPE_GLOBAL, "Select Tool");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "tool.pan", KEY_SCOPE_GLOBAL, "Pan/Hand Tool");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "tool.line", KEY_SCOPE_GLOBAL, "Line Tool");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "tool.rect", KEY_SCOPE_GLOBAL, "Rectangle Tool");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "tool.ellipse", KEY_SCOPE_GLOBAL, "Ellipse Tool");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nFile\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.new", KEY_SCOPE_GLOBAL, "New Document");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.open", KEY_SCOPE_GLOBAL, "Open Document");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "file.save", KEY_SCOPE_GLOBAL, "Save Document");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nEdit\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.undo", KEY_SCOPE_GLOBAL, "Undo");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.redo", KEY_SCOPE_GLOBAL, "Redo");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.cut", KEY_SCOPE_GLOBAL, "Cut");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.copy", KEY_SCOPE_GLOBAL, "Copy");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.paste", KEY_SCOPE_GLOBAL, "Paste");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.delete", KEY_SCOPE_GLOBAL, "Delete Selection");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "edit.select_all", KEY_SCOPE_GLOBAL, "Select All");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nView\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_fit", KEY_SCOPE_GLOBAL, "Zoom to Fit");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_in", KEY_SCOPE_GLOBAL, "Zoom In");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "view.zoom_out", KEY_SCOPE_GLOBAL, "Zoom Out");

    snprintf(content + strlen(content),
             sizeof(content) - strlen(content),
             "\nHelp\n");
    command_registry_append_shortcut_line(content, sizeof(content), workspace, "help.shortcuts", KEY_SCOPE_GLOBAL, "Toggle This Dialog");

    return workspace_dialog_open_shortcuts(workspace, content);
}
static int command_registry_zoom_to_fit(Workspace* workspace)
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

static void command_registry_delete_selection(Workspace* workspace)
{
    DocumentSnapshot before_snapshot;
    SelectionSet before_selection;
    ObjectId ids[DOCUMENT_MAX_SELECTION];
    int selection_count = 0;
    int i = 0;

    if (!workspace || workspace->session.selection.count <= 0) {
        return;
    }

    document_snapshot_init(&before_snapshot);
    before_selection = workspace->session.selection;
    document_snapshot_capture(&before_snapshot, &workspace->core.document);
    selection_count = workspace->session.selection.count;
    for (i = 0; i < selection_count; ++i) {
        ids[i] = workspace->session.selection.ids[i];
    }
    for (i = 0; i < selection_count; ++i) {
        document_remove_object(&workspace->core.document, ids[i]);
    }
    selection_set_clear(&workspace->session.selection);
    if (!document_history_push(&workspace->core.history,
                               &before_snapshot,
                               &before_selection,
                               &workspace->core.document,
                               &workspace->session.selection)) {
        document_snapshot_free(&before_snapshot);
    }
    workspace_sync_document_dirty(workspace);
}

static int command_registry_cut_selection(Workspace* workspace)
{
    if (!workspace || workspace->session.selection.count <= 0) {
        return 0;
    }

    if (!command_registry_copy_selection(workspace)) {
        return 0;
    }

    command_registry_delete_selection(workspace);
    return 1;
}

static void command_registry_select_all(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    selection_set_clear(&workspace->session.selection);
    for (i = 0; i < workspace->core.document.count; ++i) {
        selection_set_add(&workspace->session.selection,
                          workspace->core.document.objects[i]->id);
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

int command_registry_is_available(const Workspace* workspace,
                                  EditorCommand command)
{
    switch (command) {
    case EDITOR_COMMAND_FILE_SAVE:
        return workspace && workspace->services.save_document;
    case EDITOR_COMMAND_FILE_SAVE_AS:
        return workspace && workspace->services.save_as_document;
    case EDITOR_COMMAND_FILE_EXPORT_PNG:
        return workspace && workspace->services.export_png;
    case EDITOR_COMMAND_HELP_SHORTCUTS:
        return 1;
    case EDITOR_COMMAND_EDIT_CUT:
    case EDITOR_COMMAND_EDIT_COPY:
    case EDITOR_COMMAND_EDIT_DELETE:
        return workspace && workspace->session.selection.count > 0;
    case EDITOR_COMMAND_EDIT_PASTE:
        return workspace && workspace->session.clipboard_count > 0;
    case EDITOR_COMMAND_FILE_NEW:
    case EDITOR_COMMAND_FILE_OPEN:
    case EDITOR_COMMAND_FILE_EXIT:
    case EDITOR_COMMAND_EDIT_UNDO:
    case EDITOR_COMMAND_EDIT_REDO:
    case EDITOR_COMMAND_EDIT_SELECT_ALL:
    case EDITOR_COMMAND_VIEW_ZOOM_IN:
    case EDITOR_COMMAND_VIEW_ZOOM_OUT:
    case EDITOR_COMMAND_VIEW_ZOOM_FIT:
    case EDITOR_COMMAND_VIEW_TOGGLE_GRID:
    case EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR:
    case EDITOR_COMMAND_TOOL_SELECT:
    case EDITOR_COMMAND_TOOL_PAN:
    case EDITOR_COMMAND_TOOL_LINE:
    case EDITOR_COMMAND_TOOL_RECT:
    case EDITOR_COMMAND_TOOL_ELLIPSE:
    case EDITOR_COMMAND_HELP_ABOUT:
    case EDITOR_COMMAND_MODAL_CONFIRM:
    case EDITOR_COMMAND_MODAL_CANCEL:
        return 1;
    case EDITOR_COMMAND_NONE:
    default:
        return 0;
    }
}

int command_registry_is_menu_action_available(const Workspace* workspace, int id)
{
    const CommandDescriptor* descriptor = command_registry_find_by_menu_id(id);

    if (!descriptor) {
        return 0;
    }

    return command_registry_is_available(workspace, descriptor->command);
}

int command_registry_execute(Workspace* workspace,
                             ToolContext* tool_context,
                             EditorCommand command)
{
    if (!workspace) {
        return 0;
    }
    if (!command_registry_is_available(workspace, command)) {
        return 0;
    }

    switch (command) {
    case EDITOR_COMMAND_FILE_NEW:
        return workspace_request_action(workspace, WORKSPACE_ACTION_NEW_DOCUMENT);
    case EDITOR_COMMAND_FILE_OPEN:
        return workspace_request_action(workspace, WORKSPACE_ACTION_OPEN_DOCUMENT);
    case EDITOR_COMMAND_FILE_SAVE:
        return workspace->services.save_document ?
               workspace->services.save_document(workspace, workspace->services.command_user_data) : 0;
    case EDITOR_COMMAND_FILE_SAVE_AS:
        return command_registry_open_save_as_dialog(workspace);
    case EDITOR_COMMAND_FILE_EXPORT_PNG:
        return workspace->services.export_png ?
               workspace->services.export_png(workspace, workspace->services.command_user_data) : 0;
    case EDITOR_COMMAND_FILE_EXIT:
        return workspace_request_action(workspace, WORKSPACE_ACTION_EXIT_APPLICATION);
    case EDITOR_COMMAND_EDIT_UNDO:
        if (document_history_undo(&workspace->core.history,
                                  &workspace->core.document,
                                  &workspace->session.selection)) {
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    case EDITOR_COMMAND_EDIT_REDO:
        if (document_history_redo(&workspace->core.history,
                                  &workspace->core.document,
                                  &workspace->session.selection)) {
            workspace_sync_document_dirty(workspace);
            return 1;
        }
        return 0;
    case EDITOR_COMMAND_EDIT_CUT:
        return command_registry_cut_selection(workspace);
    case EDITOR_COMMAND_EDIT_COPY:
        return command_registry_copy_selection(workspace);
    case EDITOR_COMMAND_EDIT_PASTE:
        return command_registry_paste_clipboard(workspace);
    case EDITOR_COMMAND_EDIT_DELETE:
        command_registry_delete_selection(workspace);
        return 1;
    case EDITOR_COMMAND_EDIT_SELECT_ALL:
        command_registry_select_all(workspace);
        return 1;
    case EDITOR_COMMAND_VIEW_ZOOM_IN:
    {
        Vec2 center = canvas_view_center(&workspace->core.canvas);
        canvas_view_zoom_at_screen_point(&workspace->core.canvas, 1.25f,
                                         canvas_view_world_to_screen(&workspace->core.canvas, center));
        return 1;
    }
    case EDITOR_COMMAND_VIEW_ZOOM_OUT:
    {
        Vec2 center = canvas_view_center(&workspace->core.canvas);
        canvas_view_zoom_at_screen_point(&workspace->core.canvas, 0.8f,
                                         canvas_view_world_to_screen(&workspace->core.canvas, center));
        return 1;
    }
    case EDITOR_COMMAND_VIEW_ZOOM_FIT:
        return command_registry_zoom_to_fit(workspace);
    case EDITOR_COMMAND_VIEW_TOGGLE_GRID:
        workspace->core.canvas.show_grid = !workspace->core.canvas.show_grid;
        return 1;
    case EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR:
        return 1;
    case EDITOR_COMMAND_TOOL_SELECT:
        if (tool_context) tool_controller_set_active(&workspace->core.tools, tool_context, TOOL_KIND_SELECT);
        return 1;
    case EDITOR_COMMAND_TOOL_PAN:
        if (tool_context) tool_controller_set_active(&workspace->core.tools, tool_context, TOOL_KIND_PAN);
        return 1;
    case EDITOR_COMMAND_TOOL_LINE:
        if (tool_context) tool_controller_set_active(&workspace->core.tools, tool_context, TOOL_KIND_LINE);
        return 1;
    case EDITOR_COMMAND_TOOL_RECT:
        if (tool_context) tool_controller_set_active(&workspace->core.tools, tool_context, TOOL_KIND_RECT);
        return 1;
    case EDITOR_COMMAND_TOOL_ELLIPSE:
        if (tool_context) tool_controller_set_active(&workspace->core.tools, tool_context, TOOL_KIND_ELLIPSE);
        return 1;
    case EDITOR_COMMAND_HELP_SHORTCUTS:
        return command_registry_toggle_shortcuts_dialog(workspace);
    case EDITOR_COMMAND_HELP_ABOUT:
        return workspace_dialog_open_info(workspace,
                                          "About GLDraw",
                                          "GLDraw\nCanvas-oriented OpenGL drawing editor.\n\nCurrent build includes core document editing, undo/redo, persistence, and themeable UI.");
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

    keymap_format_command_shortcut(&workspace->session.keymap,
                                   descriptor->id,
                                   descriptor->scope,
                                   buffer,
                                   buffer_size);
}
