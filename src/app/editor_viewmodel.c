#include <ui/editor_viewmodel.h>

#include <app/workspace.h>
#include <canvas/canvas_view.h>

#include <stdio.h>
#include <string.h>

typedef struct EditorCommandMapEntry {
    EditorCommand command;
    const char* command_id;
    KeyScope scope;
} EditorCommandMapEntry;

static const EditorCommandMapEntry g_command_map[] = {
    {EDITOR_COMMAND_FILE_NEW, "file.new", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_FILE_OPEN, "file.open", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_FILE_SAVE, "file.save", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_FILE_SAVE_AS, "file.save_as", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_FILE_EXPORT_PNG, "file.export_png", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_FILE_EXIT, "file.exit", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_UNDO, "edit.undo", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_REDO, "edit.redo", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_CUT, "edit.cut", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_COPY, "edit.copy", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_PASTE, "edit.paste", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_DELETE, "edit.delete", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_EDIT_SELECT_ALL, "edit.select_all", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_VIEW_ZOOM_IN, "view.zoom_in", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_VIEW_ZOOM_OUT, "view.zoom_out", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_VIEW_ZOOM_FIT, "view.zoom_fit", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_VIEW_TOGGLE_GRID, "view.toggle_grid", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR, "view.toggle_inspector", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_TOOL_SELECT, "tool.select", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_TOOL_PAN, "tool.pan", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_TOOL_LINE, "tool.line", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_TOOL_RECT, "tool.rect", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_TOOL_ELLIPSE, "tool.ellipse", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_HELP_SHORTCUTS, "help.shortcuts", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_HELP_ABOUT, "help.about", KEY_SCOPE_GLOBAL},
    {EDITOR_COMMAND_MODAL_CONFIRM, "modal.confirm", KEY_SCOPE_MODAL},
    {EDITOR_COMMAND_MODAL_CANCEL, "modal.cancel", KEY_SCOPE_MODAL}
};

static void editor_copy_string(char* dst, size_t dst_size, const char* src)
{
    if (!dst || dst_size == 0u) {
        return;
    }

    snprintf(dst, dst_size, "%s", src ? src : "");
}

static void editor_build_tooltip(char* dst,
                                 size_t dst_size,
                                 const char* base_tooltip,
                                 const char* unavailable_reason)
{
    if (!dst || dst_size == 0u) {
        return;
    }

    if (unavailable_reason && unavailable_reason[0] != '\0') {
        snprintf(dst,
                 dst_size,
                 "%s\nUnavailable: %s",
                 base_tooltip ? base_tooltip : "",
                 unavailable_reason);
        return;
    }

    editor_copy_string(dst, dst_size, base_tooltip);
}

static const EditorCommandMapEntry* editor_command_map_lookup(EditorCommand command)
{
    size_t i = 0u;

    for (i = 0u; i < sizeof(g_command_map) / sizeof(g_command_map[0]); ++i) {
        if (g_command_map[i].command == command) {
            return &g_command_map[i];
        }
    }

    return NULL;
}

void editor_viewmodel_init(EditorViewModel* view_model)
{
    if (!view_model) {
        return;
    }

    memset(view_model, 0, sizeof(*view_model));
}

static void editor_viewmodel_build_commands(EditorViewModel* view_model,
                                            const Workspace* workspace)
{
    int i = 0;

    if (!view_model || !workspace) {
        return;
    }

    for (i = 1; i < EDITOR_VIEWMODEL_MAX_COMMANDS; ++i) {
        const EditorCommandMapEntry* map_entry =
            editor_command_map_lookup((EditorCommand)i);

        view_model->commands[i].available =
            command_registry_is_available(workspace, (EditorCommand)i);
        view_model->commands[i].shortcut[0] = '\0';
        view_model->commands[i].unavailable_reason[0] = '\0';

        if (map_entry) {
            keymap_format_command_shortcut(&workspace->session.keymap,
                                           map_entry->command_id,
                                           map_entry->scope,
                                           view_model->commands[i].shortcut,
                                           sizeof(view_model->commands[i].shortcut));
        }
        editor_copy_string(view_model->commands[i].unavailable_reason,
                           sizeof(view_model->commands[i].unavailable_reason),
                           command_registry_unavailable_reason(workspace,
                                                               (EditorCommand)i));
    }
}

