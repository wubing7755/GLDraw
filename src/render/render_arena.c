#include <render/render_arena.h>

#include <stdlib.h>

static size_t render_arena_align_up(size_t value, size_t alignment)
{
    size_t remainder = 0u;

    if (alignment <= 1u) {
        return value;
    }

    remainder = value % alignment;
    return remainder == 0u ? value : value + (alignment - remainder);
}

void render_arena_init(RenderArena* arena)
{
    if (!arena) {
        return;
    }

    arena->memory = NULL;
    arena->capacity_bytes = 0u;
    arena->offset_bytes = 0u;
}

void render_arena_reset(RenderArena* arena)
{
    if (!arena) {
        return;
    }

    arena->offset_bytes = 0u;
}

void render_arena_shutdown(RenderArena* arena)
{
    if (!arena) {
        return;
    }

    free(arena->memory);
    render_arena_init(arena);
}

int render_arena_reserve(RenderArena* arena, size_t capacity_bytes)
{
    unsigned char* memory = NULL;
    size_t capacity = 0u;

    if (!arena) {
        return 0;
    }
    if (capacity_bytes <= arena->capacity_bytes) {
        return 1;
    }

    capacity = arena->capacity_bytes > 0u ? arena->capacity_bytes : 256u;
    while (capacity < capacity_bytes) {
        capacity *= 2u;
    }

    memory = (unsigned char*)realloc(arena->memory, capacity);
    if (!memory) {
        return 0;
    }

    arena->memory = memory;
    arena->capacity_bytes = capacity;
    return 1;
}

void* render_arena_alloc(RenderArena* arena, size_t size_bytes, size_t alignment_bytes)
{
    size_t offset = 0u;
    size_t required = 0u;

    if (!arena || size_bytes == 0u) {
        return NULL;
    }

    offset = render_arena_align_up(arena->offset_bytes, alignment_bytes);
    required = offset + size_bytes;
    if (!render_arena_reserve(arena, required)) {
        return NULL;
    }

    arena->offset_bytes = required;
    return arena->memory + offset;
}
