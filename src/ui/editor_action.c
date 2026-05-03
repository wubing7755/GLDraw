#include <ui/editor_action.h>

#include <stdio.h>
#include <string.h>

static void editor_action_copy_string(char* dst, size_t dst_size, const char* src)
{
    if (!dst || dst_size == 0u) {
        return;
    }

    snprintf(dst, dst_size, "%s", src ? src : "");
}

void editor_action_init(EditorAction* action)
{
    if (!action) {
        return;
    }

    memset(action, 0, sizeof(*action));
}

EditorAction editor_action_make_execute_command(EditorCommand command)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_EXECUTE_COMMAND;
    action.payload.execute_command.command = command;
    return action;
}

EditorAction editor_action_make_set_tool(const char* tool_id)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_SET_TOOL;
    editor_action_copy_string(action.payload.set_tool.tool_id,
                              sizeof(action.payload.set_tool.tool_id),
                              tool_id);
    return action;
}

EditorAction editor_action_make_modify_property(ObjectId object_id,
                                                const char* property_name,
                                                float value)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_MODIFY_PROPERTY;
    action.payload.modify_property.object_id = object_id;
    action.payload.modify_property.value = value;
    editor_action_copy_string(action.payload.modify_property.property_name,
                              sizeof(action.payload.modify_property.property_name),
                              property_name);
    return action;
}

EditorAction editor_action_make_set_active_layer(LayerId layer_id)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_SET_ACTIVE_LAYER;
    action.payload.set_active_layer.layer_id = layer_id;
    return action;
}

EditorAction editor_action_make_set_layer_visibility(LayerId layer_id, int visible)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_SET_LAYER_VISIBILITY;
    action.payload.set_layer_visibility.layer_id = layer_id;
    action.payload.set_layer_visibility.visible = visible ? 1 : 0;
    return action;
}

EditorAction editor_action_make_set_layer_locked(LayerId layer_id, int locked)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_SET_LAYER_LOCKED;
    action.payload.set_layer_locked.layer_id = layer_id;
    action.payload.set_layer_locked.locked = locked ? 1 : 0;
    return action;
}

EditorAction editor_action_make_rename_layer(LayerId layer_id, const char* name)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_RENAME_LAYER;
    action.payload.rename_layer.layer_id = layer_id;
    editor_action_copy_string(action.payload.rename_layer.name,
                              sizeof(action.payload.rename_layer.name),
                              name);
    return action;
}

EditorAction editor_action_make_move_layer(LayerId layer_id, int target_index)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_MOVE_LAYER;
    action.payload.move_layer.layer_id = layer_id;
    action.payload.move_layer.target_index = target_index;
    return action;
}

EditorAction editor_action_make_create_layer(const char* name)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_CREATE_LAYER;
    editor_action_copy_string(action.payload.create_layer.name,
                              sizeof(action.payload.create_layer.name),
                              name);
    return action;
}

EditorAction editor_action_make_resolve_dialog(UiDialogResult result,
                                               const char* text)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_RESOLVE_DIALOG;
    action.payload.resolve_dialog.result = result;
    editor_action_copy_string(action.payload.resolve_dialog.text,
                              sizeof(action.payload.resolve_dialog.text),
                              text);
    return action;
}

EditorAction editor_action_make_set_status_message(const char* message)
{
    EditorAction action;

    editor_action_init(&action);
    action.type = EDITOR_ACTION_SET_STATUS_MESSAGE;
    editor_action_copy_string(action.payload.set_status_message.message,
                              sizeof(action.payload.set_status_message.message),
                              message);
    return action;
}

void editor_action_emit(const EditorActionSink* sink, const EditorAction* action)
{
    if (!sink || !sink->callback || !action) {
        return;
    }

    sink->callback(action, sink->user_data);
}
