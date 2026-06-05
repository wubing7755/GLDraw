#include <app/command_availability.h>
#include <app/command_catalog.h>
#include <app/command_dispatcher.h>
#include <app/command_registry.h>
#include <app/editor_controller.h>
#include <app/extension_loader.h>
#include <app/workspace_internal.h>
#include <app/workspace_dialogs.h>
#include <app/workspace_service.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <input/keymap.h>
#include <ui/editor_action.h>
#include <ui/editor_viewmodel.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                     \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #expr);                              \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define EXPECT_STR_EQ(actual, expected)                                       \
    do {                                                                      \
        if (strcmp((actual), (expected)) != 0) {                              \
            fprintf(stderr, "EXPECT_STR_EQ failed: %s:%d: \"%s\" != \"%s\"\n", \
                    __FILE__, __LINE__, (actual), (expected));                \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_SUBSTR(actual, expected)                                       \
    do {                                                                      \
        if (strstr((actual), (expected)) == NULL) {                           \
            fprintf(stderr, "EXPECT_SUBSTR failed: %s:%d: \"%s\" missing \"%s\"\n", \
                    __FILE__, __LINE__, (actual), (expected));                \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_INT_EQ(actual, expected)                                       \
    do {                                                                      \
        int actual_value = (actual);                                          \
        int expected_value = (expected);                                      \
        if (actual_value != expected_value) {                                 \
            fprintf(stderr, "EXPECT_INT_EQ failed: %s:%d: %d != %d\n",        \
                    __FILE__, __LINE__, actual_value, expected_value);        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_FLOAT_EQ(actual, expected)                                     \
    do {                                                                      \
        float actual_value = (actual);                                        \
        float expected_value = (expected);                                    \
        if (fabsf(actual_value - expected_value) > 1e-4f) {                   \
            fprintf(stderr, "EXPECT_FLOAT_EQ failed: %s:%d: %.4f != %.4f\n",  \
                    __FILE__, __LINE__, actual_value, expected_value);        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static GraphicObject* make_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
}

static EditorCommand tool_command(const char* command_id)
{
    const CommandDescriptor* descriptor = command_catalog_find_by_id(command_id);
    return descriptor ? descriptor->command : EDITOR_COMMAND_NONE;
}

static int object_scalar(Document* document, ObjectId id, const char* key, float* out_value)
{
    GraphicObject* object = document_find_object(document, id);
    return object && object_get_scalar(object, key, out_value);
}

static int init_workspace(Workspace* workspace)
{
    memset(workspace, 0, sizeof(*workspace));
    document_init(&workspace->core.document);
    if (!command_executor_init(&workspace->core.commands)) {
        return 0;
    }
    canvas_view_init(&workspace->core.canvas,
                     &workspace->core.document,
                     (RectF){0.0f, 0.0f, 800.0f, 600.0f});
    tool_controller_init(&workspace->core.tools);
    keymap_init(&workspace->session.keymap, "gldraw.test.keymap.json");
    return 1;
}

static void shutdown_workspace(Workspace* workspace)
{
    keymap_shutdown(&workspace->session.keymap);
    tool_controller_shutdown(&workspace->core.tools);
    command_executor_shutdown(&workspace->core.commands);
    workspace_clear_clipboard(workspace);
    selection_set_shutdown(&workspace->session.selection);
    document_shutdown(&workspace->core.document);
}

typedef struct TestWorkspaceCallbacks {
    int save_count;
    int save_as_count;
    int export_count;
    int action_count;
    WorkspaceActionType last_action;
} TestWorkspaceCallbacks;

static int test_workspace_save_callback(Workspace* workspace, void* user_data)
{
    TestWorkspaceCallbacks* callbacks = (TestWorkspaceCallbacks*)user_data;

    (void)workspace;
    if (!callbacks) {
        return 0;
    }
    callbacks->save_count += 1;
    return 1;
}

static int test_workspace_save_as_callback(Workspace* workspace, void* user_data)
{
    TestWorkspaceCallbacks* callbacks = (TestWorkspaceCallbacks*)user_data;

    (void)workspace;
    if (!callbacks) {
        return 0;
    }
    callbacks->save_as_count += 1;
    return 1;
}

static int test_workspace_export_callback(Workspace* workspace, void* user_data)
{
    TestWorkspaceCallbacks* callbacks = (TestWorkspaceCallbacks*)user_data;

    (void)workspace;
    if (!callbacks) {
        return 0;
    }
    callbacks->export_count += 1;
    return 1;
}

static int test_workspace_action_callback(Workspace* workspace,
                                          WorkspaceActionType action,
                                          void* user_data)
{
    TestWorkspaceCallbacks* callbacks = (TestWorkspaceCallbacks*)user_data;

    (void)workspace;
    if (!callbacks) {
        return 0;
    }
    callbacks->action_count += 1;
    callbacks->last_action = action;
    return 1;
}

static int test_editor_viewmodel_builds_selection_properties(void)
{
    Workspace workspace;
    EditorViewModel view_model = {0};
    LayerId overlay_layer = 0u;
    LayerId detail_layer = 0u;
    EditorToolView* first_tools = NULL;
    EditorLayerView* first_layers = NULL;
    int first_tool_capacity = 0;
    int first_layer_capacity = 0;

    EXPECT_TRUE(init_workspace(&workspace));
    overlay_layer = document_create_layer(&workspace.core.document, "Overlay");
    EXPECT_TRUE(overlay_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, overlay_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(10.0f, 20.0f, 40.0f, 50.0f)));
    detail_layer = document_create_layer(&workspace.core.document, "Details");
    EXPECT_TRUE(detail_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, detail_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(60.0f, 20.0f, 20.0f, 20.0f)));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(90.0f, 20.0f, 20.0f, 20.0f)));
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, overlay_layer));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, 1u));
    EXPECT_TRUE(editor_viewmodel_build(&view_model, &workspace));

    EXPECT_INT_EQ(view_model.summary.object_count, 3);
    EXPECT_INT_EQ(view_model.summary.selection_count, 1);
    EXPECT_TRUE(view_model.has_selection);
    EXPECT_STR_EQ(view_model.summary.selection_type_name, "Rectangle");
    EXPECT_TRUE(view_model.property_count >= 4);
    EXPECT_STR_EQ(view_model.tools[0].id, "select");
    EXPECT_TRUE(view_model.tools[0].active);
    EXPECT_INT_EQ(view_model.layer_count, 3);
    EXPECT_INT_EQ(view_model.layers[0].object_count, 0);
    EXPECT_TRUE(view_model.layers[1].active);
    EXPECT_INT_EQ(view_model.layers[1].object_count, 1);
    EXPECT_INT_EQ(view_model.layers[2].object_count, 2);
    EXPECT_TRUE(view_model.properties[0].editable);
    EXPECT_TRUE(view_model.tools[2].available);
    EXPECT_TRUE(view_model.tools[3].available);
    EXPECT_TRUE(view_model.tools[4].available);
    first_tools = view_model.tools;
    first_layers = view_model.layers;
    first_tool_capacity = view_model.tool_capacity;
    first_layer_capacity = view_model.layer_capacity;
    EXPECT_TRUE(first_tools != NULL);
    EXPECT_TRUE(first_layers != NULL);
    EXPECT_TRUE(first_tool_capacity >= view_model.tool_count);
    EXPECT_TRUE(first_layer_capacity >= view_model.layer_count);
    EXPECT_TRUE(editor_viewmodel_command_available(&view_model,
                                                   EDITOR_COMMAND_EDIT_UNDO) == 0);
    EXPECT_STR_EQ(editor_viewmodel_command_unavailable_reason(&view_model,
                                                              EDITOR_COMMAND_EDIT_UNDO),
                  "Nothing to undo.");

    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, overlay_layer, 1));
    EXPECT_TRUE(editor_viewmodel_build(&view_model, &workspace));
    EXPECT_TRUE(view_model.tools == first_tools);
    EXPECT_TRUE(view_model.layers == first_layers);
    EXPECT_INT_EQ(view_model.tool_capacity, first_tool_capacity);
    EXPECT_INT_EQ(view_model.layer_capacity, first_layer_capacity);
    EXPECT_TRUE(view_model.property_count >= 1);
    EXPECT_TRUE(view_model.properties[0].editable == 0);
    EXPECT_TRUE(view_model.tools[0].available);
    EXPECT_TRUE(view_model.tools[1].available);
    EXPECT_TRUE(view_model.tools[2].available == 0);
    EXPECT_TRUE(view_model.tools[3].available == 0);
    EXPECT_TRUE(view_model.tools[4].available == 0);
    EXPECT_STR_EQ(editor_viewmodel_command_unavailable_reason(&view_model,
                                                              tool_command("tool.rect")),
                  "Active layer is locked.");
    EXPECT_SUBSTR(view_model.tools[2].tooltip, "Active layer is locked.");
    EXPECT_SUBSTR(view_model.tools[3].tooltip, "Active layer is locked.");
    EXPECT_SUBSTR(view_model.tools[4].tooltip, "Active layer is locked.");

    editor_viewmodel_shutdown(&view_model);
    shutdown_workspace(&workspace);
    return 0;
}

