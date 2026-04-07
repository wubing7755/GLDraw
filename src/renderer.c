#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>

#include <core/renderer.h>
#include <core/shader.h>
#include <core/shape_manager.h>
#include <core/selection_manager.h>

static GLuint s_VAO = 0;
static GLuint s_VBO = 0;
static float* s_vertex_buf = NULL;
static size_t s_vertex_buf_capacity = 0;

/* =============================================================================
 * Phase 2: Multi-shape rendering using vtable-based geometry
 *
 * Vertex layout per vertex: x, y, r, g, b, a (6 floats = 24 bytes)
 * Each shape has different vertex counts:
 *   LINE:   2 vertices (GL_LINES)
 *   CIRCLE: 64 vertices (GL_LINE_LOOP)
 *   RECT:   4 vertices (GL_LINE_LOOP)
 *
 * We track per-shape vertex offset to issue separate draw calls.
 * =============================================================================
 */

typedef struct {
    int first_vertex;
    int vert_count;
} ShapeDrawInfo;

static ShapeDrawInfo* s_draw_infos = NULL;
static int s_draw_info_capacity = 0;
static SelectionManager* s_selection = NULL;
static const float s_selection_highlight[4] = {1.0f, 1.0f, 0.0f, 1.0f};

/* Compute total vertex count for all shapes */
static size_t compute_total_vertices(void)
{
    size_t total = 0;
    int count = sm_count();
    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        float* verts = NULL;
        int vert_count = 0;
        shape_get_geometry(s, &verts, &vert_count);
        total += vert_count;
        free(verts);  /* shape_get_geometry returns malloc'd buffer */
    }
    return total;
}

/* Rebuild vertex buffer from ShapeManager using vtable geometry */
static void rebuild_vertex_buffer(void)
{
    int count = sm_count();
    if (count == 0) {
        return;
    }

    /* Resize draw info array if needed */
    if (count > s_draw_info_capacity) {
        free(s_draw_infos);
        s_draw_infos = (ShapeDrawInfo*)malloc(count * sizeof(ShapeDrawInfo));
        s_draw_info_capacity = count;
    }

    /* First pass: compute total vertices needed */
    size_t total_verts = compute_total_vertices();
    size_t new_capacity = total_verts * 6 * sizeof(float);

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

    /* Second pass: fill buffer and track offsets */
    float* ptr = s_vertex_buf;
    int current_vert = 0;

    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        float* verts = NULL;
        int vert_count = 0;
        shape_get_geometry(s, &verts, &vert_count);

        s_draw_infos[i].first_vertex = current_vert;
        s_draw_infos[i].vert_count = vert_count;

        float r = s->color[0];
        float g = s->color[1];
        float b = s->color[2];
        float a = s->color[3];

        for (int j = 0; j < vert_count; j++) {
            *ptr++ = verts[j * 2];     /* x */
            *ptr++ = verts[j * 2 + 1]; /* y */
            *ptr++ = r;
            *ptr++ = g;
            *ptr++ = b;
            *ptr++ = a;
            current_vert++;
        }
        free(verts);
    }
}

/* Determine OpenGL primitive type based on vertex count */
static GLenum primitive_type_for_count(int vert_count)
{
    if (vert_count == 2) {
        return GL_LINES;     /* LINE */
    }
    return GL_LINE_LOOP;    /* CIRCLE, RECT */
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

    size_t buf_size = s_draw_infos ? (size_t)(s_draw_infos[count - 1].first_vertex + s_draw_infos[count - 1].vert_count) * 6 * sizeof(float) : 0;
    if (buf_size == 0) {
        return;
    }

    /* Upload to GPU */
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, buf_size, s_vertex_buf, GL_DYNAMIC_DRAW);

    /* Draw each shape with appropriate primitive type */
    shader_use();
    glBindVertexArray(s_VAO);

    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        GLenum mode = primitive_type_for_count(s_draw_infos[i].vert_count);
        float line_width = (s->line_width > 0.0f) ? s->line_width : 1.0f;

        /* Highlight selected shapes with yellow */
        int is_selected = s_selection && sel_contains(s_selection, s);
        if (is_selected) {
            /* Draw a highlight pass first, then the actual shape color on top. */
            glDisableVertexAttribArray(1);
            glVertexAttrib4f(1,
                             s_selection_highlight[0],
                             s_selection_highlight[1],
                             s_selection_highlight[2],
                             s_selection_highlight[3]);
            glLineWidth(line_width + 2.0f);
            glDrawArrays(mode, s_draw_infos[i].first_vertex, s_draw_infos[i].vert_count);
            glEnableVertexAttribArray(1);
        }

        glLineWidth(line_width);
        glDrawArrays(mode, s_draw_infos[i].first_vertex, s_draw_infos[i].vert_count);
    }

    glLineWidth(1.0f);
    glBindVertexArray(0);

    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "[Renderer] GL error after draw: %d\n", err);
    }
}

void renderer_set_selection(SelectionManager* sel)
{
    s_selection = sel;
}

void cleanup_renderer(void)
{
    glDeleteVertexArrays(1, &s_VAO);
    glDeleteBuffers(1, &s_VBO);
    free(s_vertex_buf);
    free(s_draw_infos);
    s_vertex_buf = NULL;
    s_draw_infos = NULL;
    s_vertex_buf_capacity = 0;
    s_draw_info_capacity = 0;
    s_VAO = 0;
    s_VBO = 0;
    printf("[Renderer] Cleaned up\n");
}
