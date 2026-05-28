#include <app/editor_session.h>
#include <app/extension_loader.h>
#include <base/path_utils.h>
#include <commands/command.h>
#include <document/document.h>
#include <document/object.h>
#include <document/persistence.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*TestFn)(void);

static int g_failures = 0;

static int nearly_equal(float a, float b)
{
    return fabsf(a - b) <= 1e-5f;
}

static void expect_true_impl(int condition, const char* expr, const char* file, int line)
{
    if (!condition) {
        fprintf(stderr, "%s:%d: expected true: %s\n", file, line, expr);
        g_failures++;
    }
}

static void expect_uint_eq_impl(unsigned int actual,
                                unsigned int expected,
                                const char* actual_expr,
                                const char* expected_expr,
                                const char* file,
                                int line)
{
    if (actual != expected) {
        fprintf(stderr,
                "%s:%d: expected %s == %s (%u != %u)\n",
                file,
                line,
                actual_expr,
                expected_expr,
                actual,
                expected);
        g_failures++;
    }
}

static void expect_int_eq_impl(int actual,
                               int expected,
                               const char* actual_expr,
                               const char* expected_expr,
                               const char* file,
                               int line)
{
    if (actual != expected) {
        fprintf(stderr,
                "%s:%d: expected %s == %s (%d != %d)\n",
                file,
                line,
                actual_expr,
                expected_expr,
                actual,
                expected);
        g_failures++;
    }
}

static void expect_float_eq_impl(float actual,
                                 float expected,
                                 const char* actual_expr,
                                 const char* expected_expr,
                                 const char* file,
                                 int line)
{
    if (!nearly_equal(actual, expected)) {
        fprintf(stderr,
                "%s:%d: expected %s ~= %s (%.6f != %.6f)\n",
                file,
                line,
                actual_expr,
                expected_expr,
                actual,
                expected);
        g_failures++;
    }
}

static void expect_str_eq_impl(const char* actual,
                               const char* expected,
                               const char* actual_expr,
                               const char* expected_expr,
                               const char* file,
                               int line)
{
    if (!actual || !expected || strcmp(actual, expected) != 0) {
        fprintf(stderr,
                "%s:%d: expected %s == %s (\"%s\" != \"%s\")\n",
                file,
                line,
                actual_expr,
                expected_expr,
                actual ? actual : "(null)",
                expected ? expected : "(null)");
        g_failures++;
    }
}

#define EXPECT_TRUE(expr) expect_true_impl((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) expect_true_impl(!(expr), "!(" #expr ")", __FILE__, __LINE__)
#define EXPECT_UINT_EQ(actual, expected) \
    expect_uint_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define EXPECT_INT_EQ(actual, expected) \
    expect_int_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define EXPECT_FLOAT_EQ(actual, expected) \
    expect_float_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define EXPECT_STR_EQ(actual, expected) \
    expect_str_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)

static GraphicObject* create_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
}

static GraphicObject* create_line(float x1, float y1, float x2, float y2)
{
    return object_create_line((Vec2){x1, y1}, (Vec2){x2, y2}, object_default_style());
}

static int write_text_file(const char* path, const char* text)
{
    FILE* file = fopen(path, "wb");
    size_t length = 0u;

    if (!file || !text) {
        if (file) {
            fclose(file);
        }
        return 0;
    }

    length = strlen(text);
    if (fwrite(text, 1u, length, file) != length) {
        fclose(file);
        return 0;
    }

    return fclose(file) == 0;
}

static int make_temp_path(char* buffer, size_t buffer_size, const char* suffix)
{
    char base_name[L_tmpnam];

    if (!buffer || buffer_size == 0u || !suffix) {
        return 0;
    }

#ifdef _WIN32
    if (tmpnam_s(base_name, sizeof(base_name)) != 0) {
        return 0;
    }
#else
    if (!tmpnam(base_name)) {
        return 0;
    }
#endif

    if (snprintf(buffer, buffer_size, "%s%s", base_name, suffix) >= (int)buffer_size) {
        return 0;
    }

    return 1;
}

