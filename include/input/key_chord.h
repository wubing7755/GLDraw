/**
 * @file key_chord.h
 * @brief Normalized keyboard chord parsing and formatting helpers.
 */
#ifndef GLDRAW_INPUT_KEY_CHORD_H
#define GLDRAW_INPUT_KEY_CHORD_H

#include <stddef.h>

typedef struct KeyChord {
    int key;
    int mods;
} KeyChord;

/** Return a zero-value chord representing "unbound". */
KeyChord key_chord_none(void);
/** Return non-zero when the chord contains a concrete key. */
int key_chord_is_valid(KeyChord chord);
/** Compare two chords for exact key/mod equivalence. */
int key_chord_equals(KeyChord a, KeyChord b);
/** Parse a human-readable chord such as `Ctrl+Shift+Z`. */
int key_chord_parse(const char* text, KeyChord* out_chord);
/** Format a chord into a human-readable string. */
void key_chord_format(KeyChord chord, char* buffer, size_t buffer_size);

#endif /* GLDRAW_INPUT_KEY_CHORD_H */
