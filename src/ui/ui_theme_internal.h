#ifndef GLDRAW_UI_THEME_INTERNAL_H
#define GLDRAW_UI_THEME_INTERNAL_H

#include <ui/ui_theme.h>

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

int ui_theme_builtin_count(void);
const UiThemeDescriptor* ui_theme_builtin_descriptor_at(int index);
UiThemeTokens ui_theme_builtin_tokens_at(int index);

int ui_extract_json_string_value(const char* text,
                                 const char* key,
                                 char* out_value,
                                 size_t out_value_size);
int ui_extract_json_number_value(const char* text, const char* key, float* out_value);
int ui_extract_json_bool_value(const char* text, const char* key, int* out_value);
int ui_json_key_exists(const char* text, const char* key);
int ui_parse_hex_color(const char* hex_text, struct nk_color* out_color);
int ui_theme_id_is_safe_for_json(const char* theme_id);
int ui_theme_id_from_path(const char* path, char* out_id, size_t out_id_size);
void ui_theme_apply_color_overrides(const char* source_path, const char* text, UiThemeTokens* tokens);
void ui_theme_apply_float_overrides(const char* source_path, const char* text, UiThemeTokens* tokens);
void ui_theme_apply_bool_overrides(const char* source_path, const char* text, UiThemeTokens* tokens);
void ui_theme_clamp_tokens(UiThemeTokens* tokens);

#endif /* GLDRAW_UI_THEME_INTERNAL_H */
