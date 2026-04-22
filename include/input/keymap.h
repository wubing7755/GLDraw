/**
 * @file keymap.h
 * @brief Scope-aware command keymap with default and user override layers.
 */
#ifndef GLDRAW_INPUT_KEYMAP_H
#define GLDRAW_INPUT_KEYMAP_H

#include <input/key_chord.h>

#define KEYMAP_MAX_BINDINGS 64

typedef enum KeyScope {
    KEY_SCOPE_GLOBAL = 0,
    KEY_SCOPE_MODAL,
    KEY_SCOPE_TOOL,
    KEY_SCOPE_COUNT
} KeyScope;

typedef struct KeyBinding {
    const char* command_id;
    KeyScope scope;
    KeyChord primary;
    KeyChord secondary;
} KeyBinding;

typedef struct EditorKeymap {
    KeyBinding defaults[KEYMAP_MAX_BINDINGS];
    int default_count;
    KeyBinding user_overrides[KEYMAP_MAX_BINDINGS];
    int user_override_count;
    char settings_path[260];
    char last_error[256];
} EditorKeymap;

/** Initialize the keymap and load user overrides when available. */
void keymap_init(EditorKeymap* keymap, const char* settings_path);
/** Clear runtime state owned by the keymap. */
void keymap_shutdown(EditorKeymap* keymap);
/** Resolve a scope/key chord to a command identifier, or `NULL` when unbound. */
const char* keymap_lookup_command(const EditorKeymap* keymap, KeyScope scope, KeyChord chord);
/** Get the effective shortcut text for a command, using the first bound chord. */
void keymap_format_command_shortcut(const EditorKeymap* keymap,
                                    const char* command_id,
                                    KeyScope scope,
                                    char* buffer,
                                    size_t buffer_size);

#endif /* GLDRAW_INPUT_KEYMAP_H */
