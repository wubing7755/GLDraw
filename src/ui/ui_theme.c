/**
 * @file ui_theme.c
 * @brief Theme token definitions, external theme loading, and Nuklear style apply.
 *
 * Role in project:
 * - Maintains built-in theme presets and runtime custom theme registry.
 * - Parses simple JSON theme files and maps tokens into UI style structures.
 *
 * Module relationships:
 * - Consumed by `ui_system`/`ui_menubar`.
 * - Uses file-system helpers and lightweight parsing utilities.
 */
#include <nuklear/nuklear.h>
#include <base/file_utils.h>
#include <base/log.h>
#include <base/path_utils.h>
#include <ui/ui_theme.h>

#include "ui_theme_internal.h"

#include <ctype.h>
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

/**
 * @brief Blends two color channels.
 * @param a First color.
 * @param b Second color.
 * @param t Blend factor.
 * @return Blended color.
 */
static struct nk_color ui_theme_mix_channel(struct nk_color a, struct nk_color b, float t)
{
    struct nk_color out;
    float clamped_t = t;

    if (clamped_t < 0.0f) {
        clamped_t = 0.0f;
    } else if (clamped_t > 1.0f) {
        clamped_t = 1.0f;
    }

    out.r = (nk_byte)((float)a.r + ((float)b.r - (float)a.r) * clamped_t);
    out.g = (nk_byte)((float)a.g + ((float)b.g - (float)a.g) * clamped_t);
    out.b = (nk_byte)((float)a.b + ((float)b.b - (float)a.b) * clamped_t);
    out.a = (nk_byte)((float)a.a + ((float)b.a - (float)a.a) * clamped_t);
    return out;
}

/**
 * @brief Gets custom theme index by ID.
 * @param theme_id Theme ID.
 * @return Index or -1 if not found.
 */
static int ui_theme_custom_index_of_id(const char* theme_id)
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

/**
 * @brief Gets the total theme count.
 * @param void No parameters.
 * @return Total theme count.
 */
int ui_theme_count(void)
{
    return ui_theme_builtin_count() + g_custom_theme_count;
}

/**
 * @brief Gets theme descriptor at index.
 * @param index Theme index.
 * @return Theme descriptor or NULL.
 */
const UiThemeDescriptor* ui_theme_descriptor_at(int index)
{
    int builtin_count = ui_theme_builtin_count();
    int custom_index = index - builtin_count;

    if (index < 0 || index >= ui_theme_count()) {
        return NULL;
    }
    if (index < builtin_count) {
        return ui_theme_builtin_descriptor_at(index);
    }
    return &g_custom_theme_entries[custom_index].descriptor;
}

/**
 * @brief Gets theme index by ID.
 * @param theme_id Theme ID.
 * @return Index or -1 if not found.
 */
int ui_theme_index_of_id(const char* theme_id)
{
    int builtin_count = ui_theme_builtin_count();
    int custom_index = -1;

    if (!theme_id || theme_id[0] == '\0') {
        return UI_THEME_LIGHT_INDEX;
    }

    for (int i = 0; i < builtin_count; ++i) {
        const UiThemeDescriptor* descriptor = ui_theme_builtin_descriptor_at(i);
        if (descriptor && strcmp(theme_id, descriptor->id) == 0) {
            return i;
        }
    }

    custom_index = ui_theme_custom_index_of_id(theme_id);
    if (custom_index >= 0) {
        return builtin_count + custom_index;
    }

    return -1;
}

/**
 * @brief Gets the default theme ID.
 * @param void No parameters.
 * @return Default theme ID string.
 */
const char* ui_theme_default_id(void)
{
    const UiThemeDescriptor* descriptor = ui_theme_builtin_descriptor_at(UI_THEME_LIGHT_INDEX);
    return descriptor ? descriptor->id : "gldraw-light";
}

/**
 * @brief Gets theme tokens by ID.
 * @param theme_id Theme ID.
 * @return Theme tokens.
 */