static int test_command_dispatcher_routes_actions(void)
{
    Workspace workspace;
    CommandDispatcher dispatcher;
    EditorAction action;
    GraphicObject* object = NULL;
    float width = 0.0f;
    LayerId created_layer = 0u;
    LayerId moved_layer = 0u;

    EXPECT_TRUE(init_workspace(&workspace));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, 1u));
    command_dispatcher_init(&dispatcher, &workspace);

    action = editor_action_make_set_tool("rect");
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_STR_EQ(tool_controller_active_id(&workspace.core.tools), "rect");

    action = editor_action_make_modify_property(1u, "width", 25.0f);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    object = document_find_object(&workspace.core.document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "width", &width));
    EXPECT_FLOAT_EQ(width, 25.0f);

    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(object_get_scalar(object, "width", &width));
    EXPECT_FLOAT_EQ(width, 10.0f);

    action = editor_action_make_set_status_message("Dispatcher updated status");
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_STR_EQ(workspace.session.status_message, "Dispatcher updated status");

    action = editor_action_make_create_layer("Overlay");
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_INT_EQ(document_layer_count(&workspace.core.document), 2);
    created_layer = document_active_layer_id(&workspace.core.document);
    EXPECT_TRUE(created_layer != 1u);

    action = editor_action_make_set_layer_visibility(created_layer, 0);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_layer_find(&workspace.core.document, created_layer)->visible == 0);
    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_layer_find(&workspace.core.document, created_layer)->visible == 1);

    action = editor_action_make_set_active_layer(1u);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_active_layer_id(&workspace.core.document) == 1u);
    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_active_layer_id(&workspace.core.document) == created_layer);

    action = editor_action_make_set_layer_locked(created_layer, 1);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_layer_find(&workspace.core.document, created_layer)->locked == 1);
    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_TRUE(document_layer_find(&workspace.core.document, created_layer)->locked == 0);

    action = editor_action_make_rename_layer(created_layer, "Overlay Renamed");
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_STR_EQ(document_layer_find(&workspace.core.document, created_layer)->name,
                  "Overlay Renamed");
    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_STR_EQ(document_layer_find(&workspace.core.document, created_layer)->name,
                  "Overlay");

    moved_layer = document_create_layer(&workspace.core.document, "Foreground");
    EXPECT_TRUE(moved_layer != 0u);
    action = editor_action_make_move_layer(moved_layer, 1);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_INT_EQ(document_layer_index(&workspace.core.document, moved_layer), 1);
    action = editor_action_make_execute_command(EDITOR_COMMAND_EDIT_UNDO);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_INT_EQ(document_layer_index(&workspace.core.document, moved_layer), 2);

    action = editor_action_make_set_layer_locked(1u, 1);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action));
    EXPECT_INT_EQ(workspace.session.selection.count, 0);
    action = editor_action_make_modify_property(1u, "width", 40.0f);
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action) == 0);
    EXPECT_STR_EQ(workspace.session.status_message, "Target object is on a locked layer.");
    EXPECT_TRUE(object_get_scalar(object, "width", &width));
    EXPECT_FLOAT_EQ(width, 10.0f);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_command_dispatcher_updates_save_as_dialog_input(void)
{
    Workspace workspace;
    CommandDispatcher dispatcher;
    EditorAction action;

    EXPECT_TRUE(init_workspace(&workspace));
    command_dispatcher_init(&dispatcher, &workspace);
    EXPECT_TRUE(workspace_dialog_open_save_as(&workspace, "initial.json", "."));
    EXPECT_STR_EQ(workspace_dialog_input_text(&workspace), "initial.json");

    action = editor_action_make_resolve_dialog(UI_DIALOG_RESULT_PRIMARY,
                                               "renamed-document");
    EXPECT_TRUE(command_dispatcher_dispatch(&dispatcher, &action) == 0);
    EXPECT_STR_EQ(workspace_dialog_input_text(&workspace), "renamed-document");

    workspace_dialog_close(&workspace);
    shutdown_workspace(&workspace);
    return 0;
}

