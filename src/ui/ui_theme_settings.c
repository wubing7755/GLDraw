#include <base/file_utils.h>

#include "ui_theme_internal.h"

#include <stdio.h>
#include <stdlib.h>

int ui_theme_load_selected_id(const char* path, char* out_theme_id, size_t out_theme_id_size)
{
    char* text = NULL;
    int loaded = 0;

    if (!path || !out_theme_id || out_theme_id_size == 0u) {
        return 0;
    }

    out_theme_id[0] = '\0';
    text = file_utils_read_text_file(path);
    if (!text) {
        return 0;
    }

    loaded = ui_extract_json_string_value(text,
                                          UI_THEME_CONFIG_KEY,
                                          out_theme_id,
                                          out_theme_id_size);
    if (!loaded) {
        loaded = ui_extract_json_string_value(text,
                                              UI_THEME_COMPAT_KEY,
                                              out_theme_id,
                                              out_theme_id_size);
    }

    free(text);
    return loaded && out_theme_id[0] != '\0';
}

int ui_theme_save_selected_id(const char* path, const char* theme_id)
{
    FILE* file = NULL;
    char* temp_path = NULL;
    int close_result = 0;

    if (!path || !theme_id) {
        return 0;
    }
    if (ui_theme_index_of_id(theme_id) < 0) {
        return 0;
    }
    if (!ui_theme_id_is_safe_for_json(theme_id)) {
        return 0;
    }

    temp_path = file_utils_duplicate_path_with_suffix(path, ".tmp");
    if (!temp_path) {
        return 0;
    }

    file = fopen(temp_path, "wb");
    if (!file) {
        free(temp_path);
        return 0;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"%s\": \"%s\"\n", UI_THEME_CONFIG_KEY, theme_id);
    fprintf(file, "}\n");
    if (ferror(file)) {
        fclose(file);
        remove(temp_path);
        free(temp_path);
        return 0;
    }

    close_result = fclose(file);
    if (close_result != 0) {
        remove(temp_path);
        free(temp_path);
        return 0;
    }

    if (!file_utils_replace_file_with_temp(temp_path, path)) {
        free(temp_path);
        return 0;
    }

    free(temp_path);
    return 1;
}
