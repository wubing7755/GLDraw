#include <render/buffer_pool.h>

#include <glad/glad.h>

#include <stdint.h>
#include <stdlib.h>

static int buffer_pool_track_stream(BufferPool* pool, const RenderVertexStream* stream)
{
    RenderVertexStream* streams = NULL;
    size_t capacity = 0u;

    if (!pool || !stream || !stream->vertex_array || !stream->vertex_buffer) {
        return 0;
    }
    if (pool->stream_count >= pool->stream_capacity) {
        capacity = pool->stream_capacity > 0u ? pool->stream_capacity * 2u : 4u;
        streams = (RenderVertexStream*)realloc(pool->streams, capacity * sizeof(streams[0]));
        if (!streams) {
            return 0;
        }

        pool->streams = streams;
        pool->stream_capacity = capacity;
    }

    pool->streams[pool->stream_count++] = *stream;
    return 1;
}

static void buffer_pool_sync_stream(BufferPool* pool, const RenderVertexStream* stream)
{
    size_t i = 0u;

    if (!pool || !stream) {
        return;
    }

    for (i = 0u; i < pool->stream_count; ++i) {
        if (pool->streams[i].vertex_array == stream->vertex_array &&
            pool->streams[i].vertex_buffer == stream->vertex_buffer) {
            pool->streams[i] = *stream;
            return;
        }
    }
}

void buffer_pool_init(BufferPool* pool)
{
    if (!pool) {
        return;
    }

    pool->streams = NULL;
    pool->stream_count = 0u;
    pool->stream_capacity = 0u;
}

void buffer_pool_shutdown(BufferPool* pool)
{
    size_t i = 0u;

    if (!pool) {
        return;
    }

    for (i = 0u; i < pool->stream_count; ++i) {
        if (pool->streams[i].vertex_buffer != 0u) {
            glDeleteBuffers(1, &pool->streams[i].vertex_buffer);
        }
        if (pool->streams[i].vertex_array != 0u) {
            glDeleteVertexArrays(1, &pool->streams[i].vertex_array);
        }
    }

    free(pool->streams);
    buffer_pool_init(pool);
}

int buffer_pool_create_vertex_stream(BufferPool* pool,
                                     const RenderVertexStreamDesc* desc,
                                     RenderVertexStream* out_stream)
{
    RenderVertexStream stream = {0u, 0u, 0u, 0u};
    size_t i = 0u;

    if (!pool || !desc || !out_stream || !desc->attributes || desc->attribute_count == 0u ||
        desc->stride_bytes == 0u) {
        return 0;
    }

    glGenVertexArrays(1, &stream.vertex_array);
    glGenBuffers(1, &stream.vertex_buffer);
    if (!stream.vertex_array || !stream.vertex_buffer) {
        if (stream.vertex_buffer) {
            glDeleteBuffers(1, &stream.vertex_buffer);
        }
        if (stream.vertex_array) {
            glDeleteVertexArrays(1, &stream.vertex_array);
        }
        return 0;
    }

    stream.stride_bytes = desc->stride_bytes;
    stream.capacity_bytes = desc->initial_capacity_bytes;

    glBindVertexArray(stream.vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, stream.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)stream.capacity_bytes, NULL, GL_DYNAMIC_DRAW);
    for (i = 0u; i < desc->attribute_count; ++i) {
        const RenderVertexAttribute* attribute = &desc->attributes[i];

        glVertexAttribPointer(attribute->index,
                              attribute->component_count,
                              GL_FLOAT,
                              GL_FALSE,
                              (GLsizei)stream.stride_bytes,
                              (void*)(uintptr_t)attribute->offset_bytes);
        glEnableVertexAttribArray(attribute->index);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!buffer_pool_track_stream(pool, &stream)) {
        glDeleteBuffers(1, &stream.vertex_buffer);
        glDeleteVertexArrays(1, &stream.vertex_array);
        return 0;
    }

    *out_stream = stream;
    return 1;
}

int buffer_pool_reserve(BufferPool* pool, RenderVertexStream* stream, size_t capacity_bytes)
{
    if (!pool || !stream) {
        return 0;
    }
    if (capacity_bytes <= stream->capacity_bytes) {
        return 1;
    }

    glBindBuffer(GL_ARRAY_BUFFER, stream->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)capacity_bytes, NULL, GL_DYNAMIC_DRAW);
    stream->capacity_bytes = capacity_bytes;
    buffer_pool_sync_stream(pool, stream);
    return 1;
}

void buffer_pool_bind_vertex_stream(const RenderVertexStream* stream)
{
    if (!stream) {
        return;
    }

    glBindVertexArray(stream->vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, stream->vertex_buffer);
}