static int test_tool_controller_pointer_anchor_accessors(void)
{
    Workspace workspace;
    Vec2 screen_anchor;
    Vec2 world_anchor;

    EXPECT_TRUE(init_workspace(&workspace));
    EXPECT_TRUE(tool_controller_is_pointer_captured(&workspace.core.tools) == 0);

    tool_controller_set_pointer_anchor(&workspace.core.tools,
                                       vec2_make(120.0f, 240.0f),
                                       vec2_make(-8.0f, 16.0f));
    screen_anchor = tool_controller_last_screen(&workspace.core.tools);
    world_anchor = tool_controller_last_world(&workspace.core.tools);

    EXPECT_FLOAT_EQ(screen_anchor.x, 120.0f);
    EXPECT_FLOAT_EQ(screen_anchor.y, 240.0f);
    EXPECT_FLOAT_EQ(world_anchor.x, -8.0f);
    EXPECT_FLOAT_EQ(world_anchor.y, 16.0f);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_workspace_tool_context_and_preview_accessors(void)
{
    Workspace workspace;
    ToolContext context;
    Vec2 preview_delta;

    EXPECT_TRUE(init_workspace(&workspace));
    workspace.session.selection_preview_active = 1;
    workspace.session.selection_preview_delta = vec2_make(7.0f, -3.0f);

    context = workspace_tool_context(&workspace);
    preview_delta = workspace_selection_preview_delta(&workspace);

    EXPECT_TRUE(context.document == &workspace.core.document);
    EXPECT_TRUE(context.canvas == &workspace.core.canvas);
    EXPECT_TRUE(context.selection == &workspace.session.selection);
    EXPECT_TRUE(context.ports.execute_command != NULL);
    EXPECT_TRUE(context.ports.set_selection_preview != NULL);
    EXPECT_TRUE(context.ports.sync_document_dirty != NULL);
    EXPECT_TRUE(workspace_selection_preview_active(&workspace) == 1);
    EXPECT_FLOAT_EQ(preview_delta.x, 7.0f);
    EXPECT_FLOAT_EQ(preview_delta.y, -3.0f);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_file_commands_route_to_workspace_services(void)
{
    Workspace workspace;
    ToolContext context;
    TestWorkspaceCallbacks callbacks = {0};

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);
    workspace_set_service_callbacks(&workspace,
                                    test_workspace_save_callback,
                                    test_workspace_save_as_callback,
                                    test_workspace_export_callback,
                                    NULL,
                                    test_workspace_action_callback,
                                    &callbacks);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_NEW));
    EXPECT_INT_EQ(callbacks.last_action, WORKSPACE_ACTION_NEW_DOCUMENT);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_OPEN));
    EXPECT_INT_EQ(callbacks.last_action, WORKSPACE_ACTION_OPEN_DOCUMENT);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_EXIT));
    EXPECT_INT_EQ(callbacks.last_action, WORKSPACE_ACTION_EXIT_APPLICATION);
    EXPECT_INT_EQ(callbacks.action_count, 3);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_SAVE));
    EXPECT_INT_EQ(callbacks.save_count, 1);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_EXPORT_PNG));
    EXPECT_INT_EQ(callbacks.export_count, 1);

    workspace_service_set_document_path(&workspace, "C:/tmp/example.gldraw.json");
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_SAVE_AS));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_SAVE_AS);
    EXPECT_STR_EQ(workspace_dialog_input_text(&workspace), "example.gldraw.json");
    EXPECT_SUBSTR(workspace.session.active_dialog.message, "C:/tmp");

    workspace_dialog_close(&workspace);
    shutdown_workspace(&workspace);
    return 0;
}

