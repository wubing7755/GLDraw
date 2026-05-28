#include <base/file_utils.h>
#include <base/log.h>
#include <base/path_utils.h>

#include "ui_theme_internal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static UiCustomThemeEntry g_custom_theme_entries[UI_THEME_MAX_CUSTOM];
static int g_custom_theme_count = 0;
static char g_last_reload_error[256] = "";

static unsigned long long ui_theme_hash_bytes(unsigned long long seed, const void* data, size_t size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    unsigned long long hash = seed;

    if (!bytes) {
        return hash;
    }

    for (size_t i = 0u; i < size; ++i) {
        hash ^= (unsigned long long)bytes[i];
        hash *= UI_THEME_HASH_PRIME;
    }
    return hash;
}

static unsigned long long ui_theme_hash_cstring(unsigned long long seed, const char* text)
{
    if (!text) {
        return seed;
    }
    return ui_theme_hash_bytes(seed, text, strlen(text));
}

static unsigned long long ui_theme_hash_u64(unsigned long long seed, unsigned long long value)
{
    return ui_theme_hash_bytes(seed, &value, sizeof(value));
}

int ui_theme_custom_count(void)
{
    return g_custom_theme_count;
}

const UiThemeDescriptor* ui_theme_custom_descriptor_at(int custom_index)
{
    if (custom_index < 0 || custom_index >= g_custom_theme_count) {
        return NULL;
    }
    return &g_custom_theme_entries[custom_index].descriptor;
}

int ui_theme_custom_index_of_id(const char* theme_id)
{
    if (!theme_id || theme_id[0] == '\0') {
        return -1;
    }

    for (int i = 0; i < g_custom_theme_count; ++i) {
        if (strcmp(theme_id, g_custom_theme_entries[i].descriptor.id) == 0) {
            return i;
        }
    }
    return -1;
}

UiThemeTokens ui_theme_custom_tokens_at(int custom_index)
{
    if (custom_index < 0 || custom_index >= g_custom_theme_count) {
        return ui_theme_default_tokens();
    }
    return g_custom_theme_entries[custom_index].tokens;
}

static int ui_theme_store_custom_entry(const char* theme_id,
                                       const char* label,
                                       const UiThemeTokens* tokens)
{
    int existing_index = -1;
    int target_index = -1;

    if (!theme_id || !tokens) {
        return 0;
    }
    if (!ui_theme_id_is_safe_for_json(theme_id)) {
        return 0;
    }
    if (ui_theme_index_of_id(theme_id) >= 0 && ui_theme_custom_index_of_id(theme_id) < 0) {
        return 0;
    }

    existing_index = ui_theme_custom_index_of_id(theme_id);
    if (existing_index >= 0) {
        target_index = existing_index;
    } else {
        if (g_custom_theme_count >= UI_THEME_MAX_CUSTOM) {
            return 0;
        }
        target_index = g_custom_theme_count++;
    }

    snprintf(g_custom_theme_entries[target_index].id,
             sizeof(g_custom_theme_entries[target_index].id),
             "%s",
             theme_id);
    snprintf(g_custom_theme_entries[target_index].label,
             sizeof(g_custom_theme_entries[target_index].label),
             "%s",
             (label && label[0] != '\0') ? label : theme_id);

    g_custom_theme_entries[target_index].descriptor.id = g_custom_theme_entries[target_index].id;
    g_custom_theme_entries[target_index].descriptor.label = g_custom_theme_entries[target_index].label;
    g_custom_theme_entries[target_index].tokens = *tokens;
    return 1;
}

