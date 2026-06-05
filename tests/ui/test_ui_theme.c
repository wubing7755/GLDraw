#include <ui/ui_theme.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define test_mkdir(path) _mkdir(path)
#define test_rmdir(path) _rmdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define test_mkdir(path) mkdir((path), 0777)
#define test_rmdir(path) rmdir(path)
#endif

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

static int ensure_directory(const char* path)
{
    if (test_mkdir(path) == 0 || errno == EEXIST) {
        return 1;
    }
    return 0;
}

static void cleanup_theme_test_files(void)
{
    remove("gldraw_theme_test_tmp/valid.json");
    remove("gldraw_theme_test_tmp/invalid.json");
    remove("gldraw_theme_test_tmp/settings.json");
    test_rmdir("gldraw_theme_test_tmp");
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

static int test_external_theme_load_error_and_clamp_behavior(void)
{
    UiThemeTokens tokens;
    int base_count = ui_theme_count();
    int custom_index = -1;

    cleanup_theme_test_files();
    EXPECT_TRUE(ensure_directory("gldraw_theme_test_tmp"));
    EXPECT_TRUE(write_text_file("gldraw_theme_test_tmp/valid.json",
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

    EXPECT_INT_EQ(ui_theme_reload_external("gldraw_theme_test_tmp"), 1);
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

    EXPECT_TRUE(write_text_file("gldraw_theme_test_tmp/invalid.json",
                                "{ \"id\": \"not a safe id\" }\n"));
    EXPECT_INT_EQ(ui_theme_reload_external("gldraw_theme_test_tmp"), -1);
    EXPECT_TRUE(ui_theme_last_reload_error()[0] != '\0');
    EXPECT_TRUE(ui_theme_index_of_id("test-clamped-theme") >= base_count);

    cleanup_theme_test_files();
    return 0;
}

static int test_selected_theme_id_load_save(void)
{
    char loaded_theme_id[64];

    cleanup_theme_test_files();
    EXPECT_TRUE(ensure_directory("gldraw_theme_test_tmp"));
    EXPECT_TRUE(ui_theme_save_selected_id("gldraw_theme_test_tmp/settings.json",
                                          "gldraw-dark-plus"));
    EXPECT_TRUE(ui_theme_load_selected_id("gldraw_theme_test_tmp/settings.json",
                                          loaded_theme_id,
                                          sizeof(loaded_theme_id)));
    EXPECT_STR_EQ(loaded_theme_id, "gldraw-dark-plus");

    EXPECT_TRUE(write_text_file("gldraw_theme_test_tmp/settings.json",
                                "{ \"theme\": \"gldraw-high-contrast\" }\n"));
    EXPECT_TRUE(ui_theme_load_selected_id("gldraw_theme_test_tmp/settings.json",
                                          loaded_theme_id,
                                          sizeof(loaded_theme_id)));
    EXPECT_STR_EQ(loaded_theme_id, "gldraw-high-contrast");

    cleanup_theme_test_files();
    return 0;
}

int main(void)
{
    if (test_builtin_theme_lookup_and_fallback()) return 1;
    if (test_external_theme_load_error_and_clamp_behavior()) return 1;
    if (test_selected_theme_id_load_save()) return 1;

    printf("[PASS] ui theme\n");
    return 0;
}
