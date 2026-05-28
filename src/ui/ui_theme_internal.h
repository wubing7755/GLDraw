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

#endif /* GLDRAW_UI_THEME_INTERNAL_H */