static int test_dialog_commands_route_to_workspace_dialogs(void)
{
    Workspace workspace;
    ToolContext context;
    TestWorkspaceCallbacks callbacks = {0};

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);
    workspace_set_service_callbacks(&workspace,
                                    test_workspace_save_callback,
                                    test_workspace_save_as_callback,
                                    test_workspace_export_callback,
                                    NULL,
                                    test_workspace_action_callback,
                                    &callbacks);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_HELP_SHORTCUTS));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_SHORTCUTS);
    EXPECT_SUBSTR(workspace.session.active_dialog.message, "Tools");
    EXPECT_SUBSTR(workspace.session.active_dialog.message, "Toggle This Dialog");
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_HELP_SHORTCUTS));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_NONE);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_HELP_ABOUT));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_INFO);
    EXPECT_STR_EQ(workspace.session.active_dialog.title, "About GLDraw");
    EXPECT_SUBSTR(workspace.session.active_dialog.message, "Canvas-oriented");
    workspace_dialog_close(&workspace);

    workspace.session.document_dirty = 1;
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_OPEN));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_CONFIRM_UNSAVED);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_MODAL_CANCEL));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_NONE);
    EXPECT_INT_EQ(callbacks.save_count, 0);
    EXPECT_INT_EQ(callbacks.action_count, 0);

    workspace.session.document_dirty = 1;
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_FILE_NEW));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_CONFIRM_UNSAVED);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_MODAL_CONFIRM));
    EXPECT_INT_EQ(workspace.session.active_dialog.kind, UI_DIALOG_NONE);
    EXPECT_INT_EQ(callbacks.save_count, 1);
    EXPECT_INT_EQ(callbacks.action_count, 1);
    EXPECT_INT_EQ(callbacks.last_action, WORKSPACE_ACTION_NEW_DOCUMENT);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_editor_controller_builds_tool_events_and_render_scene(void)
{
    Workspace workspace;
    EditorRenderScene scene;
    ToolEvent event;

    EXPECT_TRUE(init_workspace(&workspace));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, 1u));

    editor_controller_sync_pointer_anchor(&workspace, vec2_make(400.0f, 300.0f));
    event = editor_controller_make_tool_event(&workspace,
                                              vec2_make(410.0f, 290.0f),
                                              -1,
                                              0,
                                              0.0f);
    EXPECT_FLOAT_EQ(event.delta_screen.x, 10.0f);
    EXPECT_FLOAT_EQ(event.delta_screen.y, -10.0f);
    EXPECT_FLOAT_EQ(event.world_pos.x, 10.0f);
    EXPECT_FLOAT_EQ(event.world_pos.y, 10.0f);
    EXPECT_FLOAT_EQ(event.delta_world.x, 10.0f);
    EXPECT_FLOAT_EQ(event.delta_world.y, 10.0f);

    EXPECT_TRUE(editor_controller_render_scene(&workspace, &scene));
    EXPECT_TRUE(scene.document == &workspace.core.document);
    EXPECT_TRUE(scene.selection == &workspace.session.selection);
    EXPECT_TRUE(scene.canvas == &workspace.core.canvas);
    EXPECT_INT_EQ(scene.selection->count, 1);
    EXPECT_TRUE(scene.overlay_object == NULL);

    editor_controller_set_canvas_background(&workspace,
                                            (Color){0.25f, 0.50f, 0.75f, 1.0f});
    EXPECT_FLOAT_EQ(workspace.core.canvas.background.r, 0.25f);
    EXPECT_FLOAT_EQ(workspace.core.canvas.background.g, 0.50f);
    EXPECT_FLOAT_EQ(workspace.core.canvas.background.b, 0.75f);

    editor_controller_set_canvas_viewport(&workspace,
                                          (RectF){5.0f, 6.0f, 320.0f, 240.0f});
    EXPECT_FLOAT_EQ(canvas_view_viewport(&workspace.core.canvas).x, 5.0f);
    EXPECT_FLOAT_EQ(canvas_view_viewport(&workspace.core.canvas).y, 6.0f);
    EXPECT_FLOAT_EQ(canvas_view_viewport(&workspace.core.canvas).w, 320.0f);
    EXPECT_FLOAT_EQ(canvas_view_viewport(&workspace.core.canvas).h, 240.0f);
    EXPECT_TRUE(editor_controller_canvas(&workspace) == &workspace.core.canvas);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_locked_layers_block_pick_and_shape_creation(void)
{
    Workspace workspace;
    ToolContext context;
    ToolEvent down_event;
    ToolEvent up_event;
    GraphicObject* picked = NULL;
    LayerId locked_layer = 0u;

    EXPECT_TRUE(init_workspace(&workspace));

    locked_layer = document_create_layer(&workspace.core.document, "Locked");
    EXPECT_TRUE(locked_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(-10.0f, -10.0f, 20.0f, 20.0f)));
    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, locked_layer, 1));

    picked = canvas_view_pick_object(&workspace.core.canvas,
                                     vec2_make(400.0f, 300.0f),
                                     8.0f);
    EXPECT_TRUE(picked == NULL);

    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(tool_controller_set_active(&workspace.core.tools, NULL, "rect"));

    context = workspace_tool_context(&workspace);

    memset(&down_event, 0, sizeof(down_event));
    down_event.button = 0; /* GLFW_MOUSE_BUTTON_LEFT */
    down_event.screen_pos = vec2_make(400.0f, 300.0f);
    down_event.world_pos = vec2_make(0.0f, 0.0f);

    up_event = down_event;
    up_event.world_pos = vec2_make(50.0f, 50.0f);

    tool_controller_pointer_down(&workspace.core.tools, &context, &down_event);
    tool_controller_pointer_up(&workspace.core.tools, &context, &up_event);
    EXPECT_INT_EQ(workspace.core.document.count, 1);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_dynamic_tool_command_activates_tool(void)
{
    Workspace workspace;
    ToolContext context;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, tool_command("tool.rect")));
    EXPECT_STR_EQ(tool_controller_active_id(&workspace.core.tools), "rect");

    shutdown_workspace(&workspace);
    return 0;
}

