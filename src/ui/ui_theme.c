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
#include <ui/ui_theme.h>
#include <base/log.h>

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

#define UI_THEME_CONFIG_KEY "workbench.colorTheme"
#define UI_THEME_COMPAT_KEY "theme"
#define UI_THEME_MAX_CUSTOM 32
#define UI_THEME_MAX_ID_LEN 64
#define UI_THEME_MAX_LABEL_LEN 96
#define UI_THEME_MAX_PATH_LEN 512
#define UI_THEME_HASH_OFFSET_BASIS 1469598103934665603ull
#define UI_THEME_HASH_PRIME 1099511628211ull

enum {
    UI_THEME_LIGHT_INDEX = 0,
    UI_THEME_DARK_PLUS_INDEX = 1,
    UI_THEME_HIGH_CONTRAST_INDEX = 2
};

typedef struct UiCustomThemeEntry {
    UiThemeDescriptor descriptor;
    UiThemeTokens tokens;
    char id[UI_THEME_MAX_ID_LEN];
    char label[UI_THEME_MAX_LABEL_LEN];
} UiCustomThemeEntry;

static const UiThemeDescriptor g_builtin_theme_descriptors[] = {
    {"gldraw-light", "GLDraw Light"},
    {"gldraw-dark-plus", "GLDraw Dark+"},
    {"gldraw-high-contrast", "GLDraw High Contrast"},
};

static UiCustomThemeEntry g_custom_theme_entries[UI_THEME_MAX_CUSTOM];
static int g_custom_theme_count = 0;
static char g_last_reload_error[256] = "";

static UiThemeTokens ui_theme_light_tokens(void);
static UiThemeTokens ui_theme_dark_plus_tokens(void);
static UiThemeTokens ui_theme_high_contrast_tokens(void);
static int ui_theme_builtin_count(void);

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
 * @brief Gets light theme tokens.
 * @param void No parameters.
 * @return Light theme tokens.
 */
static UiThemeTokens ui_theme_light_tokens(void)
{
    UiThemeTokens tokens;

    tokens.primary = nk_rgba(47, 109, 188, 255);
    tokens.primary_hover = nk_rgba(62, 123, 205, 255);
    tokens.primary_active = nk_rgba(37, 89, 160, 255);

    tokens.background = nk_rgba(226, 233, 241, 255);
    tokens.panel = nk_rgba(244, 248, 253, 255);
    tokens.panel_hover = nk_rgba(233, 240, 248, 255);
    tokens.canvas_background = nk_rgba(218, 226, 236, 255);

    tokens.text = nk_rgba(24, 35, 49, 255);
    tokens.text_secondary = nk_rgba(84, 96, 111, 255);
    tokens.text_disabled = nk_rgba(140, 151, 166, 255);

    tokens.border = nk_rgba(181, 194, 212, 255);
    tokens.border_hover = nk_rgba(149, 166, 189, 255);

    tokens.success = nk_rgba(45, 143, 105, 255);
    tokens.warning = nk_rgba(194, 129, 42, 255);
    tokens.error = nk_rgba(182, 74, 74, 255);

    tokens.row_height = 30.0f;
    tokens.panel_width = 292.0f;
    tokens.menu_height = 34.0f;
    tokens.status_height = 32.0f;
    tokens.tool_rail_width = 96.0f;

    tokens.padding = 5.0f;
    tokens.margin = 0.0f;
    tokens.gap = 0.0f;

    tokens.border_radius = 0.0f;

    tokens.enable_transitions = nk_true;
    tokens.transition_duration = 0.12f;
    return tokens;
}

/**
 * @brief Gets dark plus theme tokens.
 * @param void No parameters.
 * @return Dark plus theme tokens.
 */