static void editor_viewmodel_build_tools(EditorViewModel* view_model,
                                         const Workspace* workspace)
{
    int tool_count = 0;
    int i = 0;
    const char* active_id = NULL;

    if (!view_model || !workspace) {
        return;
    }

    active_id = tool_controller_active_id(&workspace->core.tools);
    tool_count = tool_controller_tool_count(&workspace->core.tools);
    if (tool_count > TOOL_MAX_TYPES) {
        tool_count = TOOL_MAX_TYPES;
    }

    view_model->tool_count = tool_count;
    for (i = 0; i < tool_count; ++i) {
        const ToolDescriptor* descriptor =
            tool_controller_tool_descriptor_at(&workspace->core.tools, i);
        EditorToolView* tool_view = &view_model->tools[i];
        const CommandDescriptor* command_descriptor = NULL;

        if (!descriptor) {
            continue;
        }

        editor_copy_string(tool_view->id, sizeof(tool_view->id), descriptor->id);
        editor_copy_string(tool_view->name, sizeof(tool_view->name), descriptor->name);
        editor_copy_string(tool_view->icon, sizeof(tool_view->icon), descriptor->icon);
        tool_view->active = (active_id && strcmp(active_id, descriptor->id) == 0);
        tool_view->available = 1;
        tool_view->command = EDITOR_COMMAND_NONE;
        tool_view->shortcut[0] = '\0';

        command_descriptor = descriptor->command_id
                                 ? command_registry_find_by_id(descriptor->command_id)
                                 : NULL;
        if (command_descriptor) {
            tool_view->command = command_descriptor->command;
            tool_view->available =
                editor_viewmodel_command_available(view_model, command_descriptor->command);
            editor_copy_string(tool_view->shortcut,
                               sizeof(tool_view->shortcut),
                               editor_viewmodel_command_shortcut(view_model,
                                                                 command_descriptor->command));
        }

        editor_build_tooltip(tool_view->tooltip,
                             sizeof(tool_view->tooltip),
                             descriptor->tooltip ? descriptor->tooltip : descriptor->name,
                             tool_view->available
                                 ? ""
                                 : command_registry_unavailable_reason(workspace,
                                                                       tool_view->command));
    }
}

static void editor_viewmodel_build_properties(EditorViewModel* view_model,
                                              const Workspace* workspace)
{
    const GraphicObject* object = NULL;
    const GraphicPropertyDef* schema = NULL;
    int object_editable = 0;
    int property_count = 0;
    int i = 0;

    if (!view_model || !workspace) {
        return;
    }

    object = selection_set_primary_object(&workspace->session.selection,
                                          &workspace->core.document);
    view_model->has_selection = (object != NULL);
    if (!object) {
        return;
    }

    object_editable =
        !document_layer_is_locked(&workspace->core.document, object->layer_id);
    editor_copy_string(view_model->summary.selection_type_name,
                       sizeof(view_model->summary.selection_type_name),
                       object_type_name(object->type));
    schema = object_property_schema(object, &property_count);
    if (!schema || property_count <= 0) {
        return;
    }
    if (property_count > GRAPHIC_OBJECT_MAX_PROPERTIES) {
        property_count = GRAPHIC_OBJECT_MAX_PROPERTIES;
    }

    view_model->property_count = property_count;
    for (i = 0; i < property_count; ++i) {
        EditorPropertyView* property_view = &view_model->properties[i];

        property_view->object_id = object->id;
        editor_copy_string(property_view->name,
                           sizeof(property_view->name),
                           schema[i].name);
        property_view->type = schema[i].type;
        property_view->editable = object_editable;
        property_view->min_value = schema[i].min_value;
        property_view->max_value = schema[i].max_value;
        property_view->step = schema[i].step;
        property_view->inc_per_pixel = schema[i].inc_per_pixel;
        object_get_scalar(object, schema[i].name, &property_view->value);
    }
}