static int test_command_registry_respects_locked_layers(void)
{
    Workspace workspace;
    ToolContext context;
    LayerId locked_layer = 0u;

    EXPECT_TRUE(init_workspace(&workspace));

    locked_layer = document_create_layer(&workspace.core.document, "Locked");
    EXPECT_TRUE(locked_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, locked_layer, 1));

    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, 1u));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(20.0f, 0.0f, 10.0f, 10.0f)));

    context = workspace_tool_context(&workspace);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_SELECT_ALL));
    EXPECT_INT_EQ(workspace.session.selection.count, 1);
    EXPECT_TRUE(selection_set_contains(&workspace.session.selection, 2u));
    EXPECT_TRUE(selection_set_contains(&workspace.session.selection, 1u) == 0);

    workspace.session.clipboard_objects =
        (GraphicObject**)calloc(1u, sizeof(workspace.session.clipboard_objects[0]));
    EXPECT_TRUE(workspace.session.clipboard_objects != NULL);
    workspace.session.clipboard_capacity = 1;
    workspace.session.clipboard_objects[0] =
        object_clone(document_find_object(&workspace.core.document, 2u));
    EXPECT_TRUE(workspace.session.clipboard_objects[0] != NULL);
    workspace.session.clipboard_count = 1;
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(command_availability_is_available(&workspace, tool_command("tool.rect")) == 0);
    EXPECT_STR_EQ(command_availability_unavailable_reason(&workspace, tool_command("tool.rect")),
                  "Active layer is locked.");
    EXPECT_TRUE(command_registry_execute(&workspace, &context, tool_command("tool.rect")) == 0);
    EXPECT_TRUE(command_availability_is_available(&workspace, EDITOR_COMMAND_EDIT_PASTE) == 0);
    EXPECT_STR_EQ(command_availability_unavailable_reason(&workspace, EDITOR_COMMAND_EDIT_PASTE),
                  "Active layer is locked.");

    shutdown_workspace(&workspace);
    return 0;
}

