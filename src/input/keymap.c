/**
 * @file keymap.c
 * @brief Scope-aware keymap storage and shortcut formatting.
 */
#include <input/keymap.h>

#include <tools/tool.h>

#include <stdio.h>
#include <string.h>

static const struct {
    const char* command_id;
    KeyScope scope;
    const char* primary;
    const char* secondary;
} g_default_bindings[] = {
    {"file.new", KEY_SCOPE_GLOBAL, "Ctrl+N", ""},
    {"file.open", KEY_SCOPE_GLOBAL, "Ctrl+O", ""},
    {"file.save", KEY_SCOPE_GLOBAL, "Ctrl+S", ""},
    {"edit.undo", KEY_SCOPE_GLOBAL, "Ctrl+Z", ""},
    {"edit.redo", KEY_SCOPE_GLOBAL, "Ctrl+Y", "Ctrl+Shift+Z"},
    {"edit.cut", KEY_SCOPE_GLOBAL, "Ctrl+X", ""},
    {"edit.copy", KEY_SCOPE_GLOBAL, "Ctrl+C", ""},
    {"edit.paste", KEY_SCOPE_GLOBAL, "Ctrl+V", ""},
    {"edit.delete", KEY_SCOPE_GLOBAL, "Delete", "Backspace"},
    {"edit.select_all", KEY_SCOPE_GLOBAL, "Ctrl+A", ""},
    {"view.zoom_in", KEY_SCOPE_GLOBAL, "Ctrl+Plus", ""},
    {"view.zoom_out", KEY_SCOPE_GLOBAL, "Ctrl+Minus", ""},
    {"view.zoom_fit", KEY_SCOPE_GLOBAL, "Ctrl+0", ""},
    {"help.shortcuts", KEY_SCOPE_GLOBAL, "Shift+Slash", ""},
    {"help.shortcuts", KEY_SCOPE_MODAL, "Shift+Slash", ""},
    {"modal.confirm", KEY_SCOPE_MODAL, "Enter", ""},
    {"modal.cancel", KEY_SCOPE_MODAL, "Esc", ""}
};

static int keymap_binding_matches(const KeyBinding* binding, KeyScope scope, KeyChord chord)
{
    if (!binding || binding->scope != scope || !binding->command_id) {
        return 0;
    }

    return key_chord_equals(binding->primary, chord) ||
           key_chord_equals(binding->secondary, chord);
}

static const KeyBinding* keymap_find_effective_binding(const EditorKeymap* keymap,
                                                       const char* command_id,
                                                       KeyScope scope)
{
    int i = 0;

    if (!keymap || !command_id) {
        return NULL;
    }

    for (i = 0; i < keymap->user_override_count; ++i) {
        if (keymap->user_overrides[i].scope == scope &&
            keymap->user_overrides[i].command_id &&
            strcmp(keymap->user_overrides[i].command_id, command_id) == 0) {
            return &keymap->user_overrides[i];
        }
    }

    for (i = 0; i < keymap->default_count; ++i) {
        if (keymap->defaults[i].scope == scope &&
            keymap->defaults[i].command_id &&
            strcmp(keymap->defaults[i].command_id, command_id) == 0) {
            return &keymap->defaults[i];
        }
    }

    return NULL;
}

void keymap_init(EditorKeymap* keymap, const char* settings_path)
{
    int i = 0;
    int tool_index = 0;

    if (!keymap) {
        return;
    }

    memset(keymap, 0, sizeof(*keymap));
    if (settings_path) {
        snprintf(keymap->settings_path, sizeof(keymap->settings_path), "%s", settings_path);
    }

    for (i = 0; i < (int)(sizeof(g_default_bindings) / sizeof(g_default_bindings[0])); ++i) {
        KeyBinding* binding = &keymap->defaults[keymap->default_count];
        binding->command_id = g_default_bindings[i].command_id;
        binding->scope = g_default_bindings[i].scope;
        binding->primary = key_chord_none();
        binding->secondary = key_chord_none();
        key_chord_parse(g_default_bindings[i].primary, &binding->primary);
        if (g_default_bindings[i].secondary[0] != '\0') {
            key_chord_parse(g_default_bindings[i].secondary, &binding->secondary);
        }
        keymap->default_count++;
    }

    for (tool_index = 0; tool_index < tool_registry_count(); ++tool_index) {
        const ToolDescriptor* descriptor = tool_registry_at(tool_index);
        KeyBinding* binding = NULL;

        if (!descriptor || !descriptor->command_id || !descriptor->default_shortcut ||
            descriptor->default_shortcut[0] == '\0') {
            continue;
        }

        binding = &keymap->defaults[keymap->default_count];
        binding->command_id = descriptor->command_id;
        binding->scope = KEY_SCOPE_GLOBAL;
        binding->primary = key_chord_none();
        binding->secondary = key_chord_none();
        key_chord_parse(descriptor->default_shortcut, &binding->primary);
        keymap->default_count++;
    }
}

void keymap_shutdown(EditorKeymap* keymap)
{
    if (!keymap) {
        return;
    }

    memset(keymap, 0, sizeof(*keymap));
}

const char* keymap_lookup_command(const EditorKeymap* keymap, KeyScope scope, KeyChord chord)
{
    int i = 0;

    if (!keymap || !key_chord_is_valid(chord)) {
        return NULL;
    }

    for (i = 0; i < keymap->user_override_count; ++i) {
        if (keymap_binding_matches(&keymap->user_overrides[i], scope, chord)) {
            return keymap->user_overrides[i].command_id;
        }
    }

    for (i = 0; i < keymap->default_count; ++i) {
        if (keymap_binding_matches(&keymap->defaults[i], scope, chord)) {
            return keymap->defaults[i].command_id;
        }
    }

    return NULL;
}

void keymap_format_command_shortcut(const EditorKeymap* keymap,
                                    const char* command_id,
                                    KeyScope scope,
                                    char* buffer,
                                    size_t buffer_size)
{
    const KeyBinding* binding = NULL;

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!keymap || !command_id) {
        return;
    }

    binding = keymap_find_effective_binding(keymap, command_id, scope);
    if (!binding) {
        return;
    }

    key_chord_format(binding->primary, buffer, buffer_size);
}
