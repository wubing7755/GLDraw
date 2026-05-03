#include <app/command_dispatcher.h>
#include <app/extension_loader.h>
#include <app/workspace.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <input/keymap.h>
#include <ui/editor_action.h>
#include <ui/editor_viewmodel.h>

#include <math.h>
#include <stdio.h>
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
    const CommandDescriptor* descriptor = command_registry_find_by_id(command_id);
    return descriptor ? descriptor->command : EDITOR_COMMAND_NONE;
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

static int test_editor_viewmodel_builds_selection_properties(void)
{
    Workspace workspace;
    EditorViewModel view_model;
    LayerId overlay_layer = 0u;

    EXPECT_TRUE(init_workspace(&workspace));
    overlay_layer = document_create_layer(&workspace.core.document, "Overlay");
    EXPECT_TRUE(overlay_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, overlay_layer));
    EXPECT_TRUE(document_add_object(&workspace.core.document,
                                    make_rect(10.0f, 20.0f, 40.0f, 50.0f)));
    EXPECT_TRUE(selection_set_add(&workspace.session.selection, 1u));
    EXPECT_TRUE(editor_viewmodel_build(&view_model, &workspace));

    EXPECT_INT_EQ(view_model.summary.object_count, 1);
    EXPECT_INT_EQ(view_model.summary.selection_count, 1);
    EXPECT_TRUE(view_model.has_selection);
    EXPECT_STR_EQ(view_model.summary.selection_type_name, "Rectangle");
    EXPECT_TRUE(view_model.property_count >= 4);
    EXPECT_STR_EQ(view_model.tools[0].id, "select");
    EXPECT_TRUE(view_model.tools[0].active);
    EXPECT_INT_EQ(view_model.layer_count, 2);
    EXPECT_TRUE(view_model.layers[1].active);
    EXPECT_INT_EQ(view_model.layers[1].object_count, 1);
    EXPECT_TRUE(view_model.properties[0].editable);
    EXPECT_TRUE(view_model.tools[2].available);
    EXPECT_TRUE(view_model.tools[3].available);
    EXPECT_TRUE(view_model.tools[4].available);
    EXPECT_TRUE(editor_viewmodel_command_available(&view_model,
                                                   EDITOR_COMMAND_EDIT_UNDO) == 0);
    EXPECT_STR_EQ(editor_viewmodel_command_unavailable_reason(&view_model,
                                                              EDITOR_COMMAND_EDIT_UNDO),
                  "Nothing to undo.");

    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, overlay_layer, 1));
    EXPECT_TRUE(editor_viewmodel_build(&view_model, &workspace));
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

    memset(&context, 0, sizeof(context));
    context.workspace = &workspace;
    context.document = &workspace.core.document;
    context.canvas = &workspace.core.canvas;
    context.selection = &workspace.session.selection;

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

    memset(&context, 0, sizeof(context));
    context.workspace = &workspace;
    context.document = &workspace.core.document;
    context.canvas = &workspace.core.canvas;
    context.selection = &workspace.session.selection;

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
    EXPECT_TRUE(command_registry_is_available(&workspace, tool_command("tool.rect")) == 0);
    EXPECT_STR_EQ(command_registry_unavailable_reason(&workspace, tool_command("tool.rect")),
                  "Active layer is locked.");
    EXPECT_TRUE(command_registry_execute(&workspace, &context, tool_command("tool.rect")) == 0);
    EXPECT_TRUE(command_registry_is_available(&workspace, EDITOR_COMMAND_EDIT_PASTE) == 0);
    EXPECT_STR_EQ(command_registry_unavailable_reason(&workspace, EDITOR_COMMAND_EDIT_PASTE),
                  "Active layer is locked.");

    shutdown_workspace(&workspace);
    return 0;
}

int main(void)
{
    extension_loader_register_all();

    if (test_editor_viewmodel_builds_selection_properties()) return 1;
    if (test_command_dispatcher_routes_actions()) return 1;
    if (test_locked_layers_block_pick_and_shape_creation()) return 1;
    if (test_command_registry_respects_locked_layers()) return 1;

    printf("[PASS] editor viewmodel and dispatcher\n");
    return 0;
}
