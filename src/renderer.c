#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string.h>

#include <core/app_state.h>
#include <core/renderer.h>
#include <core/shader.h>
#include <core/shape_manager.h>
#include <core/macros.h>

static GLuint s_VAO = 0;
static GLuint s_VBO = 0;
static float* s_vertex_buf = NULL;
static size_t s_vertex_buf_capacity = 0;
static float* s_geometry_buf = NULL;
static size_t s_geometry_buf_capacity = 0;
static size_t s_uploaded_buf_size = 0;
static int s_gpu_buffer_dirty = 1;

/* Base resolution for normalized coordinates */
#define BASE_WIDTH 800
#define BASE_HEIGHT 600

/* Orthographic projection matrix (column-major, matches OpenGL) */
static float s_projection[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

/* Update projection matrix to map (0,0)-(base_w,base_h) to viewport */
static void update_projection(void)
{
    float left = 0.0f;
    float right = (float)BASE_WIDTH;
    float bottom = (float)BASE_HEIGHT;
    float top = 0.0f;  /* Y flipped for screen coordinates */

    /* Column-major orthographic projection */
    s_projection[0] = 2.0f / (right - left);
    s_projection[5] = 2.0f / (top - bottom);
    s_projection[10] = -1.0f;
    s_projection[12] = -(right + left) / (right - left);
    s_projection[13] = -(top + bottom) / (top - bottom);
    s_projection[14] = 0.0f;
    s_projection[15] = 1.0f;
}

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
static unsigned int* s_shape_revisions = NULL;
static int s_cached_shape_count = 0;
static unsigned int s_cached_shape_manager_revision = 0;
static const float s_selection_highlight[4] = {1.0f, 1.0f, 0.0f, 1.0f};

/* Compute total vertex count for all shapes */
static size_t compute_total_vertices(void)
{
    size_t total = 0;
    int count = sm_count();
    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        total += (size_t)shape_get_vertex_count(s);
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
        free(s_shape_revisions);
        s_draw_infos = (ShapeDrawInfo*)malloc(count * sizeof(ShapeDrawInfo));
        s_shape_revisions = (unsigned int*)malloc(count * sizeof(unsigned int));
        s_draw_info_capacity = count;
        if (UNLIKELY(!s_draw_infos || !s_shape_revisions)) {
            LOG_ERROR("Failed to allocate draw metadata");
            SAFE_FREE(s_draw_infos);
            SAFE_FREE(s_shape_revisions);
            s_draw_info_capacity = 0;
            return;
        }
    }

    /* First pass: compute total vertices needed */
    size_t total_verts = compute_total_vertices();
    size_t new_capacity = total_verts * 6 * sizeof(float);
    size_t geometry_capacity = total_verts * 2 * sizeof(float);

    if (new_capacity > s_vertex_buf_capacity) {
        free(s_vertex_buf);
        s_vertex_buf = (float*)malloc(new_capacity);
        if (UNLIKELY(!s_vertex_buf)) {
            LOG_ERROR("Failed to allocate vertex buffer");
            s_vertex_buf_capacity = 0;
            return;
        }
        s_vertex_buf_capacity = new_capacity;
    }

    if (geometry_capacity > s_geometry_buf_capacity) {
        free(s_geometry_buf);
        s_geometry_buf = (float*)malloc(geometry_capacity);
        if (UNLIKELY(!s_geometry_buf)) {
            LOG_ERROR("Failed to allocate geometry scratch buffer");
            s_geometry_buf_capacity = 0;
            return;
        }
        s_geometry_buf_capacity = geometry_capacity;
    }

    /* Second pass: fill buffer and track offsets */
    float* ptr = s_vertex_buf;
    int current_vert = 0;

    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        int vert_count = shape_get_vertex_count(s);
        float* shape_vertices = s_geometry_buf;

        s_draw_infos[i].first_vertex = current_vert;
        s_draw_infos[i].vert_count = vert_count;
        s_shape_revisions[i] = shape_get_revision(s);

        shape_write_geometry(s, shape_vertices);

        float r = s->color[0];
        float g = s->color[1];
        float b = s->color[2];
        float a = s->color[3];

        for (int j = 0; j < vert_count; j++) {
            /* Normalize coordinates to 0-1 range for resolution-independent rendering */
            *ptr++ = shape_vertices[j * 2] / BASE_WIDTH;       /* normalized x */
            *ptr++ = shape_vertices[j * 2 + 1] / BASE_HEIGHT;  /* normalized y */
            *ptr++ = r;
            *ptr++ = g;
            *ptr++ = b;
            *ptr++ = a;
            current_vert++;
        }
    }

    s_uploaded_buf_size = (size_t)current_vert * 6 * sizeof(float);
    s_cached_shape_count = count;
    s_cached_shape_manager_revision = sm_get_revision();
    s_gpu_buffer_dirty = 1;
}