UiThemeTokens ui_theme_tokens_for_id(const char* theme_id)
{
    int theme_index = ui_theme_index_of_id(theme_id);
    int builtin_count = ui_theme_builtin_count();

    if (theme_index < 0) {
        return ui_theme_default_tokens();
    }
    if (theme_index < builtin_count) {
        return ui_theme_builtin_tokens_at(theme_index);
    }
    return g_custom_theme_entries[theme_index - builtin_count].tokens;
}

/**
 * @brief Computes hash of byte data.
 * @param seed Hash seed.
 * @param data Data bytes.
 * @param size Data size.
 * @return Hash value.
 */
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

/**
 * @brief Computes hash of a C string.
 * @param seed Hash seed.
 * @param text C string.
 * @return Hash value.
 */
static unsigned long long ui_theme_hash_cstring(unsigned long long seed, const char* text)
{
    if (!text) {
        return seed;
    }
    return ui_theme_hash_bytes(seed, text, strlen(text));
}

/**
 * @brief Computes hash of a 64-bit value.
 * @param seed Hash seed.
 * @param value 64-bit value.
 * @return Hash value.
 */
static unsigned long long ui_theme_hash_u64(unsigned long long seed, unsigned long long value)
{
    return ui_theme_hash_bytes(seed, &value, sizeof(value));
}

/**
 * @brief Stores a custom theme entry.
 * @param theme_id Theme ID.
 * @param label Theme label.
 * @param tokens Theme tokens.
 * @return 1 on success, 0 on failure.
 */
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
        /* Built-in IDs are reserved and cannot be overridden by file themes. */
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

/**
 * @brief Loads an external theme file.
 * @param path Theme file path.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Rescans and reloads the external theme directory.
 * @param directory_path [in] Theme directory path.
 * @return Number of successfully loaded themes.
 *
 * Algorithm steps:
 * 1. Backs up current external theme set.
 * 2. Clears runtime external theme table.
 * 3. Scans .json files in directory and parses each.
 * 4. If all fail this round and cache existed before, rolls back to old set.
 */
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

/**
 * @brief Gets the last reload error message.
 * @param void No parameters.
 * @return Error message string.
 */
const char* ui_theme_last_reload_error(void)
{
    return g_last_reload_error;
}

/**
 * @brief Computes signature of external theme directory.
 * @param directory_path Directory path.
 * @return Directory signature hash.
 */
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

/**
 * @brief Loads the selected theme ID from settings file.
 * @param path Settings file path.
 * @param out_theme_id Output theme ID buffer.
 * @param out_theme_id_size Buffer size.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Saves the selected theme ID to settings file.
 * @param path Settings file path.
 * @param theme_id Theme ID to save.
 * @return 1 on success, 0 on failure.
 */
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

/**
 * @brief Applies theme tokens to Nuklear context.
 * @param ctx Nuklear context.
 * @param tokens Theme tokens to apply.
 * @return None.
 */
