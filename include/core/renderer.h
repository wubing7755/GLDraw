#ifndef RENDERER_H
#define RENDERER_H

int init_renderer(void);
void render_frame(void);
void cleanup_renderer(void);
void renderer_on_viewport_change(void);

#endif /* RENDERER_H */
