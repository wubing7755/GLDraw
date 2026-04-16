#include <nuklear/nuklear.h>
#include <ui/ui_theme.h>

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

UiThemeTokens ui_theme_default_tokens(void)
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