static int vertex_buffer_is_dirty(void)
{
    int count = sm_count();

    if (count != s_cached_shape_count) {
        return 1;
    }

    if (sm_get_revision() != s_cached_shape_manager_revision) {
        return 1;
    }

    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        if (!s || !s_shape_revisions || s_shape_revisions[i] != shape_get_revision(s)) {
            return 1;
        }
    }

    return 0;
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
    LOG_DEBUG_F("VAO created: %u", s_VAO);

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

    /* Initialize projection matrix */
    update_projection();

    LOG_DEBUG("Renderer initialized (dynamic line buffer)");
    return 0;
}

void renderer_on_viewport_change(void)
{
    update_projection();
}

void render_frame(void)
{
    GLenum err;
    SelectionManager* selection = app_state_get_selection();

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int count = sm_count();
    if (count == 0) {
        glDisable(GL_BLEND);
        return;
    }

    if (vertex_buffer_is_dirty()) {
        /* Rebuild vertex buffer from ShapeManager */
        rebuild_vertex_buffer();
    }

    if (!s_vertex_buf || s_vertex_buf_capacity == 0 || !s_draw_infos) {
        glDisable(GL_BLEND);
        return;
    }

    if (s_uploaded_buf_size == 0) {
        glDisable(GL_BLEND);
        return;
    }

    if (s_gpu_buffer_dirty) {
        glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
        glBufferData(GL_ARRAY_BUFFER, s_uploaded_buf_size, s_vertex_buf, GL_DYNAMIC_DRAW);
        s_gpu_buffer_dirty = 0;
    }

    /* Draw each shape with appropriate primitive type */
    shader_use();

    /* Set projection matrix uniform */
    GLint proj_loc = glGetUniformLocation(s_shader_program, "uProjection");
    if (proj_loc >= 0) {
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, s_projection);
    }

    glBindVertexArray(s_VAO);

    for (int i = 0; i < count; i++) {
        Shape* s = sm_get(i);
        GLenum mode = primitive_type_for_count(s_draw_infos[i].vert_count);
        float line_width = (s->line_width > 0.0f) ? s->line_width : 1.0f;

        /* Highlight selected shapes with yellow */
        int is_selected = selection && sel_contains(selection, s);
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
    glDisable(GL_BLEND);

    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR_F("GL error after draw: %d", err);
    }
}

void cleanup_renderer(void)
{
    glDeleteVertexArrays(1, &s_VAO);
    glDeleteBuffers(1, &s_VBO);
    SAFE_FREE(s_vertex_buf);
    SAFE_FREE(s_geometry_buf);
    SAFE_FREE(s_draw_infos);
    SAFE_FREE(s_shape_revisions);
    s_vertex_buf_capacity = 0;
    s_geometry_buf_capacity = 0;
    s_uploaded_buf_size = 0;
    s_gpu_buffer_dirty = 1;
    s_draw_info_capacity = 0;
    s_cached_shape_count = 0;
    s_cached_shape_manager_revision = 0;
    s_VAO = 0;
    s_VBO = 0;
    LOG_DEBUG("Renderer cleaned up");
}
