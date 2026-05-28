/**
 * @file ui_theme.c
 * @brief Public theme registry orchestration.
 *
 * Role in project:
 * - Combines built-in and external theme registries behind the public theme API.
 * - Leaves parsing, external loading, settings, and Nuklear application to focused modules.
 *
 * Module relationships:
 * - Consumed by `ui_system`/`ui_menubar`.
 * - Uses private helpers declared in `ui_theme_internal.h`.
 */
#include <ui/ui_theme.h>

#include "ui_theme_internal.h"

#include <string.h>

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