static int test_document_selection_and_deletion(void)
{
    Document document;
    SelectionSet selection = {0};
    GraphicObject* first = NULL;
    GraphicObject* second = NULL;
    ObjectId* ids = NULL;
    int i = 0;

    document_init(&document);

    first = create_rect(1.0f, 2.0f, 3.0f, 4.0f);
    second = create_line(0.0f, 0.0f, 5.0f, 6.0f);
    EXPECT_TRUE(document_add_object(&document, first));
    EXPECT_TRUE(document_add_object(&document, second));

    EXPECT_INT_EQ(document.count, 2);
    EXPECT_UINT_EQ(document.objects[0]->id, 1u);
    EXPECT_UINT_EQ(document.objects[1]->id, 2u);
    EXPECT_UINT_EQ(document.next_id, 3u);

    EXPECT_TRUE(selection_set_add(&selection, 1u));
    EXPECT_TRUE(selection_set_add(&selection, 2u));
    EXPECT_TRUE(selection_set_contains(&selection, 1u));
    EXPECT_TRUE(selection_set_contains(&selection, 2u));

    selection_set_toggle(&selection, 2u);
    EXPECT_FALSE(selection_set_contains(&selection, 2u));
    EXPECT_INT_EQ(selection.count, 1);

    EXPECT_TRUE(document_remove_object(&document, 1u));
    EXPECT_INT_EQ(document.count, 1);
    selection_set_remove(&selection, 1u);
    EXPECT_FALSE(selection_set_contains(&selection, 1u));
    EXPECT_UINT_EQ(document.objects[0]->id, 2u);

    EXPECT_TRUE(selection_set_add(&selection, 2u));
    ids = (ObjectId*)calloc((size_t)selection.count, sizeof(ids[0]));
    EXPECT_TRUE(ids != NULL);
    for (i = 0; i < selection.count; ++i) {
        ids[i] = selection.ids[i];
    }
    for (i = 0; i < selection.count; ++i) {
        EXPECT_TRUE(document_remove_object(&document, ids[i]));
    }
    free(ids);
    selection_set_clear(&selection);
    EXPECT_INT_EQ(document.count, 0);
    EXPECT_INT_EQ(selection.count, 0);

    selection_set_shutdown(&selection);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_command_executor_create_undo_redo(void)
{
    Document document;
    CommandExecutor executor;

    document_init(&document);
    EXPECT_TRUE(command_executor_init(&executor));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_object(
                                             create_rect(0.0f, 0.0f, 10.0f, 20.0f)),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_object(
                                             create_rect(10.0f, 10.0f, 5.0f, 5.0f)),
                                         &document));
    EXPECT_TRUE(command_executor_can_undo(&executor));

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(command_executor_can_undo(&executor));
    EXPECT_TRUE(command_executor_can_redo(&executor));

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document.count, 2);
    EXPECT_TRUE(command_executor_can_undo(&executor));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_command_executor_property_undo_redo(void)
{
    Document document;
    CommandExecutor executor;
    GraphicObject* object = NULL;
    float value = 0.0f;

    document_init(&document);
    EXPECT_TRUE(command_executor_init(&executor));
    EXPECT_TRUE(document_add_object(&document, create_rect(0.0f, 0.0f, 10.0f, 20.0f)));

    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document,
                                                                                   object->id,
                                                                                   "width",
                                                                                   42.0f),
                                         &document));

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "width", &value));
    EXPECT_FLOAT_EQ(value, 10.0f);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "width", &value));
    EXPECT_FLOAT_EQ(value, 42.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_command_executor_move_undo_redo(void)
{
    Document document;
    CommandExecutor executor;
    GraphicObject* object = NULL;
    ObjectId ids[1] = {0u};
    Vec2 delta = {3.0f, -4.0f};
    float x = 0.0f;
    float y = 0.0f;

    document_init(&document);
    EXPECT_TRUE(command_executor_init(&executor));
    EXPECT_TRUE(document_add_object(&document, create_rect(2.0f, 8.0f, 10.0f, 20.0f)));

    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    ids[0] = object->id;
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_objects(ids, 1, delta),
                                         &document));

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "x", &x));
    EXPECT_TRUE(object_get_scalar(object, "y", &y));
    EXPECT_FLOAT_EQ(x, 2.0f);
    EXPECT_FLOAT_EQ(y, 8.0f);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    object = document_find_object(&document, 1u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "x", &x));
    EXPECT_TRUE(object_get_scalar(object, "y", &y));
    EXPECT_FLOAT_EQ(x, 5.0f);
    EXPECT_FLOAT_EQ(y, 4.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_persistence_round_trip(void)
{
    Document saved;
    Document loaded;
    char path[L_tmpnam + 16];
    const DocumentLayer* loaded_overlay = NULL;
    const GraphicObject* loaded_rect = NULL;
    const GraphicObject* loaded_line = NULL;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float x2 = 0.0f;
    LayerId overlay_layer = 0u;
    GraphicStyle rect_style;

    document_init(&saved);
    document_init(&loaded);

    overlay_layer = document_create_layer(&saved, "Overlay");
    EXPECT_TRUE(document_rename_layer(&saved, overlay_layer, "Overlay Locked"));
    EXPECT_TRUE(document_set_layer_visibility(&saved, overlay_layer, 0));
    EXPECT_TRUE(document_set_layer_locked(&saved, overlay_layer, 1));
    EXPECT_TRUE(document_set_layer_blend_mode(&saved,
                                              overlay_layer,
                                              DOCUMENT_LAYER_BLEND_MULTIPLY));
    EXPECT_TRUE(document_set_active_layer(&saved, overlay_layer));

    rect_style = object_default_style();
    rect_style.stroke_color.r = 0.25f;
    rect_style.stroke_color.g = 0.50f;
    rect_style.stroke_color.b = 0.75f;
    rect_style.stroke_color.a = 0.90f;
    rect_style.stroke_width = 3.5f;
    EXPECT_TRUE(document_add_object(&saved,
                                    object_create_rect((RectF){3.0f, 4.0f, 20.0f, 30.0f},
                                                       rect_style)));
    EXPECT_TRUE(document_add_object(&saved, create_line(1.0f, 2.0f, 9.0f, 10.0f)));

    EXPECT_TRUE(make_temp_path(path, sizeof(path), ".json"));
    EXPECT_TRUE(document_save_json(&saved, path));
    EXPECT_TRUE(document_load_json(&loaded, path));

    EXPECT_INT_EQ(loaded.count, 2);
    EXPECT_UINT_EQ(loaded.next_id, saved.next_id);
    EXPECT_INT_EQ(document_layer_count(&loaded), 2);
    loaded_overlay = document_layer_find_const(&loaded, overlay_layer);
    EXPECT_TRUE(loaded_overlay != NULL);
    EXPECT_STR_EQ(loaded_overlay->name, "Overlay Locked");
    EXPECT_INT_EQ(loaded_overlay->visible, 0);
    EXPECT_INT_EQ(loaded_overlay->locked, 1);
    EXPECT_INT_EQ(loaded_overlay->blend_mode, DOCUMENT_LAYER_BLEND_MULTIPLY);
    EXPECT_UINT_EQ(document_active_layer_id(&loaded), overlay_layer);

    loaded_rect = document_find_object(&loaded, 1u);
    loaded_line = document_find_object(&loaded, 2u);
    EXPECT_TRUE(loaded_rect != NULL);
    EXPECT_TRUE(loaded_line != NULL);
    EXPECT_TRUE(loaded_rect->layer_id == overlay_layer);
    EXPECT_TRUE(object_get_scalar(loaded_rect, "x", &x));
    EXPECT_TRUE(object_get_scalar(loaded_rect, "y", &y));
    EXPECT_TRUE(object_get_scalar(loaded_rect, "width", &width));
    EXPECT_TRUE(object_get_scalar(loaded_rect, "height", &height));
    EXPECT_FLOAT_EQ(x, 3.0f);
    EXPECT_FLOAT_EQ(y, 4.0f);
    EXPECT_FLOAT_EQ(width, 20.0f);
    EXPECT_FLOAT_EQ(height, 30.0f);
    EXPECT_FLOAT_EQ(loaded_rect->style.stroke_color.r, 0.25f);
    EXPECT_FLOAT_EQ(loaded_rect->style.stroke_color.g, 0.50f);
    EXPECT_FLOAT_EQ(loaded_rect->style.stroke_color.b, 0.75f);
    EXPECT_FLOAT_EQ(loaded_rect->style.stroke_color.a, 0.90f);
    EXPECT_FLOAT_EQ(loaded_rect->style.stroke_width, 3.5f);
    EXPECT_TRUE(object_get_scalar(loaded_line, "x2", &x2));
    EXPECT_FLOAT_EQ(x2, 9.0f);

    remove(path);
    document_shutdown(&loaded);
    document_shutdown(&saved);
    return g_failures == 0;
}

static int test_persistence_ignores_unknown_fields(void)
{
    Document document;
    char path[L_tmpnam + 16];
    const DocumentLayer* layer = NULL;
    GraphicObject* object = NULL;
    float width = 0.0f;

    document_init(&document);
    EXPECT_TRUE(make_temp_path(path, sizeof(path), ".json"));
    EXPECT_TRUE(write_text_file(path,
                                "{\n"
                                "  \"format\": \"gldraw-document\",\n"
                                "  \"version\": 2,\n"
                                "  \"unknown_root\": { \"ignored\": true },\n"
                                "  \"next_id\": 8,\n"
                                "  \"active_layer_id\": 2,\n"
                                "  \"layers\": [\n"
                                "    { \"id\": 1, \"name\": \"Layer 1\", \"visible\": true, \"locked\": false, \"blend_mode\": \"normal\", \"unknown\": [1, 2] },\n"
                                "    { \"id\": 2, \"name\": \"Work\", \"visible\": false, \"locked\": true, \"blend_mode\": \"screen\", \"unknown\": null }\n"
                                "  ],\n"
                                "  \"objects\": [\n"
                                "    {\n"
                                "      \"id\": 7,\n"
                                "      \"layer_id\": 2,\n"
                                "      \"type\": \"rect\",\n"
                                "      \"unknown_object\": { \"nested\": [false] },\n"
                                "      \"stroke\": { \"r\": 1, \"g\": 0.5, \"b\": 0.25, \"a\": 1, \"width\": 2, \"ignored\": 42 },\n"
                                "      \"properties\": { \"x\": 11, \"y\": 12, \"width\": 13, \"height\": 14 }\n"
                                "    }\n"
                                "  ]\n"
                                "}\n"));

    EXPECT_TRUE(document_load_json(&document, path));
    EXPECT_INT_EQ(document_layer_count(&document), 2);
    layer = document_layer_find_const(&document, 2u);
    EXPECT_TRUE(layer != NULL);
    EXPECT_STR_EQ(layer->name, "Work");
    EXPECT_INT_EQ(layer->visible, 0);
    EXPECT_INT_EQ(layer->locked, 1);
    EXPECT_INT_EQ(layer->blend_mode, DOCUMENT_LAYER_BLEND_SCREEN);
    EXPECT_UINT_EQ(document_active_layer_id(&document), 2u);
    object = document_find_object(&document, 7u);
    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(object_get_scalar(object, "width", &width));
    EXPECT_FLOAT_EQ(width, 13.0f);
    EXPECT_UINT_EQ(document.next_id, 8u);

    remove(path);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_persistence_rejects_invalid_json(void)
{
    Document document;
    char path[L_tmpnam + 16];

    document_init(&document);
    EXPECT_TRUE(document_add_object(&document, create_rect(0.0f, 0.0f, 1.0f, 1.0f)));

    EXPECT_TRUE(make_temp_path(path, sizeof(path), ".json"));
    EXPECT_TRUE(write_text_file(path, "{\"format\":\"gldraw-document\",\"version\":99,\"objects\":[]}"));
    EXPECT_FALSE(document_load_json(&document, path));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_UINT_EQ(document.objects[0]->id, 1u);

    EXPECT_TRUE(write_text_file(path, "{\"format\":\"gldraw-document\","));
    EXPECT_FALSE(document_load_json(&document, path));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_UINT_EQ(document.objects[0]->id, 1u);

    remove(path);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_path_utils_common_cases(void)
{
    char buffer[GLDRAW_PATH_MAX];
    char small_buffer[1];

    EXPECT_STR_EQ(path_utils_basename_or_default("C:\\dir\\file.json", "fallback.json"),
                  "file.json");
    EXPECT_STR_EQ(path_utils_basename_or_default("dir/file.json", "fallback.json"),
                  "file.json");
    EXPECT_STR_EQ(path_utils_basename_or_default("", "fallback.json"), "fallback.json");
    EXPECT_STR_EQ(path_utils_basename_or_default("dir/", "fallback.json"), "fallback.json");

    EXPECT_TRUE(path_utils_dirname("C:\\file.json", buffer, sizeof(buffer)));
    EXPECT_STR_EQ(buffer, "C:\\");
    EXPECT_TRUE(path_utils_dirname("dir/file.json", buffer, sizeof(buffer)));
    EXPECT_STR_EQ(buffer, "dir");
    EXPECT_TRUE(path_utils_dirname("file.json", buffer, sizeof(buffer)));
    EXPECT_STR_EQ(buffer, ".");
    EXPECT_FALSE(path_utils_dirname("file.json", small_buffer, sizeof(small_buffer)));

    EXPECT_TRUE(path_utils_has_extension("theme.JSON", ".json"));
    EXPECT_FALSE(path_utils_has_extension("theme.json.bak", ".json"));

    EXPECT_TRUE(path_utils_copy_trimmed("  copy  ", buffer, sizeof(buffer)));
    EXPECT_STR_EQ(buffer, "copy");
    EXPECT_FALSE(path_utils_copy_trimmed("   ", buffer, sizeof(buffer)));

    EXPECT_TRUE(path_utils_is_safe_filename("copy.json"));
    EXPECT_FALSE(path_utils_is_safe_filename("dir/copy.json"));
    EXPECT_FALSE(path_utils_is_safe_filename("dir\\copy.json"));
    EXPECT_FALSE(path_utils_is_safe_filename("bad:name.json"));
    EXPECT_FALSE(path_utils_is_safe_filename("."));
    EXPECT_FALSE(path_utils_is_safe_filename(".."));

    EXPECT_TRUE(path_utils_join_same_directory("dir/base.json",
                                               "copy",
                                               ".json",
                                               buffer,
                                               sizeof(buffer)));
    EXPECT_STR_EQ(buffer, "dir/copy.json");
    EXPECT_TRUE(path_utils_join_same_directory("C:\\dir\\base.json",
                                               "copy.JSON",
                                               ".json",
                                               buffer,
                                               sizeof(buffer)));
    EXPECT_STR_EQ(buffer, "C:\\dir\\copy.JSON");

    return g_failures == 0;
}

static int test_layer_visibility_and_spatial_query(void)
{
    Document document;
    GraphicObject* hidden = NULL;
    GraphicObject* visible = NULL;
    LayerId hidden_layer = 0u;
    int* indices = NULL;
    int count = 0;

    document_init(&document);
    hidden_layer = document_create_layer(&document, "Hidden");
    EXPECT_TRUE(hidden_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&document, hidden_layer));
    hidden = create_rect(0.0f, 0.0f, 20.0f, 20.0f);
    EXPECT_TRUE(document_add_object(&document, hidden));
    EXPECT_TRUE(document_set_layer_visibility(&document, hidden_layer, 0));

    EXPECT_TRUE(document_set_active_layer(&document, 1u));
    visible = create_rect(100.0f, 100.0f, 20.0f, 20.0f);
    EXPECT_TRUE(document_add_object(&document, visible));

    indices = (int*)calloc((size_t)document.count, sizeof(indices[0]));
    EXPECT_TRUE(indices != NULL);
    count = document_query_visible_indices(&document,
                                           (RectF){-10.0f, -10.0f, 200.0f, 200.0f},
                                           indices,
                                           document.count);
    EXPECT_INT_EQ(count, 1);
    EXPECT_UINT_EQ(document.objects[indices[0]]->id, visible->id);

    free(indices);
    document_shutdown(&document);
    return g_failures == 0;
}

static int test_stress_5000_objects_and_undo_redo(void)
{
    Document document;
    CommandExecutor executor;
    int i = 0;

    document_init(&document);
    EXPECT_TRUE(command_executor_init_with_budget(&executor, 32u * 1024u * 1024u));

    for (i = 0; i < 5000; ++i) {
        float x = (float)(i % 100) * 12.0f;
        float y = (float)(i / 100) * 12.0f;
        EXPECT_TRUE(document_add_object(&document, create_rect(x, y, 10.0f, 10.0f)));
    }
    EXPECT_INT_EQ(document.count, 5000);

    for (i = 0; i < 100; ++i) {
        EXPECT_TRUE(command_executor_execute(&executor,
                                             command_create_create_object(
                                                 create_rect(1500.0f + (float)i * 12.0f,
                                                             0.0f,
                                                             10.0f,
                                                             10.0f)),
                                             &document));
    }
    for (i = 0; i < 100; ++i) {
        EXPECT_TRUE(command_executor_undo(&executor, &document));
    }
    for (i = 0; i < 100; ++i) {
        EXPECT_TRUE(command_executor_redo(&executor, &document));
    }

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return g_failures == 0;
}

int main(void)
{
    static const struct {
        const char* name;
        TestFn fn;
    } tests[] = {
        {"document selection and deletion", test_document_selection_and_deletion},
        {"command create undo/redo", test_command_executor_create_undo_redo},
        {"command property undo/redo", test_command_executor_property_undo_redo},
        {"command move undo/redo", test_command_executor_move_undo_redo},
        {"persistence round trip", test_persistence_round_trip},
        {"persistence ignores unknown fields", test_persistence_ignores_unknown_fields},
        {"persistence rejects invalid json", test_persistence_rejects_invalid_json},
        {"path utils common cases", test_path_utils_common_cases},
        {"layer visibility and spatial query", test_layer_visibility_and_spatial_query},
        {"stress 5000 objects and undo/redo", test_stress_5000_objects_and_undo_redo}
    };
    int i = 0;
    int test_count = (int)(sizeof(tests) / sizeof(tests[0]));
    int failures_before = 0;

    extension_loader_register_all();

    for (i = 0; i < test_count; ++i) {
        failures_before = g_failures;
        tests[i].fn();
        if (g_failures == failures_before) {
            printf("[PASS] %s\n", tests[i].name);
        } else {
            printf("[FAIL] %s\n", tests[i].name);
        }
    }

    if (g_failures != 0) {
        fprintf(stderr, "Test failures: %d\n", g_failures);
        return 1;
    }

    printf("All %d tests passed.\n", test_count);
    return 0;
}