static UiThemeTokens ui_theme_dark_plus_tokens(void)
{
    UiThemeTokens tokens;

    tokens.primary = nk_rgba(86, 156, 214, 255);
    tokens.primary_hover = nk_rgba(104, 173, 228, 255);
    tokens.primary_active = nk_rgba(63, 138, 203, 255);

    tokens.background = nk_rgba(25, 28, 34, 255);
    tokens.panel = nk_rgba(37, 41, 49, 255);
    tokens.panel_hover = nk_rgba(48, 54, 64, 255);
    tokens.canvas_background = nk_rgba(20, 23, 30, 255);

    tokens.text = nk_rgba(228, 233, 241, 255);
    tokens.text_secondary = nk_rgba(164, 172, 186, 255);
    tokens.text_disabled = nk_rgba(113, 121, 138, 255);

    tokens.border = nk_rgba(72, 79, 92, 255);
    tokens.border_hover = nk_rgba(103, 113, 130, 255);

    tokens.success = nk_rgba(99, 186, 120, 255);
    tokens.warning = nk_rgba(214, 167, 87, 255);
    tokens.error = nk_rgba(218, 102, 102, 255);

    tokens.row_height = 30.0f;
    tokens.panel_width = 292.0f;
    tokens.menu_height = 34.0f;
    tokens.status_height = 32.0f;
    tokens.tool_rail_width = 96.0f;

    tokens.padding = 5.0f;
    tokens.margin = 0.0f;
    tokens.gap = 0.0f;

    tokens.border_radius = 2.0f;

    tokens.enable_transitions = nk_true;
    tokens.transition_duration = 0.12f;
    return tokens;
}

/**
 * @brief Gets high contrast theme tokens.
 * @param void No parameters.
 * @return High contrast theme tokens.
 */
static UiThemeTokens ui_theme_high_contrast_tokens(void)
{
    UiThemeTokens tokens;

    tokens.primary = nk_rgba(0, 157, 255, 255);
    tokens.primary_hover = nk_rgba(79, 193, 255, 255);
    tokens.primary_active = nk_rgba(0, 121, 204, 255);

    tokens.background = nk_rgba(12, 12, 12, 255);
    tokens.panel = nk_rgba(0, 0, 0, 255);
    tokens.panel_hover = nk_rgba(24, 24, 24, 255);
    tokens.canvas_background = nk_rgba(8, 8, 8, 255);

    tokens.text = nk_rgba(255, 255, 255, 255);
    tokens.text_secondary = nk_rgba(222, 222, 222, 255);
    tokens.text_disabled = nk_rgba(148, 148, 148, 255);

    tokens.border = nk_rgba(255, 255, 255, 210);
    tokens.border_hover = nk_rgba(255, 255, 255, 255);

    tokens.success = nk_rgba(0, 255, 136, 255);
    tokens.warning = nk_rgba(255, 204, 0, 255);
    tokens.error = nk_rgba(255, 95, 95, 255);

    tokens.row_height = 30.0f;
    tokens.panel_width = 292.0f;
    tokens.menu_height = 34.0f;
    tokens.status_height = 32.0f;
    tokens.tool_rail_width = 96.0f;

    tokens.padding = 5.0f;
    tokens.margin = 0.0f;
    tokens.gap = 0.0f;

    tokens.border_radius = 0.0f;

    tokens.enable_transitions = nk_true;
    tokens.transition_duration = 0.12f;
    return tokens;
}

/**
 * @brief Gets default theme tokens.
 * @param void No parameters.
 * @return Default theme tokens.
 */
UiThemeTokens ui_theme_default_tokens(void)
{
    return ui_theme_light_tokens();
}

/**
 * @brief Gets the count of built-in themes.
 * @param void No parameters.
 * @return Built-in theme count.
 */
static int ui_theme_builtin_count(void)
{
    return (int)(sizeof(g_builtin_theme_descriptors) / sizeof(g_builtin_theme_descriptors[0]));
}

/**
 * @brief Gets built-in theme descriptor at index.
 * @param index Theme index.
 * @return Theme descriptor or NULL.
 */
static const UiThemeDescriptor* ui_theme_builtin_descriptor_at(int index)
{
    if (index < 0 || index >= ui_theme_builtin_count()) {
        return NULL;
    }
    return &g_builtin_theme_descriptors[index];
}

/**
 * @brief Gets built-in theme tokens at index.
 * @param index Theme index.
 * @return Theme tokens.
 */
