#ifndef GLDRAW_UI_UI_THEME_H
#define GLDRAW_UI_UI_THEME_H

#include <stddef.h>

#ifndef NK_NUKLEAR_H_
typedef unsigned char nk_byte;
typedef int nk_bool;
struct nk_color { nk_byte r, g, b, a; };
struct nk_context;
#endif

typedef struct UiThemeTokens {
    /* Primary */
    struct nk_color primary;
    struct nk_color primary_hover;
    struct nk_color primary_active;

    /* Background */
    struct nk_color background;
    struct nk_color panel;
    struct nk_color panel_hover;
    struct nk_color canvas_background;

    /* Text */
    struct nk_color text;
    struct nk_color text_secondary;
    struct nk_color text_disabled;

    /* Border */
    struct nk_color border;
    struct nk_color border_hover;

    /* Status */
    struct nk_color success;
    struct nk_color warning;
    struct nk_color error;

    /* Size tokens */
    float row_height;
    float panel_width;
    float menu_height;
    float status_height;
    float tool_rail_width;

    /* Spacing tokens */
    float padding;
    float margin;
    float gap;

    /* Corners */
    float border_radius;

    /* Transitions */
    nk_bool enable_transitions;
    float transition_duration;
} UiThemeTokens;

typedef struct UiThemeDescriptor {
    const char* id;
    const char* label;
} UiThemeDescriptor;

UiThemeTokens ui_theme_default_tokens(void);
UiThemeTokens ui_theme_tokens_for_id(const char* theme_id);
int ui_theme_count(void);
const UiThemeDescriptor* ui_theme_descriptor_at(int index);
int ui_theme_index_of_id(const char* theme_id);
const char* ui_theme_default_id(void);
int ui_theme_reload_external(const char* directory_path);
unsigned long long ui_theme_external_signature(const char* directory_path);
int ui_theme_load_selected_id(const char* path, char* out_theme_id, size_t out_theme_id_size);
int ui_theme_save_selected_id(const char* path, const char* theme_id);
void ui_theme_apply(struct nk_context* ctx, const UiThemeTokens* tokens);

#endif /* GLDRAW_UI_UI_THEME_H */