void ui_theme_apply(struct nk_context* ctx, const UiThemeTokens* tokens)
{
    struct nk_color table[NK_COLOR_COUNT];
    UiThemeTokens local_tokens;
    struct nk_color subtle_panel;
    struct nk_color hover_panel;

    if (!ctx) {
        return;
    }

    local_tokens = tokens ? *tokens : ui_theme_default_tokens();
    subtle_panel = ui_theme_mix_channel(local_tokens.panel, local_tokens.background, 0.5f);
    hover_panel = ui_theme_mix_channel(local_tokens.panel_hover, local_tokens.background, 0.35f);

    table[NK_COLOR_TEXT] = local_tokens.text;
    table[NK_COLOR_WINDOW] = local_tokens.panel;
    table[NK_COLOR_HEADER] = ui_theme_mix_channel(local_tokens.panel, local_tokens.background, 0.60f);
    table[NK_COLOR_BORDER] = local_tokens.border;
    table[NK_COLOR_BUTTON] = subtle_panel;
    table[NK_COLOR_BUTTON_HOVER] = local_tokens.panel_hover;
    table[NK_COLOR_BUTTON_ACTIVE] = local_tokens.primary_active;
    table[NK_COLOR_TOGGLE] = subtle_panel;
    table[NK_COLOR_TOGGLE_HOVER] = local_tokens.panel_hover;
    table[NK_COLOR_TOGGLE_CURSOR] = local_tokens.primary;
    table[NK_COLOR_SELECT] = local_tokens.panel_hover;
    table[NK_COLOR_SELECT_ACTIVE] = local_tokens.primary;
    table[NK_COLOR_SLIDER] = subtle_panel;
    table[NK_COLOR_SLIDER_CURSOR] = local_tokens.primary;
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = local_tokens.primary_hover;
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = local_tokens.primary_active;
    table[NK_COLOR_PROPERTY] = subtle_panel;
    table[NK_COLOR_EDIT] = local_tokens.panel;
    table[NK_COLOR_EDIT_CURSOR] = local_tokens.text;
    table[NK_COLOR_COMBO] = subtle_panel;
    table[NK_COLOR_CHART] = subtle_panel;
    table[NK_COLOR_CHART_COLOR] = local_tokens.primary;
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = local_tokens.warning;
    table[NK_COLOR_SCROLLBAR] = subtle_panel;
    table[NK_COLOR_SCROLLBAR_CURSOR] = local_tokens.panel_hover;
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = hover_panel;
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = local_tokens.border_hover;
    table[NK_COLOR_TAB_HEADER] = local_tokens.panel;
    table[NK_COLOR_KNOB] = subtle_panel;
    table[NK_COLOR_KNOB_CURSOR] = local_tokens.primary;
    table[NK_COLOR_KNOB_CURSOR_HOVER] = local_tokens.primary_hover;
    table[NK_COLOR_KNOB_CURSOR_ACTIVE] = local_tokens.primary_active;

    nk_style_default(ctx);
    nk_style_from_table(ctx, table);

    ctx->style.window.background = local_tokens.panel;
    ctx->style.window.fixed_background = nk_style_item_color(local_tokens.panel);
    ctx->style.window.border_color = local_tokens.border;
    ctx->style.window.rounding = local_tokens.border_radius;
    ctx->style.window.border = 1.0f;
    ctx->style.window.spacing = nk_vec2(local_tokens.padding * 0.8f, local_tokens.padding * 0.8f);
    ctx->style.window.padding = nk_vec2(local_tokens.padding * 1.6f, local_tokens.padding * 1.3f);
    ctx->style.window.group_padding = nk_vec2(local_tokens.padding * 1.6f, local_tokens.padding * 1.3f);
    ctx->style.window.tooltip_padding = nk_vec2(local_tokens.padding * 1.8f, local_tokens.padding * 1.4f);
    ctx->style.window.menu_padding = nk_vec2(local_tokens.padding * 1.3f, local_tokens.padding * 1.1f);
    ctx->style.window.tooltip_border = 1.0f;
    ctx->style.window.tooltip_border_color = local_tokens.border_hover;

    ctx->style.button.rounding = local_tokens.border_radius;
    ctx->style.button.border = 1.0f;
    ctx->style.button.border_color = local_tokens.border;
    ctx->style.button.padding = nk_vec2(local_tokens.padding * 1.5f, local_tokens.padding * 1.1f);
    ctx->style.button.text_background = local_tokens.panel;
    ctx->style.button.text_normal = local_tokens.text;
    ctx->style.button.text_hover = local_tokens.text;
    ctx->style.button.text_active = nk_rgba(255, 255, 255, 255);

    ctx->style.menu_button.rounding = local_tokens.border_radius;
    ctx->style.menu_button.border = 0.0f;
    ctx->style.menu_button.padding = nk_vec2(local_tokens.padding * 1.3f, local_tokens.padding);
    ctx->style.menu_button.text_normal = local_tokens.text;
    ctx->style.menu_button.text_hover = local_tokens.text;
    ctx->style.menu_button.text_active = local_tokens.text;
    ctx->style.menu_button.normal = nk_style_item_color(local_tokens.panel);
    ctx->style.menu_button.hover = nk_style_item_color(local_tokens.panel_hover);
    ctx->style.menu_button.active = nk_style_item_color(hover_panel);
}