static UiThemeTokens ui_theme_builtin_tokens_at(int index)
{
    switch (index) {
    case UI_THEME_DARK_PLUS_INDEX:
        return ui_theme_dark_plus_tokens();
    case UI_THEME_HIGH_CONTRAST_INDEX:
        return ui_theme_high_contrast_tokens();
    case UI_THEME_LIGHT_INDEX:
    default:
        return ui_theme_light_tokens();
    }
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
        if (strcmp(theme_id, g_builtin_theme_descriptors[i].id) == 0) {
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
    return g_builtin_theme_descriptors[UI_THEME_LIGHT_INDEX].id;
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
 * @brief Reads a text file.
 * @param path File path.
 * @return File contents or NULL.
 */
static char* ui_read_text_file(const char* path)
{
    FILE* file = NULL;
    char* buffer = NULL;
    long size = 0;
    size_t read_size = 0;

    if (!path || path[0] == '\0') {
        return NULL;
    }

    file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char*)malloc((size_t)size + 1u);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1u, (size_t)size, file);
    if (read_size != (size_t)size && ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[read_size] = '\0';
    fclose(file);
    return buffer;
}

/**
 * @brief Extracts a JSON string value.
 * @param text JSON text.
 * @param key Key name.
 * @param out_value Output string buffer.
 * @param out_value_size Buffer size.
 * @return 1 on success, 0 on failure.
 */
static int ui_extract_json_string_value(const char* text,
                                        const char* key,
                                        char* out_value,
                                        size_t out_value_size)
{
    char key_pattern[96];
    const char* cursor = NULL;
    int key_pattern_length = 0;
    size_t out_length = 0u;

    if (!text || !key || !out_value || out_value_size == 0u) {
        return 0;
    }

    key_pattern_length = snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    if (key_pattern_length <= 0 || (size_t)key_pattern_length >= sizeof(key_pattern)) {
        return 0;
    }

    cursor = strstr(text, key_pattern);
    if (!cursor) {
        return 0;
    }

    cursor += (size_t)key_pattern_length;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor != ':') {
        return 0;
    }

    cursor++;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor != '"') {
        return 0;
    }

    cursor++;
    while (*cursor != '\0') {
        if (*cursor == '"') {
            out_value[out_length] = '\0';
            return 1;
        }

        if (*cursor == '\\') {
            cursor++;
            if (*cursor == '\0') {
                return 0;
            }
        }

        if (out_length + 1u >= out_value_size) {
            return 0;
        }

        out_value[out_length++] = *cursor;
        cursor++;
    }

    return 0;
}

/**
 * @brief Extracts a JSON number value.
 * @param text JSON text.
 * @param key Key name.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int ui_extract_json_number_value(const char* text, const char* key, float* out_value)
{
    char key_pattern[96];
    const char* cursor = NULL;
    char* number_end = NULL;
    int key_pattern_length = 0;
    double number_value = 0.0;

    if (!text || !key || !out_value) {
        return 0;
    }

    key_pattern_length = snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    if (key_pattern_length <= 0 || (size_t)key_pattern_length >= sizeof(key_pattern)) {
        return 0;
    }

    cursor = strstr(text, key_pattern);
    if (!cursor) {
        return 0;
    }

    cursor += (size_t)key_pattern_length;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor != ':') {
        return 0;
    }

    cursor++;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }

    number_value = strtod(cursor, &number_end);
    if (number_end == cursor) {
        return 0;
    }

    *out_value = (float)number_value;
    return 1;
}

/**
 * @brief Extracts a JSON boolean value.
 * @param text JSON text.
 * @param key Key name.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int ui_extract_json_bool_value(const char* text, const char* key, int* out_value)
{
    char key_pattern[96];
    const char* cursor = NULL;
    int key_pattern_length = 0;

    if (!text || !key || !out_value) {
        return 0;
    }

    key_pattern_length = snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    if (key_pattern_length <= 0 || (size_t)key_pattern_length >= sizeof(key_pattern)) {
        return 0;
    }

    cursor = strstr(text, key_pattern);
    if (!cursor) {
        return 0;
    }

    cursor += (size_t)key_pattern_length;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor != ':') {
        return 0;
    }

    cursor++;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }

    if (strncmp(cursor, "true", 4) == 0) {
        *out_value = 1;
        return 1;
    }
    if (strncmp(cursor, "false", 5) == 0) {
        *out_value = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief Checks if a JSON key exists.
 * @param text JSON text.
 * @param key Key name.
 * @return 1 if exists, 0 otherwise.
 */