static int test_edit_delete_prunes_locked_selection(void)
{
    Workspace workspace;
    ToolContext context;
    LayerId locked_layer = 0u;
    ObjectId locked_id = 1u;
    ObjectId editable_id = 2u;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    locked_layer = document_create_layer(&workspace.core.document, "Locked");
    EXPECT_TRUE(locked_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, locked_layer, 1));

    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, 1u));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(20.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, locked_id));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, editable_id));

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_DELETE));
    EXPECT_TRUE(document_find_object(&workspace.core.document, locked_id) != NULL);
    EXPECT_TRUE(document_find_object(&workspace.core.document, editable_id) == NULL);
    EXPECT_INT_EQ(workspace.session.selection.count, 0);
    EXPECT_TRUE(command_executor_can_undo(&workspace.core.commands));

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_UNDO));
    EXPECT_TRUE(document_find_object(&workspace.core.document, editable_id) != NULL);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_clipboard_copy_paste_cut_commands(void)
{
    Workspace workspace;
    ToolContext context;
    ObjectId source_id = 1u;
    ObjectId pasted_id = 0u;
    float source_x = 0.0f;
    float pasted_x = 0.0f;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(10.0f, 20.0f, 30.0f, 40.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, source_id));

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_COPY));
    EXPECT_INT_EQ(workspace.session.clipboard_count, 1);
    EXPECT_INT_EQ(workspace.session.clipboard_paste_serial, 0);
    EXPECT_TRUE(workspace.session.clipboard_objects[0] != NULL);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_PASTE));
    EXPECT_INT_EQ(workspace.core.document.count, 2);
    EXPECT_INT_EQ(workspace.session.selection.count, 1);
    pasted_id = workspace.session.selection.ids[0];
    EXPECT_TRUE(pasted_id != source_id);
    EXPECT_INT_EQ(workspace.session.clipboard_paste_serial, 1);
    EXPECT_TRUE(object_scalar(&workspace.core.document, source_id, "x", &source_x));
    EXPECT_TRUE(object_scalar(&workspace.core.document, pasted_id, "x", &pasted_x));
    EXPECT_TRUE(pasted_x > source_x);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_CUT));
    EXPECT_TRUE(document_find_object(&workspace.core.document, pasted_id) == NULL);
    EXPECT_INT_EQ(workspace.core.document.count, 1);
    EXPECT_INT_EQ(workspace.session.clipboard_count, 1);
    EXPECT_INT_EQ(workspace.session.selection.count, 0);
    EXPECT_TRUE(command_executor_can_undo(&workspace.core.commands));

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_UNDO));
    EXPECT_TRUE(document_find_object(&workspace.core.document, pasted_id) != NULL);
    EXPECT_INT_EQ(workspace.core.document.count, 2);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_clipboard_cut_prunes_locked_selection(void)
{
    Workspace workspace;
    ToolContext context;
    LayerId locked_layer = 0u;
    ObjectId locked_id = 1u;
    ObjectId editable_id = 2u;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    locked_layer = document_create_layer(&workspace.core.document, "Locked");
    EXPECT_TRUE(locked_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, locked_layer, 1));

    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, 1u));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(20.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, locked_id));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, editable_id));

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_CUT));
    EXPECT_TRUE(document_find_object(&workspace.core.document, locked_id) != NULL);
    EXPECT_TRUE(document_find_object(&workspace.core.document, editable_id) == NULL);
    EXPECT_INT_EQ(workspace.core.document.count, 1);
    EXPECT_INT_EQ(workspace.session.clipboard_count, 1);

    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_EDIT_UNDO));
    EXPECT_TRUE(document_find_object(&workspace.core.document, editable_id) != NULL);
    EXPECT_INT_EQ(workspace.core.document.count, 2);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_view_commands_update_canvas_state(void)
{
    Workspace workspace;
    ToolContext context;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace.core.canvas), 1.0f);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_ZOOM_IN));
    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace.core.canvas), 1.25f);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_ZOOM_OUT));
    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace.core.canvas), 1.0f);

    EXPECT_TRUE(workspace.core.canvas.show_grid);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_TOGGLE_GRID));
    EXPECT_TRUE(!workspace.core.canvas.show_grid);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_TOGGLE_GRID));
    EXPECT_TRUE(workspace.core.canvas.show_grid);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_TOGGLE_INSPECTOR));

    canvas_view_set_center_zoom(&workspace.core.canvas, vec2_make(42.0f, -17.0f), 3.0f);
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_ZOOM_FIT));
    EXPECT_FLOAT_EQ(canvas_view_center(&workspace.core.canvas).x, 0.0f);
    EXPECT_FLOAT_EQ(canvas_view_center(&workspace.core.canvas).y, 0.0f);
    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace.core.canvas), 1.0f);

    shutdown_workspace(&workspace);
    return 0;
}

