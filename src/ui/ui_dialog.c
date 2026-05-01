/**
 * @file ui_dialog.c
 * @brief Reusable modal dialog rendering.
 */
#include <ui/ui_dialog.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include "nuklear/nuklear.h"

#include <stdio.h>
#include <string.h>

static int ui_dialog_supports_backdrop_close(const UiDialogState* dialog)
{
    (void)dialog;
    return 0;
}

static int ui_dialog_supports_header_close(const UiDialogState* dialog)
{
    (void)dialog;
    return 0;
}

static void ui_dialog_render_message_lines(struct nk_context* ctx,
                                           const UiDialogState* dialog)
{
    char local[sizeof(dialog->message)];
    char* line = NULL;

    if (!ctx || !dialog || dialog->message[0] == '\0') {
        return;
    }

    snprintf(local, sizeof(local), "%s", dialog->message);
    line = strtok(local, "\n");
    while (line) {
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_label(ctx, line, NK_TEXT_LEFT);
        line = strtok(NULL, "\n");
    }
}

UiDialogResult ui_dialog_show(struct nk_context* ctx,
                              UiDialogState* dialog,
                              int window_width,
                              int window_height)
{
    struct nk_style_item blocker_background;
    struct nk_rect full_bounds;
    struct nk_rect bounds;
    float dialog_width = 0.0f;
    float dialog_height = 0.0f;
    UiDialogResult result = UI_DIALOG_RESULT_NONE;
    int i = 0;
    nk_flags window_flags = 0;
    const char* window_title = NULL;

    if (!ctx || !dialog || dialog->kind == UI_DIALOG_NONE) {
        return UI_DIALOG_RESULT_NONE;
    }

    dialog_width = (dialog->width > 0.0f) ? dialog->width : 360.0f;
    dialog_height = (dialog->height > 0.0f) ? dialog->height : 170.0f;
    if ((float)window_width < dialog_width + 24.0f) {
        dialog_width = (float)window_width - 24.0f;
    }
    if ((float)window_height < dialog_height + 24.0f) {
        dialog_height = (float)window_height - 24.0f;
    }
    if (dialog_width < 240.0f) {
        dialog_width = 240.0f;
    }
    if (dialog_height < 140.0f) {
        dialog_height = 140.0f;
    }

    bounds = nk_rect(((float)window_width - dialog_width) * 0.5f,
                     ((float)window_height - dialog_height) * 0.5f,
                     dialog_width,
                     dialog_height);
    full_bounds = nk_rect(0.0f, 0.0f, (float)window_width, (float)window_height);
    window_title = dialog->title[0] != '\0' ? dialog->title : "Dialog";

    if (dialog->modal) {
        blocker_background = nk_style_item_color(nk_rgba(0, 0, 0, 96));
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, blocker_background);
        if (nk_begin(ctx,
                     "##modal_blocker##",
                     full_bounds,
                     NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            nk_layout_row_dynamic(ctx, 1.0f, 1);
            nk_label(ctx, "", NK_TEXT_LEFT);
        }
        nk_end(ctx);
        nk_style_pop_style_item(ctx);

        if (ui_dialog_supports_backdrop_close(dialog) &&
            nk_input_is_mouse_click_in_rect(&ctx->input, NK_BUTTON_LEFT, full_bounds) &&
            !nk_input_is_mouse_hovering_rect(&ctx->input, bounds)) {
            result = UI_DIALOG_RESULT_CANCEL;
        }
    }

    window_flags = NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE;
    if (ui_dialog_supports_header_close(dialog)) {
        window_flags |= NK_WINDOW_CLOSABLE;
    }

    if (nk_begin(ctx, window_title, bounds, window_flags)) {
        ui_dialog_render_message_lines(ctx, dialog);
        if (dialog->kind == UI_DIALOG_SAVE_AS) {
            nk_layout_row_dynamic(ctx, 20.0f, 1);
            nk_label(ctx, "Filename", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 28.0f, 1);
            nk_edit_string_zero_terminated(ctx,
                                           NK_EDIT_FIELD,
                                           dialog->payload.text,
                                           (int)sizeof(dialog->payload.text),
                                           nk_filter_default);
        }
        nk_layout_row_dynamic(ctx, 12.0f, 1);
        nk_label(ctx, "", NK_TEXT_LEFT);
        if (dialog->button_count > 0) {
            nk_layout_row_dynamic(ctx, 34.0f, dialog->button_count);
            for (i = 0; i < dialog->button_count; ++i) {
                if (nk_button_label(ctx, dialog->buttons[i].label)) {
                    result = dialog->buttons[i].result;
                }
            }
        }
    }
    nk_end(ctx);

    if (ui_dialog_supports_header_close(dialog) &&
        nk_window_is_closed(ctx, window_title)) {
        result = UI_DIALOG_RESULT_CANCEL;
    }

    return result;
}
