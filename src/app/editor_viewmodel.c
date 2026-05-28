#include <ui/editor_viewmodel.h>

#include <app/command_availability.h>
#include <app/command_catalog.h>
#include <app/workspace.h>
#include <canvas/canvas_view.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct EditorViewModelBuildContext {
    const Workspace* workspace;
    const CanvasView* canvas;
    const CommandExecutor* commands;
    const Document* document;
    const SelectionSet* selection;
    const ToolController* tools;
    const EditorKeymap* keymap;
    const UiDialogState* active_dialog;
    const char* document_path;
    const char* status_message;
} EditorViewModelBuildContext;

typedef struct EditorSelectionSnapshot {
    const GraphicObject* object;
    const char* type_name;
    int has_selection;
    int editable;
} EditorSelectionSnapshot;

static int editor_viewmodel_capture_context(EditorViewModelBuildContext* context,
                                            const Workspace* workspace)
{
    if (!context || !workspace) {
        return 0;
    }

    memset(context, 0, sizeof(*context));
    context->workspace = workspace;
    context->canvas = workspace_get_canvas_const(workspace);
    context->commands = workspace_get_command_executor_const(workspace);
    context->document = workspace_get_document_const(workspace);
    context->selection = workspace_get_selection_const(workspace);
    context->tools = workspace_get_tool_controller_const(workspace);
    context->keymap = workspace_get_keymap_const(workspace);
    context->active_dialog = workspace_get_active_dialog_const(workspace);
    context->document_path = workspace_get_current_document_path(workspace);
    context->status_message = workspace_get_status_message(workspace);
    return 1;
}

static EditorSelectionSnapshot editor_viewmodel_capture_selection(
    const EditorViewModelBuildContext* context)
{
    EditorSelectionSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    if (!context || !context->document || !context->selection ||
        context->selection->count <= 0) {
        return snapshot;
    }

    snapshot.object =
        document_find_object(context->document, context->selection->ids[0]);
    snapshot.has_selection = snapshot.object != NULL;
    if (!snapshot.object) {
        return snapshot;
    }

    snapshot.editable =
        !document_layer_is_locked(context->document, snapshot.object->layer_id);
    snapshot.type_name = object_type_name(snapshot.object->type);
    return snapshot;
}

void editor_viewmodel_init(EditorViewModel* view_model)
{
    if (!view_model) {
        return;
    }

    memset(view_model, 0, sizeof(*view_model));
}

void editor_viewmodel_shutdown(EditorViewModel* view_model)
{
    if (!view_model) {
        return;
    }

    free(view_model->tools);
    view_model->tools = NULL;
    view_model->tool_count = 0;
    view_model->tool_capacity = 0;

    free(view_model->layers);
    view_model->layers = NULL;
    view_model->layer_count = 0;
    view_model->layer_capacity = 0;
}

static void editor_viewmodel_clear_for_build(EditorViewModel* view_model)
{
    EditorToolView* tools = NULL;
    EditorLayerView* layers = NULL;
    int tool_capacity = 0;
    int layer_capacity = 0;

    if (!view_model) {
        return;
    }

    tools = view_model->tools;
    layers = view_model->layers;
    tool_capacity = view_model->tool_capacity;
    layer_capacity = view_model->layer_capacity;

    memset(view_model, 0, sizeof(*view_model));
    view_model->tools = tools;
    view_model->tool_capacity = tool_capacity;
    view_model->layers = layers;
    view_model->layer_capacity = layer_capacity;
}

static void editor_viewmodel_build_commands(EditorViewModel* view_model,
                                            const EditorViewModelBuildContext* context)
{
    int i = 0;

    if (!view_model || !context || !context->workspace) {
        return;
    }

    for (i = 1; i < EDITOR_VIEWMODEL_MAX_COMMANDS; ++i) {
        const CommandDescriptor* descriptor =
            command_catalog_find_by_command((EditorCommand)i);

        view_model->commands[i].available =
            command_availability_is_available(context->workspace,
                                             (EditorCommand)i);
        view_model->commands[i].shortcut[0] = '\0';
        view_model->commands[i].unavailable_reason[0] = '\0';

        if (descriptor && context->keymap) {
            keymap_format_command_shortcut(context->keymap,
                                           descriptor->id,
                                           descriptor->scope,
                                           view_model->commands[i].shortcut,
                                           sizeof(view_model->commands[i].shortcut));
        }
        editor_copy_string(view_model->commands[i].unavailable_reason,
                           sizeof(view_model->commands[i].unavailable_reason),
                           command_availability_unavailable_reason(
                               context->workspace,
                               (EditorCommand)i));
    }
}

