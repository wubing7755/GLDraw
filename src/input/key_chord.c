/**
 * @file key_chord.c
 * @brief Keyboard chord parsing and formatting helpers.
 */
#include <input/key_chord.h>

#include <GLFW/glfw3.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct NamedKey {
    const char* name;
    int key;
} NamedKey;

static const NamedKey g_named_keys[] = {
    {"Enter", GLFW_KEY_ENTER},
    {"Esc", GLFW_KEY_ESCAPE},
    {"Escape", GLFW_KEY_ESCAPE},
    {"Delete", GLFW_KEY_DELETE},
    {"Backspace", GLFW_KEY_BACKSPACE},
    {"Slash", GLFW_KEY_SLASH},
    {"Plus", GLFW_KEY_EQUAL},
    {"Minus", GLFW_KEY_MINUS},
    {"0", GLFW_KEY_0},
    {"A", GLFW_KEY_A},
    {"C", GLFW_KEY_C},
    {"E", GLFW_KEY_E},
    {"H", GLFW_KEY_H},
    {"L", GLFW_KEY_L},
    {"N", GLFW_KEY_N},
    {"O", GLFW_KEY_O},
    {"R", GLFW_KEY_R},
    {"S", GLFW_KEY_S},
    {"V", GLFW_KEY_V},
    {"X", GLFW_KEY_X},
    {"Y", GLFW_KEY_Y},
    {"Z", GLFW_KEY_Z}
};

static int key_chord_named_key_from_token(const char* token)
{
    size_t i = 0u;

    for (i = 0u; i < sizeof(g_named_keys) / sizeof(g_named_keys[0]); ++i) {
        if (strcmp(token, g_named_keys[i].name) == 0) {
            return g_named_keys[i].key;
        }
    }

    return GLFW_KEY_UNKNOWN;
}

static const char* key_chord_name_for_key(int key)
{
    size_t i = 0u;
    static char fallback_name[2];

    for (i = 0u; i < sizeof(g_named_keys) / sizeof(g_named_keys[0]); ++i) {
        if (g_named_keys[i].key == key) {
            return g_named_keys[i].name;
        }
    }

    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        fallback_name[0] = (char)('A' + (key - GLFW_KEY_A));
        fallback_name[1] = '\0';
        return fallback_name;
    }

    return "";
}

KeyChord key_chord_none(void)
{
    KeyChord chord;
    chord.key = GLFW_KEY_UNKNOWN;
    chord.mods = 0;
    return chord;
}

int key_chord_is_valid(KeyChord chord)
{
    return chord.key != GLFW_KEY_UNKNOWN;
}

int key_chord_equals(KeyChord a, KeyChord b)
{
    return a.key == b.key && a.mods == b.mods;
}

int key_chord_parse(const char* text, KeyChord* out_chord)
{
    char local[64];
    char* token = NULL;
    int key = GLFW_KEY_UNKNOWN;
    int mods = 0;

    if (!text || !out_chord || text[0] == '\0') {
        return 0;
    }

    snprintf(local, sizeof(local), "%s", text);
    token = strtok(local, "+");
    while (token) {
        if (strcmp(token, "Ctrl") == 0) {
            mods |= GLFW_MOD_CONTROL;
        } else if (strcmp(token, "Shift") == 0) {
            mods |= GLFW_MOD_SHIFT;
        } else if (strcmp(token, "Alt") == 0) {
            mods |= GLFW_MOD_ALT;
        } else {
            key = key_chord_named_key_from_token(token);
            if (key == GLFW_KEY_UNKNOWN &&
                strlen(token) == 1u &&
                isalpha((unsigned char)token[0])) {
                key = GLFW_KEY_A + (toupper((unsigned char)token[0]) - 'A');
            }
        }
        token = strtok(NULL, "+");
    }

    if (key == GLFW_KEY_UNKNOWN) {
        return 0;
    }

    out_chord->key = key;
    out_chord->mods = mods;
    return 1;
}

void key_chord_format(KeyChord chord, char* buffer, size_t buffer_size)
{
    const char* key_name = NULL;
    int wrote_prefix = 0;

    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!key_chord_is_valid(chord)) {
        return;
    }

    if ((chord.mods & GLFW_MOD_CONTROL) != 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%sCtrl", wrote_prefix ? "+" : "");
        wrote_prefix = 1;
    }
    if ((chord.mods & GLFW_MOD_SHIFT) != 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%sShift", wrote_prefix ? "+" : "");
        wrote_prefix = 1;
    }
    if ((chord.mods & GLFW_MOD_ALT) != 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%sAlt", wrote_prefix ? "+" : "");
        wrote_prefix = 1;
    }

    key_name = key_chord_name_for_key(chord.key);
    if (key_name[0] != '\0') {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%s%s", wrote_prefix ? "+" : "", key_name);
    }
}
