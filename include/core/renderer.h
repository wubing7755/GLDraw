#ifndef RENDERER_H
#define RENDERER_H

int init_renderer(void);
void render_frame(void);  /* no longer takes AppState* */
void cleanup_renderer(void);

#endif /* RENDERER_H */