static void editor_viewmodel_build_layers(EditorViewModel* view_model,
                                          const Workspace* workspace)
{
    int i = 0;
    int j = 0;

    if (!view_model || !workspace) {
        return;
    }

    view_model->layer_count = document_layer_count(&workspace->core.document);
    if (view_model->layer_count > DOCUMENT_MAX_LAYERS) {
        view_model->layer_count = DOCUMENT_MAX_LAYERS;
    }

    for (i = 0; i < view_model->layer_count; ++i) {
        const DocumentLayer* layer = document_layer_at(&workspace->core.document, i);
        EditorLayerView* layer_view = &view_model->layers[i];

        if (!layer) {
            continue;
        }

        layer_view->id = layer->id;
        editor_copy_string(layer_view->name, sizeof(layer_view->name), layer->name);
        layer_view->visible = layer->visible;
        layer_view->locked = layer->locked;
        layer_view->active = (workspace->core.document.active_layer_id == layer->id);
        layer_view->object_count = 0;

        for (j = 0; j < workspace->core.document.count; ++j) {
            const GraphicObject* object = workspace->core.document.objects[j];
            if (object && object->layer_id == layer->id) {
                layer_view->object_count++;
            }
        }
    }
}

int editor_viewmodel_build(EditorViewModel* view_model, const Workspace* workspace)
{
    if (!view_model || !workspace) {
        return 0;
    }

    editor_viewmodel_init(view_model);
    view_model->summary.object_count = workspace->core.document.count;
    view_model->summary.selection_count = workspace->session.selection.count;
    view_model->summary.undo_count =
        (int)(workspace->core.commands.undo_count + (size_t)workspace->core.history.undo_count);
    view_model->summary.redo_count =
        (int)(workspace->core.commands.redo_count + (size_t)workspace->core.history.redo_count);
    view_model->summary.zoom_percent = canvas_view_zoom(&workspace->core.canvas) * 100.0f;
    view_model->summary.document_dirty = workspace->session.document_dirty;
    editor_copy_string(view_model->summary.current_document_path,
                       sizeof(view_model->summary.current_document_path),
                       workspace->session.current_document_path[0] != '\0'
                           ? workspace->session.current_document_path
                           : "(default)");
    editor_copy_string(view_model->summary.status_message,
                       sizeof(view_model->summary.status_message),
                       workspace->session.status_message[0] != '\0'
                           ? workspace->session.status_message
                           : "Ready");
    editor_copy_string(view_model->summary.active_tool_label,
                       sizeof(view_model->summary.active_tool_label),
                       tool_controller_active_label(&workspace->core.tools));
    view_model->active_dialog = workspace->session.active_dialog;

    editor_viewmodel_build_commands(view_model, workspace);
    editor_viewmodel_build_tools(view_model, workspace);
    editor_viewmodel_build_properties(view_model, workspace);
    editor_viewmodel_build_layers(view_model, workspace);
    return 1;
}

int editor_viewmodel_command_available(const EditorViewModel* view_model,
                                       EditorCommand command)
{
    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return 0;
    }

    return view_model->commands[command].available;
}

const char* editor_viewmodel_command_shortcut(const EditorViewModel* view_model,
                                              EditorCommand command)
{
    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return "";
    }

    return view_model->commands[command].shortcut;
}

const char* editor_viewmodel_command_unavailable_reason(const EditorViewModel* view_model,
                                                        EditorCommand command)
{
    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return "";
    }

    return view_model->commands[command].unavailable_reason;
}
