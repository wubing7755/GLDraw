/**
 * @file workspace_service.h
 * @brief Document lifecycle operations: new, open, save, export.
 *
 * These functions operate on the Workspace and abstract document
 * manipulation away from the application shell. They ensure all
 * mutations go through the CommandExecutor.
 */
#ifndef GLDRAW_APP_WORKSPACE_SERVICE_H
#define GLDRAW_APP_WORKSPACE_SERVICE_H

#include <app/workspace.h>

/**
 * Initialize the workspace and all core subsystems (document, command executor,
 * canvas, tool controller, keymap, and builtin object type registration).
 * Must be called once before any workspace operations.
 * Returns non-zero on success, zero on failure.
 */
int workspace_init(Workspace* workspace, RectF viewport, const char* keymap_path);

/**
 * Shut down the workspace and release all core subsystem resources in reverse
 * ownership order. Safe no-op when workspace is NULL.
 */
void workspace_shutdown(Workspace* workspace);

/** Reset the workspace to a new empty document state. */
int workspace_service_new_document(Workspace* workspace);

/**
 * Save the current document to the given path.
 * Returns non-zero on success, zero on failure.
 */
int workspace_service_save_to_path(Workspace* workspace, const char* path);

/**
 * Save the current document to its current path.
 * Returns non-zero on success, zero on failure.
 */
int workspace_service_save(Workspace* workspace);

/**
 * Load a document from the given path into the workspace.
 * Returns non-zero on success, zero on failure.
 */
int workspace_service_load_from_path(Workspace* workspace, const char* path);

/**
 * Load the document at the workspace's current path.
 * Returns non-zero on success, zero on failure.
 */
int workspace_service_load(Workspace* workspace);

/**
 * Check whether a file exists at the given path.
 * Returns 1 if the file exists, 0 otherwise.
 */
int workspace_service_file_exists(const char* path);

/**
 * Get the current document path from the workspace.
 * Returns the path string (may be a default if none set).
 */
const char* workspace_service_document_path(const Workspace* workspace);

/**
 * Copy the given path into the workspace's current document path buffer.
 */
void workspace_service_set_document_path(Workspace* workspace, const char* path);

#endif /* GLDRAW_APP_WORKSPACE_SERVICE_H */
