#ifndef RENDERER_H
#define RENDERER_H

#include <core/selection_manager.h>

int init_renderer(void);
void render_frame(void);
void cleanup_renderer(void);
void renderer_set_selection(SelectionManager* sel);

#endif /* RENDERER_H */