static int test_view_zoom_fit_frames_small_document(void)
{
    Workspace workspace;
    ToolContext context;
    Vec2 center;

    EXPECT_TRUE(init_workspace(&workspace));
    context = workspace_tool_context(&workspace);

    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(10.0f, 20.0f, 1.0f, 1.0f)));
    EXPECT_TRUE(command_registry_execute(&workspace, &context, EDITOR_COMMAND_VIEW_ZOOM_FIT));

    center = canvas_view_center(&workspace.core.canvas);
    EXPECT_FLOAT_EQ(center.x, 10.5f);
    EXPECT_FLOAT_EQ(center.y, 20.5f);
    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace.core.canvas), 12.0f);

    shutdown_workspace(&workspace);
    return 0;
}

int main(void)
{
    extension_loader_register_all();

    if (test_editor_viewmodel_builds_selection_properties()) return 1;
    if (test_command_dispatcher_routes_actions()) return 1;
    if (test_file_commands_route_to_workspace_services()) return 1;
    if (test_dialog_commands_route_to_workspace_dialogs()) return 1;
    if (test_command_dispatcher_updates_save_as_dialog_input()) return 1;
    if (test_tool_controller_pointer_anchor_accessors()) return 1;
    if (test_workspace_tool_context_and_preview_accessors()) return 1;
    if (test_editor_controller_builds_tool_events_and_render_scene()) return 1;
    if (test_locked_layers_block_pick_and_shape_creation()) return 1;
    if (test_dynamic_tool_command_activates_tool()) return 1;
    if (test_command_registry_respects_locked_layers()) return 1;
    if (test_edit_delete_prunes_locked_selection()) return 1;
    if (test_clipboard_copy_paste_cut_commands()) return 1;
    if (test_clipboard_cut_prunes_locked_selection()) return 1;
    if (test_view_commands_update_canvas_state()) return 1;
    if (test_view_zoom_fit_frames_small_document()) return 1;

    printf("[PASS] editor viewmodel and dispatcher\n");
    return 0;
}
