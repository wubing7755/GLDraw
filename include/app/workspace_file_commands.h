/**
 * @file workspace_file_commands.h
 * @brief Workspace-owned file command execution helpers.
 */
#ifndef GLDRAW_APP_WORKSPACE_FILE_COMMANDS_H
#define GLDRAW_APP_WORKSPACE_FILE_COMMANDS_H

#include <app/workspace_service_types.h>

/** Request a new document action through the workspace action policy. */
int workspace_file_new_document(Workspace* workspace);
/** Request an open document action through the workspace action policy. */
int workspace_file_open_document(Workspace* workspace);
/** Save the current document through the registered workspace service. */
int workspace_file_save_document(Workspace* workspace);
/** Open the workspace-owned Save As dialog. */
int workspace_file_save_document_as(Workspace* workspace);
/** Export the current document as PNG through the registered workspace service. */
int workspace_file_export_png(Workspace* workspace);
/** Request application exit through the workspace action policy. */
int workspace_file_exit_application(Workspace* workspace);

#endif /* GLDRAW_APP_WORKSPACE_FILE_COMMANDS_H */