static int ui_theme_load_external_file(const char* path)
{
    char* text = NULL;
    char theme_id[UI_THEME_MAX_ID_LEN];
    char theme_label[UI_THEME_MAX_LABEL_LEN];
    char base_theme_id[UI_THEME_MAX_ID_LEN];
    UiThemeTokens tokens;
    int base_index = -1;
    int loaded = 0;

    if (!path) {
        return 0;
    }

    text = file_utils_read_text_file(path);
    if (!text) {
        return 0;
    }

    theme_id[0] = '\0';
    theme_label[0] = '\0';
    base_theme_id[0] = '\0';

    if (!ui_extract_json_string_value(text, "id", theme_id, sizeof(theme_id))) {
        ui_theme_id_from_path(path, theme_id, sizeof(theme_id));
    }
    if (!ui_theme_id_is_safe_for_json(theme_id)) {
        LOG_WARN("[theme][parse][id] invalid theme id path=%s id=%s",
                 path,
                 theme_id);
        free(text);
        return 0;
    }

    ui_extract_json_string_value(text, "label", theme_label, sizeof(theme_label));
    if (!ui_extract_json_string_value(text, "base_theme", base_theme_id, sizeof(base_theme_id))) {
        if (!ui_extract_json_string_value(text, "base", base_theme_id, sizeof(base_theme_id))) {
            ui_extract_json_string_value(text, "extends", base_theme_id, sizeof(base_theme_id));
        }
    }

    base_index = ui_theme_index_of_id(base_theme_id);
    if (base_index >= 0) {
        const UiThemeDescriptor* base_descriptor = ui_theme_descriptor_at(base_index);
        tokens = ui_theme_tokens_for_id(base_descriptor ? base_descriptor->id : ui_theme_default_id());
    } else {
        tokens = ui_theme_default_tokens();
    }

    ui_theme_apply_color_overrides(path, text, &tokens);
    ui_theme_apply_float_overrides(path, text, &tokens);
    ui_theme_apply_bool_overrides(path, text, &tokens);
    ui_theme_clamp_tokens(&tokens);
    loaded = ui_theme_store_custom_entry(theme_id, theme_label, &tokens);
    if (!loaded) {
        LOG_WARN("[theme][parse][store] failed to store parsed theme path=%s id=%s",
                 path,
                 theme_id);
    }

    free(text);
    return loaded;
}

int ui_theme_reload_external(const char* directory_path)
{
    int loaded_count = 0;
    int parse_failed = 0;
    int scanned_json_files = 0;
    UiCustomThemeEntry previous_entries[UI_THEME_MAX_CUSTOM];
    int previous_count = g_custom_theme_count;

    memcpy(previous_entries, g_custom_theme_entries, sizeof(previous_entries));
    g_custom_theme_count = 0;
    memset(g_custom_theme_entries, 0, sizeof(g_custom_theme_entries));
    g_last_reload_error[0] = '\0';

    if (!directory_path || directory_path[0] == '\0') {
        return 0;
    }

#ifdef _WIN32
    {
        WIN32_FIND_DATAA find_data;
        HANDLE find_handle = INVALID_HANDLE_VALUE;
        char search_pattern[UI_THEME_MAX_PATH_LEN];

        snprintf(search_pattern, sizeof(search_pattern), "%s\\*.json", directory_path);
        find_handle = FindFirstFileA(search_pattern, &find_data);
        if (find_handle == INVALID_HANDLE_VALUE) {
            return 0;
        }

        do {
            char file_path[UI_THEME_MAX_PATH_LEN];
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                continue;
            }

            snprintf(file_path, sizeof(file_path), "%s\\%s", directory_path, find_data.cFileName);
            scanned_json_files++;
            if (ui_theme_load_external_file(file_path)) {
                loaded_count++;
            } else {
                parse_failed = 1;
                if (g_last_reload_error[0] == '\0') {
                    snprintf(g_last_reload_error,
                             sizeof(g_last_reload_error),
                             "Failed parsing theme file: %.200s",
                             file_path);
                }
            }
        } while (FindNextFileA(find_handle, &find_data));

        FindClose(find_handle);
    }