static int editor_viewmodel_reserve_tools(EditorViewModel* view_model, int needed)
{
    EditorToolView* tools = NULL;
    int capacity = 0;

    if (!view_model || needed <= 0) {
        return view_model != NULL;
    }
    if (needed <= view_model->tool_capacity) {
        return 1;
    }

    capacity = view_model->tool_capacity > 0 ? view_model->tool_capacity : 8;
    while (capacity < needed) {
        capacity *= 2;
    }

    tools = (EditorToolView*)realloc(view_model->tools,
                                     (size_t)capacity * sizeof(view_model->tools[0]));
    if (!tools) {
        return 0;
    }

    view_model->tools = tools;
    view_model->tool_capacity = capacity;
    return 1;
}

static void editor_viewmodel_build_tools(EditorViewModel* view_model,
                                         const EditorViewModelBuildContext* context)
{
    int tool_count = 0;
    int i = 0;
    const char* active_id = NULL;

    if (!view_model || !context || !context->workspace || !context->tools) {
        return;
    }

    active_id = tool_controller_active_id(context->tools);
    tool_count = tool_controller_tool_count(context->tools);
    if (!editor_viewmodel_reserve_tools(view_model, tool_count)) {
        view_model->tool_count = 0;
        return;
    }

    view_model->tool_count = tool_count;
    for (i = 0; i < tool_count; ++i) {
        const ToolDescriptor* descriptor =
            tool_controller_tool_descriptor_at(context->tools, i);
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
        tool_view->unavailable_reason[0] = '\0';

        command_descriptor = descriptor->command_id
                                 ? command_catalog_find_by_id(descriptor->command_id)
                                 : NULL;
        if (command_descriptor) {
            tool_view->command = command_descriptor->command;
            tool_view->available =
                command_availability_is_available(context->workspace,
                                                 command_descriptor->command);
            if (context->keymap) {
                keymap_format_command_shortcut(context->keymap,
                                               descriptor->command_id,
                                               KEY_SCOPE_GLOBAL,
                                               tool_view->shortcut,
                                               sizeof(tool_view->shortcut));
            }
            editor_copy_string(tool_view->unavailable_reason,
                               sizeof(tool_view->unavailable_reason),
                               command_availability_unavailable_reason(
                                   context->workspace,
                                   command_descriptor->command));
        }

        editor_build_tooltip(tool_view->tooltip,
                             sizeof(tool_view->tooltip),
                             descriptor->tooltip ? descriptor->tooltip : descriptor->name,
                             tool_view->available ? "" : tool_view->unavailable_reason);
    }
}

static void editor_viewmodel_build_properties(
    EditorViewModel* view_model,
    const EditorViewModelBuildContext* context,
    const EditorSelectionSnapshot* selection_snapshot)
{
    const GraphicObject* object = NULL;
    const GraphicPropertyDef* schema = NULL;
    int property_count = 0;
    int i = 0;

    if (!view_model || !context || !context->document || !selection_snapshot) {
        return;
    }

    object = selection_snapshot->object;
    view_model->has_selection = selection_snapshot->has_selection;
    if (!object) {
        return;
    }

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
        property_view->editable = selection_snapshot->editable;
        property_view->min_value = schema[i].min_value;
        property_view->max_value = schema[i].max_value;
        property_view->step = schema[i].step;
        property_view->inc_per_pixel = schema[i].inc_per_pixel;
        object_get_scalar(object, schema[i].name, &property_view->value);
    }
}

static int editor_viewmodel_reserve_layers(EditorViewModel* view_model, int needed)
{
    EditorLayerView* layers = NULL;
    int capacity = 0;

    if (!view_model || needed <= 0) {
        return view_model != NULL;
    }
    if (needed <= view_model->layer_capacity) {
        return 1;
    }

    capacity = view_model->layer_capacity > 0 ? view_model->layer_capacity : 8;
    while (capacity < needed) {
        capacity *= 2;
    }

    layers = (EditorLayerView*)realloc(view_model->layers,
                                       (size_t)capacity * sizeof(view_model->layers[0]));
    if (!layers) {
        return 0;
    }

    view_model->layers = layers;
    view_model->layer_capacity = capacity;
    return 1;
}

static void editor_viewmodel_build_layers(EditorViewModel* view_model,
                                          const EditorViewModelBuildContext* context)
{
    int i = 0;
    int j = 0;
    int doc_layer_count = 0;

    if (!view_model || !context || !context->document) {
        return;
    }

    doc_layer_count = document_layer_count(context->document);
    if (!editor_viewmodel_reserve_layers(view_model, doc_layer_count)) {
        view_model->layer_count = 0;
        return;
    }

    view_model->layer_count = doc_layer_count;
    for (i = 0; i < view_model->layer_count; ++i) {
        const DocumentLayer* layer = document_layer_at(context->document, i);
        EditorLayerView* layer_view = &view_model->layers[i];

        if (!layer) {
            continue;
        }

        layer_view->id = layer->id;
        editor_copy_string(layer_view->name, sizeof(layer_view->name), layer->name);
        layer_view->visible = layer->visible;
        layer_view->locked = layer->locked;
        layer_view->active =
            (document_active_layer_id(context->document) == layer->id);
        layer_view->object_count = 0;

        for (j = 0; j < context->document->count; ++j) {
            const GraphicObject* object =
                document_get_object_at(context->document, j);
            if (object && object->layer_id == layer->id) {
                layer_view->object_count++;
            }
        }
    }
}

