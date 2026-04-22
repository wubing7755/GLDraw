/**
 * @file ui_theme.h
 * @brief UI theme tokens and load/apply interface.
 */
#ifndef GLDRAW_UI_UI_THEME_H
#define GLDRAW_UI_UI_THEME_H

#include <stddef.h>

#ifndef NK_NUKLEAR_H_
typedef unsigned char nk_byte;
typedef int nk_bool;
struct nk_color { nk_byte r, g, b, a; };
struct nk_context;
#endif

/**
 * @struct UiThemeTokens
 * @brief Complete set of UI theme tokens.
 *
 * @member primary Primary color.
 * @member primary_hover Primary color hover state.
 * @member primary_active Primary color active state.
 * @member background Global background color.
 * @member panel Panel background color.
 * @member panel_hover Panel hover color.
 * @member canvas_background Canvas background color.
 * @member text Primary text color.
 * @member text_secondary Secondary text color.
 * @member text_disabled Disabled text color.
 * @member border Border color.
 * @member border_hover Border hover color.
 * @member success Success state color.
 * @member warning Warning state color.
 * @member error Error state color.
 * @member row_height Row height.
 * @member panel_width Panel width.
 * @member menu_height Menu bar height.
 * @member status_height Status bar height.
 * @member tool_rail_width Tool rail width.
 * @member padding Inner padding.
 * @member margin Outer margin.
 * @member gap Spacing gap.
 * @member border_radius Corner radius.
 * @member enable_transitions Whether to enable transitions.
 * @member transition_duration Transition animation duration.
 */
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

/**
 * @struct UiThemeDescriptor
 * @brief Theme metadata.
 *
 * @member id Unique theme ID.
 * @member label Theme display name.
 */
typedef struct UiThemeDescriptor {
    const char* id;
    const char* label;
} UiThemeDescriptor;

/**
 * @brief Get the default built-in theme tokens.
 * @return Default theme tokens.
 */
UiThemeTokens ui_theme_default_tokens(void);

/**
 * @brief Get theme tokens for a given theme ID.
 * @param theme_id Theme ID.
 * @return Matching theme tokens; returns default theme if not found.
 */
UiThemeTokens ui_theme_tokens_for_id(const char* theme_id);

/**
 * @brief Get the total number of available themes (built-in + external).
 * @return Total theme count.
 */
int ui_theme_count(void);

/**
 * @brief Get a theme descriptor by index.
 * @param index Theme index.
 * @return Descriptor pointer if index is valid, `NULL` otherwise.
 */
const UiThemeDescriptor* ui_theme_descriptor_at(int index);

/**
 * @brief Look up the index for a given theme ID.
 * @param theme_id Theme ID.
 * @return Index if found, `-1` if not found.
 */
int ui_theme_index_of_id(const char* theme_id);

/**
 * @brief Get the default theme ID.
 * @return Default theme ID string.
 */
const char* ui_theme_default_id(void);

/**
 * @brief Reload external themes from a given directory.
 * @param directory_path Theme directory path.
 * @return Number of external themes successfully loaded.
 */
int ui_theme_reload_external(const char* directory_path);

/**
 * @brief Compute the external theme directory signature.
 * @param directory_path Theme directory path.
 * @return Directory signature value (used for change detection).
 */
unsigned long long ui_theme_external_signature(const char* directory_path);

/**
 * @brief Get the error message from the last theme reload.
 * @return Error string; returns an empty string if no error occurred.
 */
const char* ui_theme_last_reload_error(void);

/**
 * @brief Load the current theme ID from a settings file.
 * @param path Settings file path.
 * @param out_theme_id Theme ID output buffer.
 * @param out_theme_id_size Output buffer size.
 * @return Non-zero on success, zero on failure.
 */
int ui_theme_load_selected_id(const char* path, char* out_theme_id, size_t out_theme_id_size);

/**
 * @brief Save the current theme ID to a settings file.
 * @param path Settings file path.
 * @param theme_id Theme ID.
 * @return Non-zero on success, zero on failure.
 */
int ui_theme_save_selected_id(const char* path, const char* theme_id);

/**
 * @brief Apply theme tokens to the Nuklear stylesheet.
 * @param ctx Nuklear context.
 * @param tokens Theme tokens.
 * @return No return value.
 */
void ui_theme_apply(struct nk_context* ctx, const UiThemeTokens* tokens);

#endif /* GLDRAW_UI_UI_THEME_H */
