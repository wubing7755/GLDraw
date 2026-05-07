/**
 * @file application_file_actions.h
 * @brief Application-owned file workflow helpers.
 */
#ifndef GLDRAW_APP_APPLICATION_FILE_ACTIONS_H
#define GLDRAW_APP_APPLICATION_FILE_ACTIONS_H

typedef struct Application Application;

int application_new_document(Application* app);
int application_save_document_to_path(Application* app, const char* path);
int application_save_document(Application* app);
int application_save_as_document(Application* app);
int application_load_document_from_path(Application* app, const char* path);
int application_load_document(Application* app);
int application_open_document_with_picker(Application* app);
int application_request_export_png(Application* app);
void application_flush_pending_export_png(Application* app);
void application_open_startup_document(Application* app);

#endif /* GLDRAW_APP_APPLICATION_FILE_ACTIONS_H */
