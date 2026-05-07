#include <app/workspace_service.h>

#include <base/math2d.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                      \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",                \
                    __FILE__, __LINE__, #expr);                               \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

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

#define EXPECT_UINT_EQ(actual, expected)                                      \
    do {                                                                      \
        unsigned int actual_value = (actual);                                 \
        unsigned int expected_value = (expected);                             \
        if (actual_value != expected_value) {                                 \
            fprintf(stderr, "EXPECT_UINT_EQ failed: %s:%d: %u != %u\n",       \
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

#define EXPECT_STR_EQ(actual, expected)                                       \
    do {                                                                      \
        if (strcmp((actual), (expected)) != 0) {                              \
            fprintf(stderr, "EXPECT_STR_EQ failed: %s:%d: \"%s\" != \"%s\"\n",\
                    __FILE__, __LINE__, (actual), (expected));                \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static const char* k_temp_document_path = "/tmp/gldraw_workspace_service_test.json";

static GraphicObject* make_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
}

static int seed_clipboard_with_object(Workspace* workspace, const GraphicObject* source)
{
    if (!workspace || !source) {
        return 0;
    }

    workspace_clear_clipboard(workspace);
    workspace->session.clipboard_objects =
        (GraphicObject**)calloc(1u, sizeof(workspace->session.clipboard_objects[0]));
    if (!workspace->session.clipboard_objects) {
        return 0;
    }

    workspace->session.clipboard_objects[0] = object_clone(source);
    if (!workspace->session.clipboard_objects[0]) {
        workspace_clear_clipboard(workspace);
        return 0;
    }

    workspace->session.clipboard_count = 1;
    workspace->session.clipboard_capacity = 1;
    workspace->session.clipboard_paste_serial = 3u;
    return 1;
}

static int make_workspace_dirty(Workspace* workspace)
{
    GraphicObject* object = NULL;

    if (!workspace) {
        return 0;
    }

    if (!document_add_object(&workspace->core.document,
                             make_rect(10.0f, 20.0f, 30.0f, 40.0f))) {
        return 0;
    }
    object = document_find_object(&workspace->core.document, 1u);
    if (!object) {
        return 0;
    }
    if (!selection_set_add(&workspace->session.selection, object->id)) {
        return 0;
    }
    if (!seed_clipboard_with_object(workspace, object)) {
        return 0;
    }
    if (!command_executor_execute(
            &workspace->core.commands,
            command_create_set_property_from_document(&workspace->core.document,
                                                      object->id,
                                                      "width",
                                                      55.0f),
            &workspace->core.document)) {
        return 0;
    }
    if (!tool_controller_set_active(&workspace->core.tools, NULL, "rect")) {
        return 0;
    }

    canvas_view_set_center_zoom(&workspace->core.canvas, vec2_make(22.0f, -14.0f), 2.5f);
    workspace_service_set_document_path(workspace, "dirty.json");
    workspace_set_status_message(workspace, "Dirty workspace");
    workspace->session.saved_revision = 1u;
    workspace->session.document_dirty = 1;
    return 1;
}

static int expect_runtime_state_cleared(const Workspace* workspace)
{
    const char* active_id = NULL;

    if (!workspace) {
        return 0;
    }

    if (workspace->session.selection.count != 0) {
        return 0;
    }
    if (workspace->session.clipboard_count != 0) {
        return 0;
    }
    if (workspace->session.clipboard_capacity != 0) {
        return 0;
    }
    if (workspace->session.clipboard_paste_serial != 0u) {
        return 0;
    }
    if (command_executor_can_undo(&workspace->core.commands)) {
        return 0;
    }
    if (command_executor_can_redo(&workspace->core.commands)) {
        return 0;
    }

    active_id = tool_controller_active_id(&workspace->core.tools);
    if (!active_id || strcmp(active_id, "select") != 0) {
        return 0;
    }
    if (workspace->session.saved_revision != workspace->core.document.revision) {
        return 0;
    }
    if (workspace->session.document_dirty) {
        return 0;
    }

    return 1;
}

static int test_new_document_resets_workspace_state(Workspace* workspace)
{
    EXPECT_TRUE(make_workspace_dirty(workspace));

    EXPECT_TRUE(workspace_service_new_document(workspace));

    EXPECT_INT_EQ(workspace->core.document.count, 0);
    EXPECT_UINT_EQ(workspace->core.document.next_id, 1u);
    EXPECT_UINT_EQ(workspace->core.document.active_layer_id, 1u);
    EXPECT_TRUE(expect_runtime_state_cleared(workspace));
    EXPECT_TRUE(workspace->session.current_document_path[0] == '\0');
    EXPECT_STR_EQ(workspace->session.status_message, "New empty document");
    EXPECT_FLOAT_EQ(canvas_view_center(&workspace->core.canvas).x, 0.0f);
    EXPECT_FLOAT_EQ(canvas_view_center(&workspace->core.canvas).y, 0.0f);
    EXPECT_FLOAT_EQ(canvas_view_zoom(&workspace->core.canvas), 1.0f);
    return 0;
}

static int test_load_document_resets_runtime_state(Workspace* workspace)
{
    EXPECT_TRUE(document_add_object(&workspace->core.document,
                                    make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(workspace_service_save_to_path(workspace, k_temp_document_path));
    EXPECT_TRUE(make_workspace_dirty(workspace));

    EXPECT_TRUE(workspace_service_load_from_path(workspace, k_temp_document_path));

    EXPECT_INT_EQ(workspace->core.document.count, 1);
    EXPECT_TRUE(document_find_object(&workspace->core.document, 1u) != NULL);
    EXPECT_TRUE(document_find_object(&workspace->core.document, 2u) == NULL);
    EXPECT_UINT_EQ(workspace->core.document.next_id, 2u);
    EXPECT_TRUE(expect_runtime_state_cleared(workspace));
    EXPECT_STR_EQ(workspace_service_document_path(workspace), k_temp_document_path);
    EXPECT_STR_EQ(workspace->session.status_message,
                  "Loaded document: /tmp/gldraw_workspace_service_test.json");
    return 0;
}

int main(void)
{
    Workspace workspace;
    int result = 0;

    remove(k_temp_document_path);
    memset(&workspace, 0, sizeof(workspace));

    if (!workspace_init(&workspace,
                        (RectF){0.0f, 0.0f, 800.0f, 600.0f},
                        "gldraw.test.keymap.json")) {
        fprintf(stderr, "workspace_init failed\n");
        return 1;
    }

    if (test_new_document_resets_workspace_state(&workspace)) {
        result = 1;
        goto cleanup;
    }
    if (test_load_document_resets_runtime_state(&workspace)) {
        result = 1;
        goto cleanup;
    }

    printf("[PASS] workspace service\n");

cleanup:
    workspace_shutdown(&workspace);
    remove(k_temp_document_path);
    return result;
}