#else
    {
        DIR* directory = NULL;
        struct dirent* entry = NULL;

        directory = opendir(directory_path);
        if (!directory) {
            return 0;
        }

        while ((entry = readdir(directory)) != NULL) {
            char file_path[UI_THEME_MAX_PATH_LEN];

            if (entry->d_name[0] == '.') {
                continue;
            }
            if (!path_utils_has_extension(entry->d_name, ".json")) {
                continue;
            }

            snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);
            scanned_json_files++;
            if (ui_theme_load_external_file(file_path)) {
                loaded_count++;
            } else {
                parse_failed = 1;
                if (g_last_reload_error[0] == '\0') {
                    snprintf(g_last_reload_error,
                             sizeof(g_last_reload_error),
                             "Failed parsing theme file: %.200s",
                             file_path);
                }
            }
        }

        closedir(directory);
    }
#endif

    if (parse_failed) {
        memcpy(g_custom_theme_entries, previous_entries, sizeof(previous_entries));
        g_custom_theme_count = previous_count;
        if (g_last_reload_error[0] == '\0') {
            snprintf(g_last_reload_error,
                     sizeof(g_last_reload_error),
                     "Theme reload failed (%d json files scanned)",
                     scanned_json_files);
        }
        LOG_ERROR("[theme][reload] %s", g_last_reload_error);
        return -1;
    }

    return loaded_count;
}

const char* ui_theme_last_reload_error(void)
{
    return g_last_reload_error;
}

unsigned long long ui_theme_external_signature(const char* directory_path)
{
    unsigned long long aggregate_hash = UI_THEME_HASH_OFFSET_BASIS;
    unsigned long long file_count = 0ull;

    if (!directory_path || directory_path[0] == '\0') {
        return aggregate_hash;
    }

#ifdef _WIN32
    {
        WIN32_FIND_DATAA find_data;
        HANDLE find_handle = INVALID_HANDLE_VALUE;
        char search_pattern[UI_THEME_MAX_PATH_LEN];

        snprintf(search_pattern, sizeof(search_pattern), "%s\\*.json", directory_path);
        find_handle = FindFirstFileA(search_pattern, &find_data);
        if (find_handle == INVALID_HANDLE_VALUE) {
            return aggregate_hash;
        }

        do {
            unsigned long long file_hash = UI_THEME_HASH_OFFSET_BASIS;
            unsigned long long file_size = 0ull;
            unsigned long long file_time = 0ull;

            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                continue;
            }

            file_size = ((unsigned long long)find_data.nFileSizeHigh << 32) |
                        (unsigned long long)find_data.nFileSizeLow;
            file_time = ((unsigned long long)find_data.ftLastWriteTime.dwHighDateTime << 32) |
                        (unsigned long long)find_data.ftLastWriteTime.dwLowDateTime;

            file_hash = ui_theme_hash_cstring(file_hash, find_data.cFileName);
            file_hash = ui_theme_hash_u64(file_hash, file_size);
            file_hash = ui_theme_hash_u64(file_hash, file_time);

            aggregate_hash ^= file_hash;
            file_count++;
        } while (FindNextFileA(find_handle, &find_data));

        FindClose(find_handle);
    }
#else
    {
        DIR* directory = NULL;
        struct dirent* entry = NULL;

        directory = opendir(directory_path);
        if (!directory) {
            return aggregate_hash;
        }

        while ((entry = readdir(directory)) != NULL) {
            char file_path[UI_THEME_MAX_PATH_LEN];
            struct stat info;
            unsigned long long file_hash = UI_THEME_HASH_OFFSET_BASIS;
            unsigned long long file_size = 0ull;
            unsigned long long file_time = 0ull;

            if (entry->d_name[0] == '.') {
                continue;
            }
            if (!path_utils_has_extension(entry->d_name, ".json")) {
                continue;
            }

            snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);
            if (stat(file_path, &info) != 0) {
                continue;
            }

            file_size = (unsigned long long)info.st_size;
            file_time = (unsigned long long)info.st_mtime;

            file_hash = ui_theme_hash_cstring(file_hash, entry->d_name);
            file_hash = ui_theme_hash_u64(file_hash, file_size);
            file_hash = ui_theme_hash_u64(file_hash, file_time);

            aggregate_hash ^= file_hash;
            file_count++;
        }

        closedir(directory);
    }
#endif

    aggregate_hash = ui_theme_hash_u64(aggregate_hash, file_count);
    return aggregate_hash;
}
