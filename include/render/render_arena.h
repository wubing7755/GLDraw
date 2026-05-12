#ifndef GLDRAW_RENDER_RENDER_ARENA_H
#define GLDRAW_RENDER_RENDER_ARENA_H

#include <stddef.h>

typedef struct RenderArena {
    unsigned char* memory;
    size_t capacity_bytes;
    size_t offset_bytes;
} RenderArena;

void render_arena_init(RenderArena* arena);
void render_arena_reset(RenderArena* arena);
void render_arena_shutdown(RenderArena* arena);
int render_arena_reserve(RenderArena* arena, size_t capacity_bytes);
void* render_arena_alloc(RenderArena* arena, size_t size_bytes, size_t alignment_bytes);

#endif /* GLDRAW_RENDER_RENDER_ARENA_H */
