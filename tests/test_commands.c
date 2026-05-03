#include <commands/command.h>

#include <app/extension_loader.h>
#include <base/math2d.h>

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

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_INT_EQ(actual, expected)                                      \
    do {                                                                     \
        int actual_value = (actual);                                         \
        int expected_value = (expected);                                     \
        if (actual_value != expected_value) {                                \
            fprintf(stderr, "EXPECT_INT_EQ failed: %s:%d: %s=%d expected %d\n", \
                    __FILE__, __LINE__, #actual, actual_value, expected_value); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define EXPECT_FLOAT_EQ(actual, expected)                                    \
    do {                                                                     \
        float actual_value = (actual);                                       \
        float expected_value = (expected);                                   \
        if (fabsf(actual_value - expected_value) > 1e-4f) {                  \
            fprintf(stderr, "EXPECT_FLOAT_EQ failed: %s:%d: %s=%.6f expected %.6f\n", \
                    __FILE__, __LINE__, #actual, actual_value, expected_value); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int g_fail_realloc = 0;

static void* test_malloc(size_t size) { return malloc(size); }
static void* test_calloc(size_t count, size_t size) { return calloc(count, size); }
static void* test_realloc(void* ptr, size_t size)
{
    if (g_fail_realloc) {
        (void)ptr;
        (void)size;
        return NULL;
    }
    return realloc(ptr, size);
}
static void test_free(void* ptr) { free(ptr); }

static GraphicObject* make_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
}

static int get_scalar(Document* document, ObjectId id, const char* key, float* out_value)
{
    GraphicObject* object = document_find_object(document, id);
    return object && object_get_scalar(object, key, out_value);
}

static int test_create_object_command(void)
{
    Document document;
    CommandExecutor executor;
    Command* command = NULL;

    document_init(&document);
    command_executor_init(&executor);

    command = command_create_create_object(make_rect(0.0f, 0.0f, 10.0f, 12.0f));
    EXPECT_TRUE(command_executor_execute(&executor, command, &document));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) != NULL);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document.count, 0);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) != NULL);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_delete_selection_command(void)
{
    Document document;
    CommandExecutor executor;
    SelectionSet selection = {0};
    Command* command = NULL;

    document_init(&document);
    command_executor_init(&executor);

    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(20.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(selection_set_add(&selection, 1u));

    command = command_create_delete_selection(&document, &selection);
    EXPECT_TRUE(command_executor_execute(&executor, command, &document));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) == NULL);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document.count, 2);
    EXPECT_TRUE(document_find_object(&document, 1u) != NULL);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) == NULL);

    selection_set_shutdown(&selection);
    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_move_objects_command_and_merge(void)
{
    Document document;
    CommandExecutor executor;
    ObjectId ids[1] = {1u};
    float x = 0.0f;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_objects(ids, 1, vec2_make(1.0f, 0.0f)),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_objects(ids, 1, vec2_make(2.0f, 0.0f)),
                                         &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x));
    EXPECT_FLOAT_EQ(x, 3.0f);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x));
    EXPECT_FLOAT_EQ(x, 0.0f);
    EXPECT_FALSE(command_executor_can_undo(&executor));

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x));
    EXPECT_FLOAT_EQ(x, 3.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_set_property_command_and_merge(void)
{
    Document document;
    CommandExecutor executor;
    float width = 0.0f;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "width", 20.0f),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "width", 30.0f),
                                         &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_FLOAT_EQ(width, 30.0f);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_FLOAT_EQ(width, 10.0f);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_FLOAT_EQ(width, 30.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_executor_allocation_failure_keeps_document_unchanged(void)
{
    Document document;
    CommandExecutor executor;
    CommandAllocator allocator = {test_malloc, test_calloc, test_realloc, test_free};
    Command* command = NULL;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    command = command_create_set_property_from_document(&document, 1u, "width", 20.0f);
    EXPECT_TRUE(command != NULL);

    command_set_allocator(&allocator);
    g_fail_realloc = 1;
    EXPECT_FALSE(command_executor_execute(&executor, command, &document));
    g_fail_realloc = 0;
    command_reset_allocator();

    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) != NULL);
    EXPECT_FALSE(command_executor_can_undo(&executor));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_failed_undo_redo_keeps_history_entry(void)
{
    Document document;
    CommandExecutor executor;
    ObjectId ids[1] = {1u};

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_objects(ids, 1, vec2_make(1.0f, 0.0f)),
                                         &document));
    EXPECT_TRUE(document_remove_object(&document, 1u));

    EXPECT_FALSE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(command_executor_can_undo(&executor));

    command_executor_shutdown(&executor);
    document_reset(&document);

    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "width", 20.0f),
                                         &document));
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(document_remove_object(&document, 1u));

    EXPECT_FALSE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(command_executor_can_redo(&executor));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_transaction_commit_and_undo(void)
{
    Document document;
    CommandExecutor executor;
    float width = 0.0f;
    float height = 0.0f;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_begin_transaction(&executor));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "width", 20.0f),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "height", 30.0f),
                                         &document));
    EXPECT_TRUE(command_executor_commit_transaction(&executor));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_TRUE(get_scalar(&document, 1u, "height", &height));
    EXPECT_FLOAT_EQ(width, 20.0f);
    EXPECT_FLOAT_EQ(height, 30.0f);
    EXPECT_INT_EQ((int)executor.undo_count, 1);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_TRUE(get_scalar(&document, 1u, "height", &height));
    EXPECT_FLOAT_EQ(width, 10.0f);
    EXPECT_FLOAT_EQ(height, 10.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_transaction_rollback_restores_document(void)
{
    Document document;
    CommandExecutor executor;
    float width = 0.0f;
    float height = 0.0f;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_begin_transaction(&executor));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "width", 25.0f),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document, 1u, "height", 35.0f),
                                         &document));
    command_executor_rollback_transaction(&executor, &document);
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_TRUE(get_scalar(&document, 1u, "height", &height));
    EXPECT_FLOAT_EQ(width, 10.0f);
    EXPECT_FLOAT_EQ(height, 10.0f);
    EXPECT_FALSE(command_executor_can_undo(&executor));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_memory_budget_prunes_old_undo_entries(void)
{
    Document document;
    CommandExecutor executor;

    document_init(&document);
    EXPECT_TRUE(command_executor_init_with_budget(&executor, 1u));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_object(make_rect(0.0f, 0.0f, 10.0f, 10.0f)),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_object(make_rect(20.0f, 0.0f, 10.0f, 10.0f)),
                                         &document));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_object(make_rect(40.0f, 0.0f, 10.0f, 10.0f)),
                                         &document));
    EXPECT_INT_EQ(document.count, 3);
    EXPECT_INT_EQ((int)executor.undo_count, 1);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document.count, 2);
    EXPECT_FALSE(command_executor_can_undo(&executor));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_layer_commands_undo_redo(void)
{
    Document document;
    CommandExecutor executor;
    LayerId layer_id = 0u;
    LayerId third_layer_id = 0u;

    document_init(&document);
    command_executor_init(&executor);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_create_layer("Overlay"),
                                         &document));
    EXPECT_INT_EQ(document_layer_count(&document), 2);
    layer_id = document_active_layer_id(&document);
    EXPECT_TRUE(layer_id != 1u);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document_layer_count(&document), 1);
    EXPECT_INT_EQ((int)document_active_layer_id(&document), 1);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document_layer_count(&document), 2);
    layer_id = document_active_layer_id(&document);
    EXPECT_TRUE(layer_id != 1u);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_layer_visibility(&document,
                                                                             layer_id,
                                                                             0),
                                         &document));
    EXPECT_FALSE(document_layer_find(&document, layer_id)->visible);
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(document_layer_find(&document, layer_id)->visible);
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_FALSE(document_layer_find(&document, layer_id)->visible);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_active_layer(&document, 1u),
                                         &document));
    EXPECT_INT_EQ((int)document_active_layer_id(&document), 1);
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ((int)document_active_layer_id(&document), (int)layer_id);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_layer_locked(&document,
                                                                         layer_id,
                                                                         1),
                                         &document));
    EXPECT_TRUE(document_layer_find(&document, layer_id)->locked == 1);
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(document_layer_find(&document, layer_id)->locked == 0);
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(document_layer_find(&document, layer_id)->locked == 1);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_rename_layer(&document,
                                                                     layer_id,
                                                                     "Overlay Renamed"),
                                         &document));
    EXPECT_TRUE(strcmp(document_layer_find(&document, layer_id)->name,
                       "Overlay Renamed") == 0);
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(strcmp(document_layer_find(&document, layer_id)->name,
                       "Overlay") == 0);
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(strcmp(document_layer_find(&document, layer_id)->name,
                       "Overlay Renamed") == 0);

    third_layer_id = document_create_layer(&document, "Foreground");
    EXPECT_TRUE(third_layer_id != 0u);
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_layer(&document,
                                                                   third_layer_id,
                                                                   1),
                                         &document));
    EXPECT_INT_EQ(document_layer_index(&document, third_layer_id), 1);
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document_layer_index(&document, third_layer_id), 2);
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document_layer_index(&document, third_layer_id), 1);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_command_execute_respects_locked_targets(void)
{
    Document document;
    CommandExecutor executor;
    SelectionSet selection = {0};
    ObjectId ids[1] = {1u};
    float width = 0.0f;

    document_init(&document);
    command_executor_init(&executor);

    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_set_property_from_document(&document,
                                                                                   1u,
                                                                                   "width",
                                                                                   20.0f),
                                         &document));
    EXPECT_TRUE(document_set_layer_locked(&document, 1u, 1));
    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_FLOAT_EQ(width, 20.0f);

    EXPECT_FALSE(command_executor_execute(&executor,
                                          command_create_set_property_from_document(&document,
                                                                                    1u,
                                                                                    "width",
                                                                                    30.0f),
                                          &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "width", &width));
    EXPECT_FLOAT_EQ(width, 20.0f);

    EXPECT_FALSE(command_executor_execute(&executor,
                                          command_create_move_objects(ids,
                                                                      1,
                                                                      vec2_make(5.0f, 0.0f)),
                                          &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &width));
    EXPECT_FLOAT_EQ(width, 0.0f);

    EXPECT_TRUE(selection_set_add(&selection, 1u));
    EXPECT_FALSE(command_executor_execute(&executor,
                                          command_create_delete_selection(&document, &selection),
                                          &document));
    EXPECT_INT_EQ(document.count, 1);

    EXPECT_TRUE(document_create_layer(&document, "Locked Overlay") != 0u);
    EXPECT_TRUE(document_set_active_layer(&document, 2u));
    EXPECT_TRUE(document_set_layer_locked(&document, 2u, 1));
    EXPECT_FALSE(command_executor_execute(&executor,
                                          command_create_create_object(
                                              make_rect(50.0f, 50.0f, 10.0f, 10.0f)),
                                          &document));
    EXPECT_INT_EQ(document.count, 1);

    selection_set_shutdown(&selection);
    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_command_can_execute_reports_capability(void)
{
    Document document;
    SelectionSet selection = {0};
    Command* command = NULL;

    document_init(&document);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));

    command = command_create_set_property_from_document(&document, 1u, "width", 20.0f);
    EXPECT_TRUE(command != NULL);
    EXPECT_TRUE(command_can_execute(command, &document));
    EXPECT_INT_EQ((int)command_check_execute(command, &document),
                  (int)COMMAND_EXECUTE_CHECK_OK);
    command->vtable->destroy(command);

    EXPECT_TRUE(document_set_layer_locked(&document, 1u, 1));

    command = command_create_set_property_from_document(&document, 1u, "width", 20.0f);
    EXPECT_TRUE(command != NULL);
    EXPECT_FALSE(command_can_execute(command, &document));
    EXPECT_INT_EQ((int)command_check_execute(command, &document),
                  (int)COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED);
    command->vtable->destroy(command);

    EXPECT_TRUE(selection_set_add(&selection, 1u));
    command = command_create_delete_selection(&document, &selection);
    EXPECT_TRUE(command != NULL);
    EXPECT_FALSE(command_can_execute(command, &document));
    EXPECT_INT_EQ((int)command_check_execute(command, &document),
                  (int)COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED);
    command->vtable->destroy(command);

    EXPECT_TRUE(document_create_layer(&document, "Locked Overlay") != 0u);
    EXPECT_TRUE(document_set_active_layer(&document, 2u));
    EXPECT_TRUE(document_set_layer_locked(&document, 2u, 1));
    command = command_create_create_object(make_rect(30.0f, 30.0f, 10.0f, 10.0f));
    EXPECT_TRUE(command != NULL);
    EXPECT_FALSE(command_can_execute(command, &document));
    EXPECT_INT_EQ((int)command_check_execute(command, &document),
                  (int)COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED);
    command->vtable->destroy(command);

    EXPECT_TRUE(strcmp(command_execute_check_message(COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED),
                       "Active layer is locked.") == 0);

    selection_set_shutdown(&selection);
    document_shutdown(&document);
    return 0;
}

