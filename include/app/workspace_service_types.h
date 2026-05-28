/**
 * @file workspace_service_types.h
 * @brief Workspace service and application action value types.
 */
#ifndef GLDRAW_APP_WORKSPACE_SERVICE_TYPES_H
#define GLDRAW_APP_WORKSPACE_SERVICE_TYPES_H

typedef struct Workspace Workspace;

typedef int (*WorkspaceCommandFn)(Workspace* workspace, void* user_data);

typedef enum WorkspaceActionType {
    WORKSPACE_ACTION_NONE = 0,
    WORKSPACE_ACTION_NEW_DOCUMENT,
    WORKSPACE_ACTION_OPEN_DOCUMENT,
    WORKSPACE_ACTION_EXIT_APPLICATION
} WorkspaceActionType;

typedef enum WorkspaceServiceType {
    WORKSPACE_SERVICE_SAVE_DOCUMENT = 0,
    WORKSPACE_SERVICE_SAVE_AS_DOCUMENT,
    WORKSPACE_SERVICE_EXPORT_PNG,
    WORKSPACE_SERVICE_LOAD_DOCUMENT,
    WORKSPACE_SERVICE_EXECUTE_ACTION
} WorkspaceServiceType;

typedef int (*WorkspaceActionExecutorFn)(Workspace* workspace,
                                         WorkspaceActionType action,
                                         void* user_data);

#endif /* GLDRAW_APP_WORKSPACE_SERVICE_TYPES_H */
