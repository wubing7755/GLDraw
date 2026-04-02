#ifndef APP_STATE_H
#define APP_STATE_H

/* Unified application state - shared across all modules */
typedef struct {
    float color[3];     /* RGB color for rendering */
    float offset_y;     /* Triangle Y offset (from input) */
} AppState;

extern AppState g_app_state;

#endif /* APP_STATE_H */
