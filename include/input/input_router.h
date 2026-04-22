/**
 * @file input_router.h
 * @brief Keyboard event routing from platform callbacks to editor commands.
 */
#ifndef GLDRAW_INPUT_INPUT_ROUTER_H
#define GLDRAW_INPUT_INPUT_ROUTER_H

#include <input/keymap.h>
#include <tools/tool.h>

struct Workspace;

typedef struct KeyEvent {
    int key;
    int mods;
    int action;
    int repeated;
} KeyEvent;

typedef struct InputRouterContext {
    struct Workspace* workspace;
    ToolContext* tool_context;
    int ui_has_keyboard_focus;
} InputRouterContext;

/** Handle one key event through modal/global/tool routing. */
int input_router_handle_key(const InputRouterContext* context, const KeyEvent* event);

#endif /* GLDRAW_INPUT_INPUT_ROUTER_H */
