#include <nuklear/nuklear.h>

#include "ui_theme_internal.h"

#include <stddef.h>

static const UiThemeDescriptor g_builtin_theme_descriptors[] = {
    {"gldraw-light", "GLDraw Light"},
    {"gldraw-dark-plus", "GLDraw Dark+"},
    {"gldraw-high-contrast", "GLDraw High Contrast"},
};

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

UiThemeTokens ui_theme_default_tokens(void)
{
    return ui_theme_light_tokens();
}

int ui_theme_builtin_count(void)
{
    return (int)(sizeof(g_builtin_theme_descriptors) / sizeof(g_builtin_theme_descriptors[0]));
}

const UiThemeDescriptor* ui_theme_builtin_descriptor_at(int index)
{
    if (index < 0 || index >= ui_theme_builtin_count()) {
        return NULL;
    }
    return &g_builtin_theme_descriptors[index];
}

UiThemeTokens ui_theme_builtin_tokens_at(int index)
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