static void editor_viewmodel_build_summary(
    EditorViewModel* view_model,
    const EditorViewModelBuildContext* context,
    const EditorSelectionSnapshot* selection_snapshot)
{
    if (!view_model || !context || !context->workspace) {
        return;
    }

    view_model->summary.object_count =
        context->document ? context->document->count : 0;
    view_model->summary.selection_count =
        context->selection ? context->selection->count : 0;
    view_model->summary.undo_count =
        context->commands ? (int)context->commands->undo_count : 0;
    view_model->summary.redo_count =
        context->commands ? (int)context->commands->redo_count : 0;
    view_model->summary.zoom_percent =
        context->canvas ? canvas_view_zoom(context->canvas) * 100.0f : 100.0f;
    view_model->summary.document_dirty =
        workspace_document_dirty(context->workspace);
    editor_copy_string(view_model->summary.current_document_path,
                       sizeof(view_model->summary.current_document_path),
                       context->document_path && context->document_path[0] != '\0'
                           ? context->document_path
                           : "(default)");
    editor_copy_string(view_model->summary.status_message,
                       sizeof(view_model->summary.status_message),
                       context->status_message && context->status_message[0] != '\0'
                           ? context->status_message
                           : "Ready");
    editor_copy_string(view_model->summary.active_tool_label,
                       sizeof(view_model->summary.active_tool_label),
                       context->tools ? tool_controller_active_label(context->tools) : "");
    editor_copy_string(view_model->summary.selection_type_name,
                       sizeof(view_model->summary.selection_type_name),
                       selection_snapshot && selection_snapshot->type_name
                           ? selection_snapshot->type_name
                           : "");
}

static void editor_viewmodel_build_dialog(
    EditorViewModel* view_model,
    const EditorViewModelBuildContext* context)
{
    if (!view_model || !context) {
        return;
    }

    view_model->active_dialog =
        context->active_dialog ? *context->active_dialog : (UiDialogState){0};
}

int editor_viewmodel_build(EditorViewModel* view_model, const Workspace* workspace)
{
    EditorViewModelBuildContext context;
    EditorSelectionSnapshot selection_snapshot;

    if (!view_model || !workspace) {
        return 0;
    }
    if (!editor_viewmodel_capture_context(&context, workspace)) {
        return 0;
    }
    selection_snapshot = editor_viewmodel_capture_selection(&context);

    editor_viewmodel_clear_for_build(view_model);

    editor_viewmodel_build_summary(view_model, &context, &selection_snapshot);
    editor_viewmodel_build_commands(view_model, &context);
    editor_viewmodel_build_tools(view_model, &context);
    editor_viewmodel_build_properties(view_model, &context, &selection_snapshot);
    editor_viewmodel_build_layers(view_model, &context);
    editor_viewmodel_build_dialog(view_model, &context);
    return 1;
}

int editor_viewmodel_command_available(const EditorViewModel* view_model,
                                       EditorCommand command)
{
    int i = 0;

    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        if (!view_model) {
            return 0;
        }
        for (i = 0; i < view_model->tool_count; ++i) {
            if (view_model->tools[i].command == command) {
                return view_model->tools[i].available;
            }
        }
        return 0;
    }

    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return 0;
    }

    return view_model->commands[command].available;
}

const char* editor_viewmodel_command_shortcut(const EditorViewModel* view_model,
                                              EditorCommand command)
{
    int i = 0;

    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        if (!view_model) {
            return "";
        }
        for (i = 0; i < view_model->tool_count; ++i) {
            if (view_model->tools[i].command == command) {
                return view_model->tools[i].shortcut;
            }
        }
        return "";
    }

    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return "";
    }

    return view_model->commands[command].shortcut;
}

const char* editor_viewmodel_command_unavailable_reason(const EditorViewModel* view_model,
                                                        EditorCommand command)
{
    int i = 0;

    if (command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE) {
        if (!view_model) {
            return "";
        }
        for (i = 0; i < view_model->tool_count; ++i) {
            if (view_model->tools[i].command == command) {
                return view_model->tools[i].available ? "" : view_model->tools[i].unavailable_reason;
            }
        }
        return "";
    }

    if (!view_model || command <= EDITOR_COMMAND_NONE ||
        command >= (EditorCommand)EDITOR_VIEWMODEL_MAX_COMMANDS) {
        return "";
    }

    return view_model->commands[command].unavailable_reason;
}
