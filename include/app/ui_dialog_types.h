/**
 * @file ui_dialog_types.h
 * @brief Shared UI dialog request and result value types.
 */
#ifndef GLDRAW_APP_UI_DIALOG_TYPES_H
#define GLDRAW_APP_UI_DIALOG_TYPES_H

typedef enum UiRequestType {
    UI_REQUEST_NONE = 0,
    UI_REQUEST_DIALOG
} UiRequestType;

typedef enum UiDialogKind {
    UI_DIALOG_NONE = 0,
    UI_DIALOG_CONFIRM_UNSAVED,
    UI_DIALOG_SHORTCUTS,
    UI_DIALOG_INFO,
    UI_DIALOG_SAVE_AS
} UiDialogKind;

typedef enum UiDialogResult {
    UI_DIALOG_RESULT_NONE = 0,
    UI_DIALOG_RESULT_PRIMARY,
    UI_DIALOG_RESULT_SECONDARY,
    UI_DIALOG_RESULT_CANCEL
} UiDialogResult;

typedef struct UiDialogPayload {
    int int_values[4];
    char text[1024];
} UiDialogPayload;

typedef struct UiDialogButtonSpec {
    char label[24];
    UiDialogResult result;
    int is_default;
} UiDialogButtonSpec;

typedef struct UiDialogState {
    UiDialogKind kind;
    char title[64];
    char message[1024];
    UiDialogPayload payload;
    UiDialogButtonSpec buttons[3];
    int button_count;
    float width;
    float height;
    int modal;
} UiDialogState;

#endif /* GLDRAW_APP_UI_DIALOG_TYPES_H */
