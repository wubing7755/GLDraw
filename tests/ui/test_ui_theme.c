#include <ui/ui_theme.h>

#include "../../src/ui/ui_theme_internal.h"
#include "../support/test_temp_files.h"

#include <stdio.h>
#include <string.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                      \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #expr);                              \
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
        if (actual_value < expected_value - 0.0001f ||                        \
            actual_value > expected_value + 0.0001f) {                        \
            fprintf(stderr, "EXPECT_FLOAT_EQ failed: %s:%d: %.4f != %.4f\n",  \
                    __FILE__, __LINE__, actual_value, expected_value);        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_STR_EQ(actual, expected)                                       \
    do {                                                                      \
        if (strcmp((actual), (expected)) != 0) {                              \
            fprintf(stderr, "EXPECT_STR_EQ failed: %s:%d: \"%s\" != \"%s\"\n", \
                    __FILE__, __LINE__, (actual), (expected));                \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static int write_text_file(const char* path, const char* text)
{
    FILE* file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    fputs(text, file);
    if (ferror(file)) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static void cleanup_theme_test_files(const char* directory)
{
    char path[TEST_TEMP_PATH_MAX];

    if (!directory) {
        return;
    }

    if (test_temp_join_path(path, sizeof(path), directory, "valid.json")) {
        remove(path);
    }
    if (test_temp_join_path(path, sizeof(path), directory, "invalid.json")) {
        remove(path);
    }
    if (test_temp_join_path(path, sizeof(path), directory, "settings.json")) {
        remove(path);
    }
    test_temp_remove_dir(directory);
}

static int color_equals(struct nk_color color,
                        unsigned char r,
                        unsigned char g,
                        unsigned char b,
                        unsigned char a)
{
    return color.r == r && color.g == g && color.b == b && color.a == a;
}

static int test_builtin_theme_lookup_and_fallback(void)
{
    UiThemeTokens default_tokens = ui_theme_default_tokens();
    UiThemeTokens fallback_tokens = ui_theme_tokens_for_id("missing-theme");
    const UiThemeDescriptor* default_descriptor = NULL;
    int default_index = -1;
    int dark_index = -1;

    EXPECT_TRUE(ui_theme_count() >= 3);
    EXPECT_STR_EQ(ui_theme_default_id(), "gldraw-light");

    default_index = ui_theme_index_of_id(ui_theme_default_id());
    EXPECT_INT_EQ(default_index, 0);

    default_descriptor = ui_theme_descriptor_at(default_index);
    EXPECT_TRUE(default_descriptor != NULL);
    EXPECT_STR_EQ(default_descriptor->id, "gldraw-light");
    EXPECT_TRUE(default_descriptor->label != NULL);

    dark_index = ui_theme_index_of_id("gldraw-dark-plus");
    EXPECT_TRUE(dark_index >= 0);
    EXPECT_TRUE(ui_theme_descriptor_at(dark_index) != NULL);

    EXPECT_TRUE(color_equals(fallback_tokens.primary,
                             default_tokens.primary.r,
                             default_tokens.primary.g,
                             default_tokens.primary.b,
                             default_tokens.primary.a));
    EXPECT_FLOAT_EQ(fallback_tokens.row_height, default_tokens.row_height);
    return 0;
}

static int test_json_scalar_values_require_delimiters(void)
{
    float number_value = 0.0f;
    int bool_value = -1;

    EXPECT_TRUE(ui_extract_json_number_value("{ \"row_height\": 24, \"gap\": 3 }",
                                             "row_height",
                                             &number_value));
    EXPECT_FLOAT_EQ(number_value, 24.0f);

    number_value = 12.0f;
    EXPECT_TRUE(!ui_extract_json_number_value("{ \"row_height\": 24px }",
                                              "row_height",
                                              &number_value));
    EXPECT_FLOAT_EQ(number_value, 12.0f);

    number_value = 12.0f;
    EXPECT_TRUE(!ui_extract_json_number_value("{ \"row_height\": 24 true }",
                                              "row_height",
                                              &number_value));
    EXPECT_FLOAT_EQ(number_value, 12.0f);

    EXPECT_TRUE(ui_extract_json_bool_value("{ \"enable_transitions\": true }",
                                           "enable_transitions",
                                           &bool_value));
    EXPECT_INT_EQ(bool_value, 1);

    bool_value = -1;
    EXPECT_TRUE(ui_extract_json_bool_value("{ \"enable_transitions\": false, \"x\": 1 }",
                                           "enable_transitions",
                                           &bool_value));
    EXPECT_INT_EQ(bool_value, 0);

    bool_value = -1;
    EXPECT_TRUE(!ui_extract_json_bool_value("{ \"enable_transitions\": truex }",
                                            "enable_transitions",
                                            &bool_value));
    EXPECT_INT_EQ(bool_value, -1);

    bool_value = -1;
    EXPECT_TRUE(!ui_extract_json_bool_value("{ \"enable_transitions\": false0 }",
                                            "enable_transitions",
                                            &bool_value));
    EXPECT_INT_EQ(bool_value, -1);

    return 0;
}

static int test_external_theme_load_error_and_clamp_behavior(void)
{
    UiThemeTokens tokens;
    char directory[TEST_TEMP_PATH_MAX];
    char valid_path[TEST_TEMP_PATH_MAX];
    char invalid_path[TEST_TEMP_PATH_MAX];
    int base_count = ui_theme_count();
    int custom_index = -1;

    EXPECT_TRUE(test_temp_make_dir(directory, sizeof(directory), "ui-theme"));
    EXPECT_TRUE(test_temp_join_path(valid_path, sizeof(valid_path), directory, "valid.json"));
    EXPECT_TRUE(test_temp_join_path(invalid_path, sizeof(invalid_path), directory, "invalid.json"));
    EXPECT_TRUE(write_text_file(valid_path,
                                "{\n"
                                "  \"id\": \"test-clamped-theme\",\n"
                                "  \"label\": \"Test Clamped Theme\",\n"
                                "  \"base_theme\": \"gldraw-dark-plus\",\n"
                                "  \"primary\": \"#11223344\",\n"
                                "  \"background\": \"#abcdef\",\n"
                                "  \"row_height\": 2,\n"
                                "  \"panel_width\": 9999,\n"
                                "  \"menu_height\": 12,\n"
                                "  \"status_height\": 120,\n"
                                "  \"tool_rail_width\": 12,\n"
                                "  \"transition_duration\": 4,\n"
                                "  \"enable_transitions\": false\n"
                                "}\n"));

    EXPECT_INT_EQ(ui_theme_reload_external(directory), 1);
    custom_index = ui_theme_index_of_id("test-clamped-theme");
    EXPECT_TRUE(custom_index >= base_count);

    tokens = ui_theme_tokens_for_id("test-clamped-theme");
    EXPECT_TRUE(color_equals(tokens.primary, 0x11, 0x22, 0x33, 0x44));
    EXPECT_TRUE(color_equals(tokens.background, 0xab, 0xcd, 0xef, 0xff));
    EXPECT_FLOAT_EQ(tokens.row_height, 18.0f);
    EXPECT_FLOAT_EQ(tokens.panel_width, 720.0f);
    EXPECT_FLOAT_EQ(tokens.menu_height, 20.0f);
    EXPECT_FLOAT_EQ(tokens.status_height, 64.0f);
    EXPECT_FLOAT_EQ(tokens.tool_rail_width, 48.0f);
    EXPECT_FLOAT_EQ(tokens.transition_duration, 2.0f);
    EXPECT_INT_EQ(tokens.enable_transitions, 0);

    EXPECT_TRUE(write_text_file(invalid_path,
                                "{ \"id\": \"not a safe id\" }\n"));
    EXPECT_INT_EQ(ui_theme_reload_external(directory), -1);
    EXPECT_TRUE(ui_theme_last_reload_error()[0] != '\0');
    EXPECT_TRUE(ui_theme_index_of_id("test-clamped-theme") >= base_count);

    cleanup_theme_test_files(directory);
    return 0;
}

static int test_selected_theme_id_load_save(void)
{
    char loaded_theme_id[64];
    char directory[TEST_TEMP_PATH_MAX];
    char settings_path[TEST_TEMP_PATH_MAX];

    EXPECT_TRUE(test_temp_make_dir(directory, sizeof(directory), "ui-theme-settings"));
    EXPECT_TRUE(test_temp_join_path(settings_path,
                                    sizeof(settings_path),
                                    directory,
                                    "settings.json"));
    EXPECT_TRUE(ui_theme_save_selected_id(settings_path,
                                          "gldraw-dark-plus"));
    EXPECT_TRUE(ui_theme_load_selected_id(settings_path,
                                          loaded_theme_id,
                                          sizeof(loaded_theme_id)));
    EXPECT_STR_EQ(loaded_theme_id, "gldraw-dark-plus");

    EXPECT_TRUE(write_text_file(settings_path,
                                "{ \"theme\": \"gldraw-high-contrast\" }\n"));
    EXPECT_TRUE(ui_theme_load_selected_id(settings_path,
                                          loaded_theme_id,
                                          sizeof(loaded_theme_id)));
    EXPECT_STR_EQ(loaded_theme_id, "gldraw-high-contrast");

    cleanup_theme_test_files(directory);
    return 0;
}

int main(void)
{
    if (test_builtin_theme_lookup_and_fallback()) return 1;
    if (test_json_scalar_values_require_delimiters()) return 1;
    if (test_external_theme_load_error_and_clamp_behavior()) return 1;
    if (test_selected_theme_id_load_save()) return 1;

    printf("[PASS] ui theme\n");
    return 0;
}
