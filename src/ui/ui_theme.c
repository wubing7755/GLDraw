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
#include <ui/ui_theme.h>

#include "ui_theme_internal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * @brief Gets the total theme count.
 * @param void No parameters.
 * @return Total theme count.
 */
int ui_theme_count(void)
{
    return ui_theme_builtin_count() + ui_theme_custom_count();
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
    return ui_theme_custom_descriptor_at(custom_index);
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
    return ui_theme_custom_tokens_at(theme_index - builtin_count);
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