static int ui_json_key_exists(const char* text, const char* key)
{
    char key_pattern[96];
    int key_pattern_length = 0;

    if (!text || !key) {
        return 0;
    }

    key_pattern_length = snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    if (key_pattern_length <= 0 || (size_t)key_pattern_length >= sizeof(key_pattern)) {
        return 0;
    }
    return strstr(text, key_pattern) != NULL;
}

/**
 * @brief Converts a hex digit to integer value.
 * @param c Hex character.
 * @return Integer value or -1.
 */
static int ui_hex_digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

/**
 * @brief Parses a hex byte from text.
 * @param text Text containing hex byte.
 * @param out_value Output value pointer.
 * @return 1 on success, 0 on failure.
 */
static int ui_parse_hex_byte(const char* text, unsigned char* out_value)
{
    int hi = 0;
    int lo = 0;

    if (!text || !out_value) {
        return 0;
    }

    hi = ui_hex_digit_value(text[0]);
    lo = ui_hex_digit_value(text[1]);
    if (hi < 0 || lo < 0) {
        return 0;
    }

    *out_value = (unsigned char)((hi << 4) | lo);
    return 1;
}

/**
 * @brief Parses a hex color string.
 * @param hex_text Hex color string (e.g., "#rrggbb" or "#rrggbbaa").
 * @param out_color Output color pointer.
 * @return 1 on success, 0 on failure.
 */
static int ui_parse_hex_color(const char* hex_text, struct nk_color* out_color)
{
    size_t length = 0u;
    unsigned char r = 0u;
    unsigned char g = 0u;
    unsigned char b = 0u;
    unsigned char a = 255u;

    if (!hex_text || !out_color || hex_text[0] != '#') {
        return 0;
    }

    length = strlen(hex_text);
    if (length != 7u && length != 9u) {
        return 0;
    }

    if (!ui_parse_hex_byte(hex_text + 1, &r) ||
        !ui_parse_hex_byte(hex_text + 3, &g) ||
        !ui_parse_hex_byte(hex_text + 5, &b)) {
        return 0;
    }
    if (length == 9u && !ui_parse_hex_byte(hex_text + 7, &a)) {
        return 0;
    }

    out_color->r = (nk_byte)r;
    out_color->g = (nk_byte)g;
    out_color->b = (nk_byte)b;
    out_color->a = (nk_byte)a;
    return 1;
}

/**
 * @brief Checks if path has .json extension.
 * @param path File path.
 * @return 1 if has .json extension, 0 otherwise.
 */
