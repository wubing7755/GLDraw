#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>

#include <core/renderer.h>
#include <core/shader.h>
#include "shape_manager.h"

static GLuint s_VAO = 0;
static GLuint s_VBO = 0;
static float* s_vertex_buf = NULL;
static size_t s_vertex_buf_capacity = 0;

/* =============================================================================
 * Phase 1: Dynamic line rendering — rebuild vertex buffer each frame
 *
 * Vertex layout per vertex: x, y, r, g, b, a (6 floats = 24 bytes)
 * Each LINE needs 2 vertices (start + end).
 *
 * Phase 2 note: Will add vtable-based geometry generation to support
 *               CIRCLE/RECTANGLE without changing this render function.
 * =============================================================================
 */

static void rebuild_vertex_buffer(void)
{
    int count = sm_count();
    if (count == 0) {
        s_vertex_buf_capacity = 0;
        return;
    }

    size_t vertex_count = count * 2;  /* 2 vertices per LINE */
    size_t new_capacity = vertex_count * 6 * sizeof(float);

    if (new_capacity > s_vertex_buf_capacity) {
        free(s_vertex_buf);
        s_vertex_buf = (float*)malloc(new_capacity);
        if (!s_vertex_buf) {
            fprintf(stderr, "[Renderer] Failed to allocate vertex buffer\n");
            s_vertex_buf_capacity = 0;
            return;
        }
        s_vertex_buf_capacity = new_capacity;
    }

    float* ptr = s_vertex_buf;
    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        /* Vertex 1: p1 */
        *ptr++ = s->line.p1[0];
        *ptr++ = s->line.p1[1];
        *ptr++ = s->color[0];
        *ptr++ = s->color[1];
        *ptr++ = s->color[2];
        *ptr++ = s->color[3];

        /* Vertex 2: p2 */
        *ptr++ = s->line.p2[0];
        *ptr++ = s->line.p2[1];
        *ptr++ = s->color[0];
        *ptr++ = s->color[1];
        *ptr++ = s->color[2];
        *ptr++ = s->color[3];
    }
}

int init_renderer(void)
{
    /* Create VAO */
    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);
    printf("[Renderer] VAO created: %u\n", s_VAO);

    /* Create dynamic VBO */
    glGenBuffers(1, &s_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);

    /* Vertex format: x, y, r, g, b, a (6 floats) */
    /* Attribute 0: position (x, y) */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Attribute 1: color (r, g, b, a) */
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    printf("[Renderer] Initialized (dynamic line buffer)\n");
    return 0;
}

void render_frame(void)
{
    GLenum err;

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int count = sm_count();
    if (count == 0) {
        return;
    }

    /* Rebuild vertex buffer from ShapeManager */
    rebuild_vertex_buffer();
    if (!s_vertex_buf || s_vertex_buf_capacity == 0) {
        return;
    }

    size_t vertex_count = count * 2;
    size_t buf_size = vertex_count * 6 * sizeof(float);

    /* Upload to GPU */
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, buf_size, s_vertex_buf, GL_DYNAMIC_DRAW);

    /* Draw all lines */
    shader_use();
    glBindVertexArray(s_VAO);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertex_count);
    glBindVertexArray(0);

    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "[Renderer] GL error after draw: %d\n", err);
    }
}

void cleanup_renderer(void)
{
    glDeleteVertexArrays(1, &s_VAO);
    glDeleteBuffers(1, &s_VBO);
    free(s_vertex_buf);
    s_vertex_buf = NULL;
    s_vertex_buf_capacity = 0;
    s_VAO = 0;
    s_VBO = 0;
    printf("[Renderer] Cleaned up\n");
}
