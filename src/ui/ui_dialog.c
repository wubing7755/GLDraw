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

UiDialogResult ui_dialog_show(struct nk_context* ctx,
                              const UiDialogState* dialog,
                              int window_width,
                              int window_height)
{
    struct nk_style_item blocker_background;
    struct nk_rect bounds;
    float dialog_width = 0.0f;
    float dialog_height = 0.0f;
    UiDialogResult result = UI_DIALOG_RESULT_NONE;
    int i = 0;

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

    if (dialog->modal) {
        blocker_background = nk_style_item_color(nk_rgba(0, 0, 0, 96));
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, blocker_background);
        if (nk_begin(ctx,
                     "##modal_blocker##",
                     nk_rect(0.0f, 0.0f, (float)window_width, (float)window_height),
                     NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            nk_layout_row_dynamic(ctx, 1.0f, 1);
            nk_label(ctx, "", NK_TEXT_LEFT);
        }
        nk_end(ctx);
        nk_style_pop_style_item(ctx);
    }

    if (nk_begin(ctx,
                 dialog->title[0] != '\0' ? dialog->title : "Dialog",
                 bounds,
                 NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {
        if (dialog->message[0] != '\0') {
            nk_layout_row_dynamic(ctx, 48.0f, 1);
            nk_label_wrap(ctx, dialog->message);
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

    return result;
}