static int ui_theme_path_has_json_extension(const char* path)
{
    size_t length = 0u;
    const char* ext = NULL;

    if (!path) {
        return 0;
    }

    length = strlen(path);
    if (length < 5u) {
        return 0;
    }

    ext = path + length - 5u;
    return (tolower((unsigned char)ext[0]) == '.') &&
           (tolower((unsigned char)ext[1]) == 'j') &&
           (tolower((unsigned char)ext[2]) == 's') &&
           (tolower((unsigned char)ext[3]) == 'o') &&
           (tolower((unsigned char)ext[4]) == 'n');
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
 * @brief Checks if theme ID is safe for JSON.
 * @param theme_id Theme ID to check.
 * @return 1 if safe, 0 otherwise.
 */
static int ui_theme_id_is_safe_for_json(const char* theme_id)
{
    const unsigned char* cursor = (const unsigned char*)theme_id;

    if (!theme_id || theme_id[0] == '\0') {
        return 0;
    }

    while (*cursor != '\0') {
        if (!(isalnum(*cursor) || *cursor == '-' || *cursor == '_' || *cursor == '.')) {
            return 0;
        }
        cursor++;
    }

    return 1;
}

/**
 * @brief Extracts filename from a path.
 * @param path Full file path.
 * @return Filename string.
 */
static const char* ui_theme_filename_from_path(const char* path)
{
    const char* last_slash = NULL;
    const char* last_backslash = NULL;

    if (!path) {
        return NULL;
    }

    last_slash = strrchr(path, '/');
    last_backslash = strrchr(path, '\\');
    if (!last_slash && !last_backslash) {
        return path;
    }
    if (!last_slash) {
        return last_backslash + 1;
    }
    if (!last_backslash) {
        return last_slash + 1;
    }
    return (last_slash > last_backslash) ? (last_slash + 1) : (last_backslash + 1);
}

/**
 * @brief Extracts theme ID from file path.
 * @param path File path.
 * @param out_id Output ID buffer.
 * @param out_id_size Buffer size.
 * @return 1 on success, 0 on failure.
 */
static int ui_theme_id_from_path(const char* path, char* out_id, size_t out_id_size)
{
    const char* filename = NULL;
    size_t length = 0u;
    size_t write_length = 0u;

    if (!path || !out_id || out_id_size == 0u) {
        return 0;
    }

    filename = ui_theme_filename_from_path(path);
    if (!filename || filename[0] == '\0') {
        return 0;
    }

    length = strlen(filename);
    if (length > 5u && ui_theme_path_has_json_extension(filename)) {
        length -= 5u;
    }
    if (length == 0u) {
        return 0;
    }

    write_length = (length < (out_id_size - 1u)) ? length : (out_id_size - 1u);
    memcpy(out_id, filename, write_length);
    out_id[write_length] = '\0';
    return write_length > 0u;
}

/**
 * @brief Applies color overrides from JSON text.
 * @param source_path Source file path (for logging).
 * @param text JSON text.
 * @param tokens Theme tokens to update.
 * @return None.
 */
static void ui_theme_apply_color_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
{
    typedef struct UiThemeColorField {
        const char* key;
        size_t offset;
    } UiThemeColorField;

    static const UiThemeColorField fields[] = {
        {"primary", offsetof(UiThemeTokens, primary)},
        {"primary_hover", offsetof(UiThemeTokens, primary_hover)},
        {"primary_active", offsetof(UiThemeTokens, primary_active)},
        {"background", offsetof(UiThemeTokens, background)},
        {"panel", offsetof(UiThemeTokens, panel)},
        {"panel_hover", offsetof(UiThemeTokens, panel_hover)},
        {"canvas_background", offsetof(UiThemeTokens, canvas_background)},
        {"text", offsetof(UiThemeTokens, text)},
        {"text_secondary", offsetof(UiThemeTokens, text_secondary)},
        {"text_disabled", offsetof(UiThemeTokens, text_disabled)},
        {"border", offsetof(UiThemeTokens, border)},
        {"border_hover", offsetof(UiThemeTokens, border_hover)},
        {"success", offsetof(UiThemeTokens, success)},
        {"warning", offsetof(UiThemeTokens, warning)},
        {"error", offsetof(UiThemeTokens, error)},
    };

    if (!text || !tokens) {
        return;
    }

    for (size_t i = 0; i < (sizeof(fields) / sizeof(fields[0])); ++i) {
        char color_text[16];
        struct nk_color parsed;

        if (!ui_extract_json_string_value(text, fields[i].key, color_text, sizeof(color_text))) {
            continue;
        }
        if (!ui_parse_hex_color(color_text, &parsed)) {
            LOG_WARN("[theme][parse][color] invalid color format path=%s key=%s value=%s",
                     source_path ? source_path : "(unknown)",
                     fields[i].key,
                     color_text);
            continue;
        }

        *(struct nk_color*)((char*)tokens + fields[i].offset) = parsed;
    }
}

/**
 * @brief Applies float overrides from JSON text.
 * @param source_path Source file path (for logging).
 * @param text JSON text.
 * @param tokens Theme tokens to update.
 * @return None.
 */
static void ui_theme_apply_float_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
{
    typedef struct UiThemeFloatField {
        const char* key;
        size_t offset;
    } UiThemeFloatField;

    static const UiThemeFloatField fields[] = {
        {"row_height", offsetof(UiThemeTokens, row_height)},
        {"panel_width", offsetof(UiThemeTokens, panel_width)},
        {"menu_height", offsetof(UiThemeTokens, menu_height)},
        {"status_height", offsetof(UiThemeTokens, status_height)},
        {"tool_rail_width", offsetof(UiThemeTokens, tool_rail_width)},
        {"padding", offsetof(UiThemeTokens, padding)},
        {"margin", offsetof(UiThemeTokens, margin)},
        {"gap", offsetof(UiThemeTokens, gap)},
        {"border_radius", offsetof(UiThemeTokens, border_radius)},
        {"transition_duration", offsetof(UiThemeTokens, transition_duration)},
    };

    if (!text || !tokens) {
        return;
    }

    for (size_t i = 0; i < (sizeof(fields) / sizeof(fields[0])); ++i) {
        float value = 0.0f;
        if (!ui_extract_json_number_value(text, fields[i].key, &value)) {
            if (ui_json_key_exists(text, fields[i].key)) {
                LOG_WARN("[theme][parse][float] invalid number path=%s key=%s",
                         source_path ? source_path : "(unknown)",
                         fields[i].key);
            }
            continue;
        }
        *(float*)((char*)tokens + fields[i].offset) = value;
    }
}

/**
 * @brief Applies boolean overrides from JSON text.
 * @param source_path Source file path (for logging).
 * @param text JSON text.
 * @param tokens Theme tokens to update.
 * @return None.
 */
static void ui_theme_apply_bool_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
{
    int bool_value = 0;

    if (!text || !tokens) {
        return;
    }

    if (ui_extract_json_bool_value(text, "enable_transitions", &bool_value)) {
        tokens->enable_transitions = bool_value ? nk_true : nk_false;
    } else if (ui_json_key_exists(text, "enable_transitions")) {
        LOG_WARN("[theme][parse][bool] invalid bool path=%s key=enable_transitions",
                 source_path ? source_path : "(unknown)");
    }
}

/**
 * @brief Clamps a value to a range.
 * @param value Value to clamp.
 * @param min_value Minimum value.
 * @param max_value Maximum value.
 * @return Clamped value.
 */
static float ui_theme_clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

/**
 * @brief Clamps theme token values to valid ranges.
 * @param tokens Theme tokens to clamp.
 * @return None.
 */
static void ui_theme_clamp_tokens(UiThemeTokens* tokens)
{
    if (!tokens) {
        return;
    }

    tokens->row_height = ui_theme_clamp(tokens->row_height, 18.0f, 64.0f);
    tokens->panel_width = ui_theme_clamp(tokens->panel_width, 160.0f, 720.0f);
    tokens->menu_height = ui_theme_clamp(tokens->menu_height, 20.0f, 80.0f);
    tokens->status_height = ui_theme_clamp(tokens->status_height, 18.0f, 64.0f);
    tokens->tool_rail_width = ui_theme_clamp(tokens->tool_rail_width, 48.0f, 240.0f);
    tokens->transition_duration = ui_theme_clamp(tokens->transition_duration, 0.0f, 2.0f);
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

    text = ui_read_text_file(path);
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
                             "Failed parsing theme file: %s",
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
            if (!ui_theme_path_has_json_extension(entry->d_name)) {
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
                             "Failed parsing theme file: %s",
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
            if (!ui_theme_path_has_json_extension(entry->d_name)) {
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
    text = ui_read_text_file(path);
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
 * @brief Duplicates path with suffix appended.
 * @param path Original path.
 * @param suffix Suffix to append.
 * @return Duplicated path or NULL.
 */
static char* ui_duplicate_path_with_suffix(const char* path, const char* suffix)
{
    size_t path_length = 0u;
    size_t suffix_length = 0u;
    char* result = NULL;

    if (!path || !suffix) {
        return NULL;
    }

    path_length = strlen(path);
    suffix_length = strlen(suffix);
    result = (char*)malloc(path_length + suffix_length + 1u);
    if (!result) {
        return NULL;
    }

    memcpy(result, path, path_length);
    memcpy(result + path_length, suffix, suffix_length);
    result[path_length + suffix_length] = '\0';
    return result;
}

/**
 * @brief Replaces target file with temp file atomically.
 * @param temp_path Temp file path.
 * @param target_path Target file path.
 * @return 1 on success, 0 on failure.
 */
static int ui_replace_file_with_temp(const char* temp_path, const char* target_path)
{
    if (!temp_path || !target_path) {
        return 0;
    }

#ifdef _WIN32
    if (MoveFileExA(temp_path,
                    target_path,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return 1;
    }
    remove(temp_path);
    return 0;
#else
    if (rename(temp_path, target_path) == 0) {
        return 1;
    }
    remove(temp_path);
    return 0;
#endif
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

    temp_path = ui_duplicate_path_with_suffix(path, ".tmp");
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

    if (!ui_replace_file_with_temp(temp_path, path)) {
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
