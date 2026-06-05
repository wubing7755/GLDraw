#include <nuklear/nuklear.h>
#include <base/log.h>
#include <base/path_utils.h>

#include "ui_theme_internal.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ui_extract_json_string_value(const char* text,
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

static int ui_json_scalar_has_delimiter(const char* cursor)
{
    if (!cursor) {
        return 0;
    }

    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }

    return *cursor == '\0' || *cursor == ',' || *cursor == '}' || *cursor == ']';
}

int ui_extract_json_number_value(const char* text, const char* key, float* out_value)
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
    if (!ui_json_scalar_has_delimiter(number_end)) {
        return 0;
    }

    *out_value = (float)number_value;
    return 1;
}

int ui_extract_json_bool_value(const char* text, const char* key, int* out_value)
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

    if (strncmp(cursor, "true", 4) == 0 && ui_json_scalar_has_delimiter(cursor + 4)) {
        *out_value = 1;
        return 1;
    }
    if (strncmp(cursor, "false", 5) == 0 && ui_json_scalar_has_delimiter(cursor + 5)) {
        *out_value = 0;
        return 1;
    }
    return 0;
}

int ui_json_key_exists(const char* text, const char* key)
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

int ui_parse_hex_color(const char* hex_text, struct nk_color* out_color)
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

int ui_theme_id_is_safe_for_json(const char* theme_id)
{
    const unsigned char* cursor = (const unsigned char*)theme_id;

    if (!theme_id || theme_id[0] == '\0') {
        return 0;
    }

    while (*cursor != '\0') {
        if (!(isalnum((unsigned char)*cursor) || *cursor == '-' ||
              *cursor == '_' || *cursor == '.')) {
            return 0;
        }
        cursor++;
    }

    return 1;
}

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

int ui_theme_id_from_path(const char* path, char* out_id, size_t out_id_size)
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
    if (length > 5u && path_utils_has_extension(filename, ".json")) {
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

void ui_theme_apply_color_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
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

        memcpy((char*)tokens + fields[i].offset, &parsed, sizeof(parsed));
    }
}

void ui_theme_apply_float_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
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
        memcpy((char*)tokens + fields[i].offset, &value, sizeof(value));
    }
}

void ui_theme_apply_bool_overrides(const char* source_path, const char* text, UiThemeTokens* tokens)
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

void ui_theme_clamp_tokens(UiThemeTokens* tokens)
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
