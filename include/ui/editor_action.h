#ifndef GLDRAW_UI_EDITOR_ACTION_H
#define GLDRAW_UI_EDITOR_ACTION_H

#include <app/command_types.h>
#include <app/workspace.h>
#include <base/types.h>

typedef enum EditorActionType {
    EDITOR_ACTION_NONE = 0,
    EDITOR_ACTION_EXECUTE_COMMAND,
    EDITOR_ACTION_SET_TOOL,
    EDITOR_ACTION_MODIFY_PROPERTY,
    EDITOR_ACTION_SET_ACTIVE_LAYER,
    EDITOR_ACTION_SET_LAYER_VISIBILITY,
    EDITOR_ACTION_SET_LAYER_LOCKED,
    EDITOR_ACTION_RENAME_LAYER,
    EDITOR_ACTION_MOVE_LAYER,
    EDITOR_ACTION_CREATE_LAYER,
    EDITOR_ACTION_RESOLVE_DIALOG,
    EDITOR_ACTION_SET_STATUS_MESSAGE
} EditorActionType;

#define EDITOR_ACTION_ID_CAPACITY 64
#define EDITOR_ACTION_PROPERTY_CAPACITY 32
#define EDITOR_ACTION_TEXT_CAPACITY 1024
#define EDITOR_ACTION_STATUS_CAPACITY 256

typedef struct EditorAction {
    EditorActionType type;
    union {
        struct {
            EditorCommand command;
        } execute_command;
        struct {
            char tool_id[EDITOR_ACTION_ID_CAPACITY];
        } set_tool;
        struct {
            ObjectId object_id;
            char property_name[EDITOR_ACTION_PROPERTY_CAPACITY];
            float value;
        } modify_property;
        struct {
            LayerId layer_id;
        } set_active_layer;
        struct {
            LayerId layer_id;
            int visible;
        } set_layer_visibility;
        struct {
            LayerId layer_id;
            int locked;
        } set_layer_locked;
        struct {
            LayerId layer_id;
            char name[EDITOR_ACTION_TEXT_CAPACITY];
        } rename_layer;
        struct {
            LayerId layer_id;
            int target_index;
        } move_layer;
        struct {
            char name[EDITOR_ACTION_TEXT_CAPACITY];
        } create_layer;
        struct {
            UiDialogResult result;
            char text[EDITOR_ACTION_TEXT_CAPACITY];
        } resolve_dialog;
        struct {
            char message[EDITOR_ACTION_STATUS_CAPACITY];
        } set_status_message;
    } payload;
} EditorAction;

typedef void (*EditorActionCallback)(const EditorAction* action, void* user_data);

typedef struct EditorActionSink {
    EditorActionCallback callback;
    void* user_data;
} EditorActionSink;

void editor_action_init(EditorAction* action);
EditorAction editor_action_make_execute_command(EditorCommand command);
EditorAction editor_action_make_set_tool(const char* tool_id);
EditorAction editor_action_make_modify_property(ObjectId object_id,
                                                const char* property_name,
                                                float value);
EditorAction editor_action_make_set_active_layer(LayerId layer_id);
EditorAction editor_action_make_set_layer_visibility(LayerId layer_id, int visible);
EditorAction editor_action_make_set_layer_locked(LayerId layer_id, int locked);
EditorAction editor_action_make_rename_layer(LayerId layer_id, const char* name);
EditorAction editor_action_make_move_layer(LayerId layer_id, int target_index);
EditorAction editor_action_make_create_layer(const char* name);
EditorAction editor_action_make_resolve_dialog(UiDialogResult result,
                                               const char* text);
EditorAction editor_action_make_set_status_message(const char* message);
void editor_action_emit(const EditorActionSink* sink, const EditorAction* action);

#endif /* GLDRAW_UI_EDITOR_ACTION_H */
