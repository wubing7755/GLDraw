/**
 * @file input_router.c
 * @brief Keyboard routing from modal/global/tool scopes into commands.
 */
#include <input/input_router.h>

#include <app/command_registry.h>
#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <tools/tool.h>
#include <tools/tool_controller.h>

#include <GLFW/glfw3.h>

int input_router_handle_key(const InputRouterContext* context, const KeyEvent* event)
{
    KeyChord chord;
    KeyScope scope = KEY_SCOPE_GLOBAL;
    const char* command_id = NULL;
    const CommandDescriptor* descriptor = NULL;
    EditorKeymap* keymap = NULL;
    ToolController* tools = NULL;

    if (!context || !context->workspace || !context->tool_context || !event ||
        event->action != GLFW_PRESS) {
        return 0;
    }

    if (workspace_modal_is_active(context->workspace)) {
        scope = KEY_SCOPE_MODAL;
    } else if (context->ui_has_keyboard_focus) {
        return 0;
    }

    chord.key = event->key;
    chord.mods = event->mods;
    keymap = workspace_get_keymap(context->workspace);
    command_id = keymap ? keymap_lookup_command(keymap, scope, chord) : NULL;
    if (command_id) {
        descriptor = command_registry_find_by_id(command_id);
        if (descriptor &&
            command_registry_is_available(context->workspace,
                                          descriptor->command)) {
            return command_registry_execute(context->workspace,
                                            context->tool_context,
                                            descriptor->command);
        }
    }

    if (scope == KEY_SCOPE_MODAL) {
        return 0;
    }

    tools = workspace_get_tool_controller(context->workspace);
    if (!tools) {
        return 0;
    }

    tool_controller_key_down(tools, context->tool_context, event->key, event->mods);
    return 1;
}
