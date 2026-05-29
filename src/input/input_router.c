/**
 * @file input_router.c
 * @brief Keyboard routing from modal/global/tool scopes into commands.
 */
#include <input/input_router.h>

#include <app/command_availability.h>
#include <app/command_catalog.h>
#include <app/command_registry.h>
#include <app/editor_controller.h>
#include <app/workspace.h>
#include <app/workspace_actions.h>
#include <tools/tool.h>

#include <GLFW/glfw3.h>

static int tool_key_from_glfw(int key)
{
    switch (key) {
    case GLFW_KEY_ESCAPE:
        return TOOL_INPUT_KEY_ESCAPE;
    default:
        return TOOL_INPUT_KEY_UNKNOWN;
    }
}

static int tool_mods_from_glfw(int mods)
{
    int tool_mods = 0;

    if ((mods & GLFW_MOD_SHIFT) != 0) {
        tool_mods |= TOOL_INPUT_MOD_SHIFT;
    }
    if ((mods & GLFW_MOD_CONTROL) != 0) {
        tool_mods |= TOOL_INPUT_MOD_CONTROL;
    }
    if ((mods & GLFW_MOD_ALT) != 0) {
        tool_mods |= TOOL_INPUT_MOD_ALT;
    }
    if ((mods & GLFW_MOD_SUPER) != 0) {
        tool_mods |= TOOL_INPUT_MOD_SUPER;
    }
    return tool_mods;
}

int input_router_handle_key(const InputRouterContext* context, const KeyEvent* event)
{
    KeyChord chord;
    KeyScope scope = KEY_SCOPE_GLOBAL;
    const char* command_id = NULL;
    const CommandDescriptor* descriptor = NULL;
    const EditorKeymap* keymap = NULL;

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
    keymap = workspace_get_keymap_const(context->workspace);
    command_id = keymap ? keymap_lookup_command(keymap, scope, chord) : NULL;
    if (command_id) {
        descriptor = command_catalog_find_by_id(command_id);
        if (descriptor &&
            command_availability_is_available(context->workspace,
                                              descriptor->command)) {
            return command_registry_execute(context->workspace,
                                            context->tool_context,
                                            descriptor->command);
        }
    }

    if (scope == KEY_SCOPE_MODAL) {
        return 0;
    }

    editor_controller_key_down(context->workspace,
                               context->tool_context,
                               tool_key_from_glfw(event->key),
                               tool_mods_from_glfw(event->mods));
    return 1;
}
