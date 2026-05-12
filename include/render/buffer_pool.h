#ifndef GLDRAW_RENDER_BUFFER_POOL_H
#define GLDRAW_RENDER_BUFFER_POOL_H

#include <stddef.h>

typedef struct RenderVertexAttribute {
    unsigned int index;
    int component_count;
    size_t offset_bytes;
} RenderVertexAttribute;

typedef struct RenderVertexStreamDesc {
    size_t stride_bytes;
    const RenderVertexAttribute* attributes;
    size_t attribute_count;
    size_t initial_capacity_bytes;
} RenderVertexStreamDesc;

typedef struct RenderVertexStream {
    unsigned int vertex_array;
    unsigned int vertex_buffer;
    size_t stride_bytes;
    size_t capacity_bytes;
} RenderVertexStream;

typedef struct BufferPool {
    RenderVertexStream* streams;
    size_t stream_count;
    size_t stream_capacity;
} BufferPool;

void buffer_pool_init(BufferPool* pool);
void buffer_pool_shutdown(BufferPool* pool);
int buffer_pool_create_vertex_stream(BufferPool* pool,
                                     const RenderVertexStreamDesc* desc,
                                     RenderVertexStream* out_stream);
int buffer_pool_reserve(BufferPool* pool, RenderVertexStream* stream, size_t capacity_bytes);
void buffer_pool_bind_vertex_stream(const RenderVertexStream* stream);

#endif /* GLDRAW_RENDER_BUFFER_POOL_H */