static int test_paste_command_undo_redo_and_conflict_rollback(void)
{
    Document document;
    CommandExecutor executor;
    GraphicObject* clipboard[2] = {NULL, NULL};
    Command* command = NULL;
    SelectionSet pasted_selection = {0};
    float x = 0.0f;

    document_init(&document);
    command_executor_init(&executor);

    clipboard[0] = make_rect(0.0f, 0.0f, 10.0f, 10.0f);
    clipboard[1] = make_rect(20.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE(clipboard[0] != NULL);
    EXPECT_TRUE(clipboard[1] != NULL);

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_paste_objects(&document,
                                                                      clipboard,
                                                                      2,
                                                                      vec2_make(5.0f, 0.0f),
                                                                      1u,
                                                                      &pasted_selection),
                                         &document));
    EXPECT_INT_EQ(document.count, 2);
    EXPECT_INT_EQ(pasted_selection.count, 2);
    EXPECT_TRUE(get_scalar(&document, pasted_selection.ids[0], "x", &x));
    EXPECT_FLOAT_EQ(x, 5.0f);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_INT_EQ(document.count, 0);
    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_INT_EQ(document.count, 2);

    selection_set_shutdown(&pasted_selection);
    command_executor_shutdown(&executor);
    document_shutdown(&document);
    object_destroy(clipboard[0]);
    object_destroy(clipboard[1]);

    document_init(&document);
    command_executor_init(&executor);
    clipboard[0] = make_rect(0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE(clipboard[0] != NULL);

    command = command_create_paste_objects(&document,
                                           clipboard,
                                           1,
                                           vec2_make(0.0f, 0.0f),
                                           1u,
                                           NULL);
    EXPECT_TRUE(command != NULL);
    /* Occupy the predicted ID before execution to force append failure. */
    EXPECT_TRUE(document_add_object(&document, make_rect(100.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_FALSE(command_executor_execute(&executor, command, &document));
    EXPECT_INT_EQ(document.count, 1);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    object_destroy(clipboard[0]);
    return 0;
}

static int test_move_multiple_objects_undo(void)
{
    Document document;
    CommandExecutor executor;
    ObjectId ids[3] = {1u, 2u, 3u};
    float x1 = 0.0f;
    float x2 = 0.0f;
    float x3 = 0.0f;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(10.0f, 0.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(20.0f, 0.0f, 10.0f, 10.0f)));

    EXPECT_TRUE(command_executor_execute(&executor,
                                         command_create_move_objects(ids, 3, vec2_make(5.0f, -3.0f)),
                                         &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x1));
    EXPECT_TRUE(get_scalar(&document, 2u, "x", &x2));
    EXPECT_TRUE(get_scalar(&document, 3u, "x", &x3));
    EXPECT_FLOAT_EQ(x1, 5.0f);
    EXPECT_FLOAT_EQ(x2, 15.0f);
    EXPECT_FLOAT_EQ(x3, 25.0f);

    EXPECT_TRUE(command_executor_undo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x1));
    EXPECT_TRUE(get_scalar(&document, 2u, "x", &x2));
    EXPECT_TRUE(get_scalar(&document, 3u, "x", &x3));
    EXPECT_FLOAT_EQ(x1, 0.0f);
    EXPECT_FLOAT_EQ(x2, 10.0f);
    EXPECT_FLOAT_EQ(x3, 20.0f);

    EXPECT_TRUE(command_executor_redo(&executor, &document));
    EXPECT_TRUE(get_scalar(&document, 1u, "x", &x1));
    EXPECT_TRUE(get_scalar(&document, 2u, "x", &x2));
    EXPECT_TRUE(get_scalar(&document, 3u, "x", &x3));
    EXPECT_FLOAT_EQ(x1, 5.0f);
    EXPECT_FLOAT_EQ(x2, 15.0f);
    EXPECT_FLOAT_EQ(x3, 25.0f);

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

static int test_paste_on_locked_layer_rejected(void)
{
    Document document;
    CommandExecutor executor;
    GraphicObject* clipboard[1] = {NULL};
    Command* command = NULL;
    SelectionSet pasted_selection = {0};

    document_init(&document);
    command_executor_init(&executor);
    clipboard[0] = make_rect(0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE(clipboard[0] != NULL);

    EXPECT_TRUE(document_set_layer_locked(&document, 1u, 1));
    command = command_create_paste_objects(&document,
                                           clipboard,
                                           1,
                                           vec2_make(0.0f, 0.0f),
                                           1u,
                                           &pasted_selection);
    EXPECT_TRUE(command != NULL);
    EXPECT_FALSE(command_can_execute(command, &document));
    EXPECT_FALSE(command_executor_execute(&executor, command, &document));
    EXPECT_INT_EQ(document.count, 0);

    selection_set_shutdown(&pasted_selection);
    command_executor_shutdown(&executor);
    document_shutdown(&document);
    object_destroy(clipboard[0]);
    return 0;
}

static int test_paste_command_assigns_unique_ids(void)
{
    Document document;
    CommandExecutor executor;
    GraphicObject* clipboard[3] = {NULL, NULL, NULL};
    Command* command = NULL;
    SelectionSet pasted_selection = {0};

    document_init(&document);
    command_executor_init(&executor);
    clipboard[0] = make_rect(0.0f, 0.0f, 10.0f, 10.0f);
    clipboard[1] = make_rect(20.0f, 0.0f, 10.0f, 10.0f);
    clipboard[2] = make_rect(40.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE(clipboard[0] && clipboard[1] && clipboard[2]);

    command = command_create_paste_objects(&document,
                                           clipboard,
                                           3,
                                           vec2_make(0.0f, 0.0f),
                                           1u,
                                           &pasted_selection);
    EXPECT_TRUE(command != NULL);
    EXPECT_INT_EQ(pasted_selection.count, 3);
    EXPECT_TRUE(pasted_selection.ids[0] != pasted_selection.ids[1]);
    EXPECT_TRUE(pasted_selection.ids[1] != pasted_selection.ids[2]);
    EXPECT_TRUE(pasted_selection.ids[0] != pasted_selection.ids[2]);

    EXPECT_TRUE(command_executor_execute(&executor, command, &document));
    EXPECT_INT_EQ(document.count, 3);
    EXPECT_TRUE(document_find_object(&document, pasted_selection.ids[0]) != NULL);
    EXPECT_TRUE(document_find_object(&document, pasted_selection.ids[1]) != NULL);
    EXPECT_TRUE(document_find_object(&document, pasted_selection.ids[2]) != NULL);

    selection_set_shutdown(&pasted_selection);
    command_executor_shutdown(&executor);
    document_shutdown(&document);
    object_destroy(clipboard[0]);
    object_destroy(clipboard[1]);
    object_destroy(clipboard[2]);
    return 0;
}

static int test_execute_null_command_returns_failure(void)
{
    Document document;
    CommandExecutor executor;

    document_init(&document);
    command_executor_init(&executor);
    EXPECT_FALSE(command_executor_execute(&executor, NULL, &document));
    EXPECT_FALSE(command_executor_can_undo(&executor));
    EXPECT_FALSE(command_executor_undo(&executor, &document));
    EXPECT_FALSE(command_executor_redo(&executor, &document));

    command_executor_shutdown(&executor);
    document_shutdown(&document);
    return 0;
}

int main(void)
{
    extension_loader_register_all();

    if (test_create_object_command()) return 1;
    if (test_delete_selection_command()) return 1;
    if (test_move_objects_command_and_merge()) return 1;
    if (test_set_property_command_and_merge()) return 1;
    if (test_executor_allocation_failure_keeps_document_unchanged()) return 1;
    if (test_failed_undo_redo_keeps_history_entry()) return 1;
    if (test_transaction_commit_and_undo()) return 1;
    if (test_transaction_rollback_restores_document()) return 1;
    if (test_memory_budget_prunes_old_undo_entries()) return 1;
    if (test_layer_commands_undo_redo()) return 1;
    if (test_command_execute_respects_locked_targets()) return 1;
    if (test_command_can_execute_reports_capability()) return 1;
    if (test_paste_command_undo_redo_and_conflict_rollback()) return 1;
    if (test_move_multiple_objects_undo()) return 1;
    if (test_paste_on_locked_layer_rejected()) return 1;
    if (test_paste_command_assigns_unique_ids()) return 1;
    if (test_execute_null_command_returns_failure()) return 1;

    printf("[PASS] command executor core commands\n");
    return 0;
}
