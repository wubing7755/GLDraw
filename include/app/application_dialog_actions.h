/**
 * @file application_dialog_actions.h
 * @brief Application-level dialog text and picker outcome helpers.
 */
#ifndef GLDRAW_APP_APPLICATION_DIALOG_ACTIONS_H
#define GLDRAW_APP_APPLICATION_DIALOG_ACTIONS_H

#include <platform/file_dialog.h>

typedef struct Application Application;

void application_dialog_actions_update_save_as_message(Application* app,
                                                       const char* error_text);
void application_dialog_actions_report_startup_load_failure(Application* app,
                                                            const char* path);
void application_dialog_actions_report_open_cancelled(Application* app);
void application_dialog_actions_report_open_picker_error(Application* app);
void application_dialog_actions_report_export_cancelled(Application* app);
void application_dialog_actions_report_export_picker_error(Application* app);

#endif /* GLDRAW_APP_APPLICATION_DIALOG_ACTIONS_H */
