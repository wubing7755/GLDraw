#ifndef RENDERER_H
#define RENDERER_H

#include <core/app_state.h>

int init_renderer(void);
void render_frame(const AppState* state);
void cleanup_renderer(void);

#endif /* RENDERER_H */
