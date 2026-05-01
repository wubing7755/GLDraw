/**
 * @file render_system.c
 * @brief OpenGL-based canvas rendering implementation.
 *
 * Role in project:
 * - Compiles shaders, owns VAO/VBO buffers, and renders each frame.
 * - Draws grid, axes, document objects, and transient tool overlays.
 *
 * Module relationships:
 * - Consumes `Document` geometry and `CanvasView` transforms.
 * - Invoked by application once per frame.
 */
#include <render/render_system.h>

#include <base/file_utils.h>
#include <base/log.h>
#include <base/math2d.h>

#include <glad/glad.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RENDER_VERTEX_STRIDE_FLOATS 6u

/* Some GLAD builds do not expose KHR_debug macros/constants; provide safe fallbacks. */
#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif
#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#endif
#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#endif
#ifndef GLDEBUGPROC
typedef void (APIENTRY* GLDEBUGPROC)(GLenum source,
                                     GLenum type,
                                     GLuint id,
                                     GLenum severity,
                                     GLsizei length,
                                     const GLchar* message,
                                     const void* user_param);
#endif

/** Renderer-owned GL and CPU-side buffer state. */
struct RenderSystem {
    GLuint program;
    GLuint vao;
    GLuint vbo;
    GLint screen_size_loc;
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    float framebuffer_scale_x;
    float framebuffer_scale_y;
    float* vertex_buffer;
    size_t vertex_buffer_capacity;
    size_t gpu_vertex_buffer_capacity;
    Vec2* path_buffer;
    size_t path_buffer_capacity;
    GLenum current_primitive;
    float current_line_width;
    int batched_vertex_count;
    int frame_draw_calls;
    int frame_uploads;
    int debug_callback_enabled;
    unsigned int debug_frame_counter;
};

/**
 * @brief GL state touched by the custom canvas renderer and restored on exit.
 */
typedef struct RenderPassState {
    GLint viewport[4];
    GLint scissor_box[4];
    GLfloat clear_color[4];
    GLint current_program;
    GLint vertex_array_binding;
    GLint array_buffer_binding;
    GLint blend_src_rgb;
    GLint blend_dst_rgb;
    GLint blend_src_alpha;
    GLint blend_dst_alpha;
    GLfloat line_width;
    GLboolean scissor_enabled;
    GLboolean blend_enabled;
} RenderPassState;

/**
 * @brief Log any pending GL errors.
 * @param stage Label describing the stage where the error check occurs.
 * @return No return value.
 */
static void log_gl_error_if_any(const char* stage)
{
    GLenum error_code = glGetError();
    int has_error = 0;

    while (error_code != GL_NO_ERROR) {
        has_error = 1;
        LOG_ERROR("[render][gl_error] stage=%s code=0x%04X",
                  stage ? stage : "unknown",
                  (unsigned int)error_code);
        error_code = glGetError();
    }

    if (!has_error) {
        return;
    }
}

/**
 * @brief Capture GL state that this renderer mutates during one draw pass.
 * @param state [out] Snapshot of the current GL state.
 * @return No return value.
 */
static void render_capture_pass_state(RenderPassState* state)
{
    if (!state) {
        return;
    }

    glGetIntegerv(GL_VIEWPORT, state->viewport);
    glGetIntegerv(GL_SCISSOR_BOX, state->scissor_box);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, state->clear_color);
    glGetIntegerv(GL_CURRENT_PROGRAM, &state->current_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state->vertex_array_binding);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state->array_buffer_binding);
    glGetIntegerv(GL_BLEND_SRC_RGB, &state->blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &state->blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &state->blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &state->blend_dst_alpha);
    glGetFloatv(GL_LINE_WIDTH, &state->line_width);
    state->scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);
    state->blend_enabled = glIsEnabled(GL_BLEND);
}

/**
 * @brief Restore GL state captured before the render pass.
 * @param state Previously captured GL state.
 * @return No return value.
 */
static void render_restore_pass_state(const RenderPassState* state)
{
    if (!state) {
        return;
    }

    glViewport(state->viewport[0],
               state->viewport[1],
               state->viewport[2],
               state->viewport[3]);
    glScissor(state->scissor_box[0],
              state->scissor_box[1],
              state->scissor_box[2],
              state->scissor_box[3]);
    glClearColor(state->clear_color[0],
                 state->clear_color[1],
                 state->clear_color[2],
                 state->clear_color[3]);
    glUseProgram((GLuint)state->current_program);
    glBindVertexArray((GLuint)state->vertex_array_binding);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)state->array_buffer_binding);
    glBlendFuncSeparate((GLenum)state->blend_src_rgb,
                        (GLenum)state->blend_dst_rgb,
                        (GLenum)state->blend_src_alpha,
                        (GLenum)state->blend_dst_alpha);
    glLineWidth(state->line_width);

    if (state->scissor_enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    if (state->blend_enabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

/**
 * @brief Apply the explicit GL state contract required by the canvas renderer.
 * @param renderer Renderer instance.
 * @return No return value.
 */
static void render_prepare_pass_state(const RenderSystem* renderer)
{
    if (!renderer) {
        return;
    }

    glViewport(0, 0, renderer->framebuffer_width, renderer->framebuffer_height);
    glUseProgram(renderer->program);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief GL debug message callback.
 * @param source GL message source.
 * @param type GL message type.
 * @param id GL message ID.
 * @param severity GL message severity.
 * @param length Message length.
 * @param message Message string.
 * @param user_param User parameter.
 * @return No return value.
 */
#if !defined(NDEBUG)
static void APIENTRY render_debug_callback(GLenum source,
                                           GLenum type,
                                           GLuint id,
                                           GLenum severity,
                                           GLsizei length,
                                           const GLchar* message,
                                           const void* user_param)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)user_param;

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    LOG_ERROR("[render][debug] severity=0x%04X id=%u message=%s",
              (unsigned int)severity,
              (unsigned int)id,
              message ? (const char*)message : "(null)");
}
#endif

/**
 * @brief Log frame statistics every 120 frames (debug builds only).
 * @param renderer Renderer instance.
 * @return No return value.
 */
static void render_log_frame_stats(const RenderSystem* renderer)
{
#if !defined(NDEBUG)
    const unsigned int stats_period_frames = 120u;

    if (!renderer) {
        return;
    }
    if ((renderer->debug_frame_counter % stats_period_frames) != 0u) {
        return;
    }

    LOG_INFO("[render][frame_stats] draws=%d uploads=%d batched_vertices=%d debug_cb=%d",
             renderer->frame_draw_calls,
             renderer->frame_uploads,
             renderer->batched_vertex_count,
             renderer->debug_callback_enabled);
#else
    (void)renderer;
#endif
}

/**
 * @brief Compile a shader from source string.
 * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
 * @param source Shader source code.
 * @param label Human-readable label for error messages.
 * @return Shader handle on success, `0` on failure.
 */
static GLuint compile_shader(GLenum type, const char* source, const char* label)
{
    GLuint shader = glCreateShader(type);
    GLint success = 0;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(shader, (GLsizei)sizeof(info), NULL, info);
        LOG_ERROR("Shader compilation failed for %s: %s",
                  label ? label : "unknown stage",
                  info);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

/**
 * @brief Load a shader program from vertex and fragment source files.
 * @param vertex_path Path to vertex shader file.
 * @param fragment_path Path to fragment shader file.
 * @return Program handle on success, `0` on failure.
 */
static GLuint load_program(const char* vertex_path, const char* fragment_path)
{
    char* vertex_source = file_utils_read_text_file(vertex_path);
    char* fragment_source = file_utils_read_text_file(fragment_path);
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint program = 0;
    GLint success = 0;

    if (!vertex_source || !fragment_source) {
        LOG_ERROR("[render][load_program][read_sources] Failed to read shader sources: vertex=%s fragment=%s",
                  vertex_path ? vertex_path : "(null)",
                  fragment_path ? fragment_path : "(null)");
        free(vertex_source);
        free(fragment_source);
        return 0;
    }

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source, vertex_path);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source, fragment_path);
    free(vertex_source);
    free(fragment_source);

    if (!vertex_shader || !fragment_shader) {
        if (vertex_shader) glDeleteShader(vertex_shader);
        if (fragment_shader) glDeleteShader(fragment_shader);
        return 0;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (!success) {
        char info[1024];
        glGetProgramInfoLog(program, (GLsizei)sizeof(info), NULL, info);
        LOG_ERROR("[render][load_program][link_program] Program link failed: %s (vertex=%s fragment=%s)",
                  info,
                  vertex_path ? vertex_path : "(null)",
                  fragment_path ? fragment_path : "(null)");
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

/**
 * @brief Ensure CPU screen-vertex buffer can store requested vertex count.
 * @return `1` on success, `0` on allocation failure.
 *
 * Risk note:
 * - Uses `realloc`; on failure old buffers remain valid and unchanged.
 */

/**
 * @brief Ensure CPU screen-vertex buffer can store requested vertex count.
 * @param renderer Renderer instance.
 * @param vertex_count Requested vertex count.
 * @return `1` on success, `0` on allocation failure.
 *
 * Risk note:
 * - Uses `realloc`; on failure old buffers remain valid and unchanged.
 */
static int ensure_screen_vertex_capacity(RenderSystem* renderer, size_t vertex_count)
{
    size_t required_vertex_capacity = vertex_count * RENDER_VERTEX_STRIDE_FLOATS;

    if (required_vertex_capacity > renderer->vertex_buffer_capacity) {
        float* new_buffer = (float*)realloc(renderer->vertex_buffer, required_vertex_capacity * sizeof(float));
        if (!new_buffer) {
            return 0;
        }
        renderer->vertex_buffer = new_buffer;
        renderer->vertex_buffer_capacity = required_vertex_capacity;
    }

    return 1;
}

/**
 * @brief Ensure path buffer can store requested point count.
 * @param renderer Renderer instance.
 * @param point_count Requested point count.
 * @return `1` on success, `0` on allocation failure.
 */
static int ensure_path_capacity(RenderSystem* renderer, size_t point_count)
{
    if (point_count > renderer->path_buffer_capacity) {
        Vec2* new_points = (Vec2*)realloc(renderer->path_buffer, point_count * sizeof(Vec2));
        if (!new_points) {
            return 0;
        }
        renderer->path_buffer = new_points;
        renderer->path_buffer_capacity = point_count;
    }

    return 1;
}

/**
 * @brief Ensure GPU VBO can store requested vertex count.
 * @param renderer Renderer instance.
 * @param vertex_count Requested vertex count.
 * @return `1` on success, `0` if renderer is `NULL`.
 */
static int ensure_gpu_vertex_capacity(RenderSystem* renderer, size_t vertex_count)
{
    size_t required_vertex_capacity = vertex_count * RENDER_VERTEX_STRIDE_FLOATS;

    if (!renderer) {
        return 0;
    }

    if (required_vertex_capacity > renderer->gpu_vertex_buffer_capacity) {
        glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(required_vertex_capacity * sizeof(float)),
                     NULL,
                     GL_DYNAMIC_DRAW);
        renderer->gpu_vertex_buffer_capacity = required_vertex_capacity;
    }

    return 1;
}

/**
 * @brief Write a vertex into the CPU buffer at the cursor position.
 * @param cursor [in,out] Write cursor pointer.
 * @param screen Screen coordinate.
 * @param color Vertex color.
 * @return No return value.
 */
static void write_screen_vertex(float** cursor, Vec2 screen, Color color)
{
    *(*cursor)++ = screen.x;
    *(*cursor)++ = screen.y;
    *(*cursor)++ = color.r;
    *(*cursor)++ = color.g;
    *(*cursor)++ = color.b;
    *(*cursor)++ = color.a;
}

/**
 * @brief Convert logical screen coordinates to framebuffer coordinates.
 * @param renderer Renderer instance.
 * @param logical_screen Screen coordinate in logical window space.
 * @return Screen coordinate in framebuffer space.
 */
static Vec2 render_framebuffer_point(const RenderSystem* renderer, Vec2 logical_screen)
{
    Vec2 framebuffer_screen = logical_screen;

    if (!renderer) {
        return framebuffer_screen;
    }

    framebuffer_screen.x *= renderer->framebuffer_scale_x;
    framebuffer_screen.y *= renderer->framebuffer_scale_y;
    return framebuffer_screen;
}

/**
 * @brief Normalize primitive type for batching.
 * @param primitive GL primitive type.
 * @return GL primitive type suitable for batching.
 */
static GLenum batch_primitive_for(GLenum primitive)
{
    return (primitive == GL_LINE_STRIP) ? GL_LINES : primitive;
}

/**
 * @brief Compute how many vertices a primitive type will add to the batch.
 * @param primitive GL primitive type.
 * @param point_count Number of points.
 * @return Number of vertices that will be batched.
 */
static int batch_vertex_count_for(GLenum primitive, int point_count)
{
    if (point_count <= 0) {
        return 0;
    }

    if (primitive == GL_LINE_STRIP) {
        return (point_count > 1) ? (point_count - 1) * 2 : 0;
    }

    return point_count;
}

/**
 * @brief Upload batched vertices to GPU.
 * @param renderer Renderer instance.
 * @param vertex_count Number of vertices to upload.
 * @return `1` on success, `0` on failure.
 */
static int upload_vertices(RenderSystem* renderer, int vertex_count)
{
    if (!renderer || vertex_count <= 0) {
        return 0;
    }

    if (!ensure_gpu_vertex_capacity(renderer, (size_t)vertex_count)) {
        return 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    (GLsizeiptr)(vertex_count * RENDER_VERTEX_STRIDE_FLOATS * sizeof(float)),
                    renderer->vertex_buffer);
    return 1;
}

/**
 * @brief Begin a new frame batch, resetting batch state.
 * @param renderer Renderer instance.
 * @return No return value.
 */
static void begin_frame_batch(RenderSystem* renderer)
{
    if (!renderer) {
        return;
    }

    renderer->current_primitive = 0;
    renderer->current_line_width = 1.0f;
    renderer->batched_vertex_count = 0;
    renderer->frame_draw_calls = 0;
    renderer->frame_uploads = 0;
}

/**
 * @brief Check whether a new batch can be appended to the current batch.
 * @param renderer Renderer instance.
 * @param primitive GL primitive type.
 * @param line_width Line width.
 * @param append_vertices Number of vertices to append.
 * @return `1` if appendable, `0` otherwise.
 */
static int can_append_line_batch(RenderSystem* renderer,
                                 GLenum primitive,
                                 float line_width,
                                 int append_vertices)
{
    GLenum batch_primitive = batch_primitive_for(primitive);
    float effective_line_width = (line_width > 0.5f) ? line_width : 1.0f;

    if (!renderer || append_vertices <= 0) {
        return 0;
    }

    if (renderer->batched_vertex_count <= 0) {
        return 1;
    }

    return renderer->current_primitive == batch_primitive &&
           fabsf(renderer->current_line_width - effective_line_width) <= 0.01f;
}

/**
 * @brief Submit and draw the current batched cache.
 * @param renderer [in,out] Renderer.
 * @return No return value.
 *
 * @note Only triggers upload and draw when `batched_vertex_count > 0`, then resets batch state.
 */
static void flush_batch(RenderSystem* renderer)
{
    if (!renderer || renderer->batched_vertex_count <= 0) {
        return;
    }

    if (upload_vertices(renderer, renderer->batched_vertex_count)) {
        glLineWidth(renderer->current_line_width);
        glDrawArrays(renderer->current_primitive, 0, renderer->batched_vertex_count);
        renderer->frame_draw_calls++;
        renderer->frame_uploads++;
    }

    renderer->current_primitive = 0;
    renderer->current_line_width = 1.0f;
    renderer->batched_vertex_count = 0;
}

/**
 * @brief Add line vertices to the current batch.
 * @param renderer Renderer instance.
 * @param canvas Canvas view.
 * @param points Point array.
 * @param count Point count.
 * @param color Line color.
 * @param primitive GL primitive type.
 * @param line_width Line width.
 * @return `1` on success, `0` on failure.
 */
static int append_line_vertices(RenderSystem* renderer,
                                const CanvasView* canvas,
                                const Vec2* points,
                                int count,
                                Color color,
                                GLenum primitive,
                                float line_width)
{
    GLenum batch_primitive = batch_primitive_for(primitive);
    float effective_line_width = (line_width > 0.5f) ? line_width : 1.0f;
    int append_vertices = batch_vertex_count_for(primitive, count);
    float* cursor = NULL;
    int i = 0;

    if (!renderer || !canvas || !points || append_vertices <= 0) {
        return 0;
    }

    if (renderer->batched_vertex_count <= 0) {
        renderer->current_primitive = batch_primitive;
        renderer->current_line_width = effective_line_width;
    }

    if (!ensure_screen_vertex_capacity(renderer,
                                       (size_t)(renderer->batched_vertex_count + append_vertices))) {
        return 0;
    }

    cursor = renderer->vertex_buffer +
             ((size_t)renderer->batched_vertex_count * RENDER_VERTEX_STRIDE_FLOATS);

    if (primitive == GL_LINE_STRIP) {
        for (i = 0; i < count - 1; ++i) {
            write_screen_vertex(&cursor,
                                render_framebuffer_point(renderer,
                                                         canvas_view_world_to_screen(canvas, points[i])),
                                color);
            write_screen_vertex(&cursor,
                                render_framebuffer_point(renderer,
                                                         canvas_view_world_to_screen(canvas, points[i + 1])),
                                color);
        }
    } else {
        for (i = 0; i < count; ++i) {
            write_screen_vertex(&cursor,
                                render_framebuffer_point(renderer,
                                                         canvas_view_world_to_screen(canvas, points[i])),
                                color);
        }
    }

    renderer->batched_vertex_count += append_vertices;
    return 1;
}

/**
 * @brief End the frame batch, flushing any remaining batched content.
 * @param renderer Renderer instance.
 * @return No return value.
 */
static void end_frame_batch(RenderSystem* renderer)
{
    flush_batch(renderer);
}

/**
 * @brief Draw a polyline/line-strip path.
 * @param renderer [in,out] Renderer.
 * @param canvas [in] Canvas view.
 * @param points [in] Path point array.
 * @param count [in] Point count.
 * @param color [in] Line color.
 * @param line_width [in] Line width.
 * @return No return value.
 */
static void draw_polyline(RenderSystem* renderer,
                          const CanvasView* canvas,
                          const Vec2* points,
                          int count,
                          Color color,
                          float line_width)
{
    int append_vertices = batch_vertex_count_for(GL_LINE_STRIP, count);

    if (append_vertices <= 0) {
        return;
    }

    if (!can_append_line_batch(renderer, GL_LINE_STRIP, line_width, append_vertices)) {
        flush_batch(renderer);
    }

    append_line_vertices(renderer, canvas, points, count, color, GL_LINE_STRIP, line_width);
}

/**
 * @brief Count grid lines between start and end at the given spacing.
 * @param start Start coordinate.
 * @param end End coordinate.
 * @param spacing Grid spacing.
 * @return Number of grid lines.
 */
static int count_grid_lines(float start, float end, float spacing)
{
    int count = 0;
    float value = start;
    float limit = end + spacing * 0.5f;

    if (spacing <= 0.0f) {
        return 0;
    }

    while (value <= limit) {
        count++;
        value += spacing;
    }

    return count;
}

/**
 * @brief Draw the grid and axes for the current visible area.
 * @param renderer [in,out] Renderer.
 * @param canvas [in] Canvas view.
 * @return No return value.
 *
 * Algorithm:
 * 1. Compute visible world rectangle and grid start/end coordinates;
 * 2. Batch-generate vertical/horizontal grid line segment endpoints;
 * 3. Append to line batch;
 * 4. Also draw X/Y axis highlight lines.
 */
static void draw_grid(RenderSystem* renderer, const CanvasView* canvas)
{
    RectF visible = canvas_view_visible_world_rect(canvas);
    Color grid_color = {0.20f, 0.22f, 0.25f, 1.0f};
    Color axis_color = {0.32f, 0.35f, 0.40f, 1.0f};
    float spacing = 100.0f;
    float start_x = floorf(visible.x / spacing) * spacing;
    float end_x = rectf_right(&visible);
    float start_y = floorf(visible.y / spacing) * spacing;
    float end_y = rectf_top(&visible);
    int vertical_count = count_grid_lines(start_x, end_x, spacing);
    int horizontal_count = count_grid_lines(start_y, end_y, spacing);
    int grid_vertex_count = (vertical_count + horizontal_count) * 2;
    Vec2 axis_points[4];
    int point_index = 0;
    float x = 0.0f;
    float y = 0.0f;

    if (grid_vertex_count > 0 &&
        ensure_path_capacity(renderer, (size_t)grid_vertex_count)) {
        for (x = start_x; x <= end_x + spacing * 0.5f; x += spacing) {
            renderer->path_buffer[point_index++] = vec2_make(x, visible.y);
            renderer->path_buffer[point_index++] = vec2_make(x, visible.y + visible.h);
        }

        for (y = start_y; y <= end_y + spacing * 0.5f; y += spacing) {
            renderer->path_buffer[point_index++] = vec2_make(visible.x, y);
            renderer->path_buffer[point_index++] = vec2_make(visible.x + visible.w, y);
        }

        if (!can_append_line_batch(renderer, GL_LINES, 1.0f, grid_vertex_count)) {
            flush_batch(renderer);
        }

        append_line_vertices(renderer,
                             canvas,
                             renderer->path_buffer,
                             grid_vertex_count,
                             grid_color,
                             GL_LINES,
                             1.0f);
    }

    axis_points[0] = vec2_make(visible.x, 0.0f);
    axis_points[1] = vec2_make(visible.x + visible.w, 0.0f);
    axis_points[2] = vec2_make(0.0f, visible.y);
    axis_points[3] = vec2_make(0.0f, visible.y + visible.h);

    if (!can_append_line_batch(renderer, GL_LINES, 1.5f, 4)) {
        flush_batch(renderer);
    }

    append_line_vertices(renderer, canvas, axis_points, 4, axis_color, GL_LINES, 1.5f);
}

/**
 * @brief Draw a single graphic object.
 * @param renderer Renderer instance.
 * @param canvas Canvas view.
 * @param object Object to draw.
 * @param selected Whether the object is selected (adds highlight).
 * @return No return value.
 */
static void draw_object(RenderSystem* renderer,
                        const CanvasView* canvas,
                        const GraphicObject* object,
                        int selected)
{
    int point_count = object_get_path_point_count(object);
    Color highlight = {0.98f, 0.86f, 0.24f, 1.0f};

    if (point_count <= 1 || !ensure_path_capacity(renderer, (size_t)point_count)) {
        return;
    }

    object_write_path_points(object, renderer->path_buffer);
    if (selected) {
        draw_polyline(renderer, canvas, renderer->path_buffer, point_count, highlight, object->style.stroke_width + 3.0f);
    }
    draw_polyline(renderer, canvas, renderer->path_buffer, point_count, object->style.stroke_color, object->style.stroke_width);
}

/**
 * @brief Convert canvas viewport to GL scissor box.
 * @return `1` when resulting scissor area is valid and non-empty, else `0`.
 */

/**
 * @brief Convert canvas viewport to GL scissor box.
 * @param viewport Canvas viewport rectangle.
 * @param framebuffer_width Framebuffer width.
 * @param framebuffer_height Framebuffer height.
 * @param out_scissor [out] Output scissor box (x, y, w, h).
 * @return `1` when resulting scissor area is valid and non-empty, else `0`.
 */
static int render_canvas_scissor_box(const RectF* viewport,
                                     float framebuffer_scale_x,
                                     float framebuffer_scale_y,
                                     int framebuffer_width,
                                     int framebuffer_height,
                                     GLint out_scissor[4])
{
    int x = 0;
    int y_top = 0;
    int w = 0;
    int h = 0;
    int y = 0;

    if (!viewport || !out_scissor || framebuffer_width <= 0 || framebuffer_height <= 0) {
        return 0;
    }

    if (viewport->w <= 0.0f || viewport->h <= 0.0f) {
        return 0;
    }

    if (framebuffer_scale_x <= 0.0f || framebuffer_scale_y <= 0.0f) {
        return 0;
    }

    x = (int)floorf(viewport->x * framebuffer_scale_x);
    y_top = (int)floorf(viewport->y * framebuffer_scale_y);
    w = (int)ceilf(viewport->w * framebuffer_scale_x);
    h = (int)ceilf(viewport->h * framebuffer_scale_y);
    y = framebuffer_height - y_top - h;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > framebuffer_width) {
        w = framebuffer_width - x;
    }
    if (y + h > framebuffer_height) {
        h = framebuffer_height - y;
    }
    if (w <= 0 || h <= 0) {
        return 0;
    }

    out_scissor[0] = (GLint)x;
    out_scissor[1] = (GLint)y;
    out_scissor[2] = (GLint)w;
    out_scissor[3] = (GLint)h;
    return 1;
}

static unsigned int png_crc32_update(unsigned int crc,
                                     const unsigned char* data,
                                     size_t size)
{
    size_t i = 0u;
    int bit = 0;

    crc = ~crc;
    for (i = 0u; i < size; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }

    return ~crc;
}

static unsigned int png_adler32(const unsigned char* data, size_t size)
{
    const unsigned int mod_adler = 65521u;
    unsigned int a = 1u;
    unsigned int b = 0u;
    size_t i = 0u;

    for (i = 0u; i < size; ++i) {
        a = (a + data[i]) % mod_adler;
        b = (b + a) % mod_adler;
    }

    return (b << 16) | a;
}

static int png_write_u32_be(FILE* file, unsigned int value)
{
    unsigned char bytes[4];

    if (!file) {
        return 0;
    }

    bytes[0] = (unsigned char)((value >> 24) & 0xFFu);
    bytes[1] = (unsigned char)((value >> 16) & 0xFFu);
    bytes[2] = (unsigned char)((value >> 8) & 0xFFu);
    bytes[3] = (unsigned char)(value & 0xFFu);
    return fwrite(bytes, 1u, sizeof(bytes), file) == sizeof(bytes);
}

static int png_write_chunk(FILE* file,
                           const char type[4],
                           const unsigned char* data,
                           size_t size)
{
    unsigned int crc = 0u;

    if (!file || !type || size > 0xFFFFFFFFu) {
        return 0;
    }

    if (!png_write_u32_be(file, (unsigned int)size)) {
        return 0;
    }
    if (fwrite(type, 1u, 4u, file) != 4u) {
        return 0;
    }
    if (size > 0u && fwrite(data, 1u, size, file) != size) {
        return 0;
    }

    crc = png_crc32_update(0u, (const unsigned char*)type, 4u);
    if (size > 0u) {
        crc = png_crc32_update(crc, data, size);
    }
    return png_write_u32_be(file, crc);
}

static int png_build_uncompressed_zlib(const unsigned char* data,
                                       size_t data_size,
                                       unsigned char** out_data,
                                       size_t* out_size)
{
    size_t block_count = 0u;
    size_t capacity = 0u;
    size_t source_offset = 0u;
    size_t write_offset = 0u;
    unsigned char* output = NULL;
    unsigned int adler = 0u;

    if (!data || !out_data || !out_size) {
        return 0;
    }

    block_count = (data_size + 65534u) / 65535u;
    capacity = 2u + data_size + block_count * 5u + 4u;
    output = (unsigned char*)malloc(capacity);
    if (!output) {
        return 0;
    }

    output[write_offset++] = 0x78u;
    output[write_offset++] = 0x01u;

    while (source_offset < data_size || (data_size == 0u && source_offset == 0u)) {
        size_t remaining = data_size - source_offset;
        unsigned int block_size = (remaining > 65535u) ? 65535u : (unsigned int)remaining;
        unsigned int final_block = (source_offset + block_size >= data_size) ? 1u : 0u;

        output[write_offset++] = (unsigned char)final_block;
        output[write_offset++] = (unsigned char)(block_size & 0xFFu);
        output[write_offset++] = (unsigned char)((block_size >> 8) & 0xFFu);
        output[write_offset++] = (unsigned char)((~block_size) & 0xFFu);
        output[write_offset++] = (unsigned char)(((~block_size) >> 8) & 0xFFu);

        if (block_size > 0u) {
            memcpy(output + write_offset, data + source_offset, block_size);
            write_offset += block_size;
            source_offset += block_size;
        } else {
            break;
        }
    }

    adler = png_adler32(data, data_size);
    output[write_offset++] = (unsigned char)((adler >> 24) & 0xFFu);
    output[write_offset++] = (unsigned char)((adler >> 16) & 0xFFu);
    output[write_offset++] = (unsigned char)((adler >> 8) & 0xFFu);
    output[write_offset++] = (unsigned char)(adler & 0xFFu);

    *out_data = output;
    *out_size = write_offset;
    return 1;
}

static int png_write_rgba_file(const char* path,
                               const unsigned char* rgba_bottom_up,
                               int width,
                               int height)
{
    static const unsigned char signature[8] = {137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u};
    FILE* file = NULL;
    unsigned char ihdr[13];
    unsigned char* filtered = NULL;
    unsigned char* zlib_data = NULL;
    size_t filtered_stride = 0u;
    size_t filtered_size = 0u;
    size_t zlib_size = 0u;
    int row = 0;
    int ok = 0;

    if (!path || !rgba_bottom_up || width <= 0 || height <= 0) {
        return 0;
    }

    filtered_stride = (size_t)width * 4u + 1u;
    filtered_size = filtered_stride * (size_t)height;
    filtered = (unsigned char*)malloc(filtered_size);
    if (!filtered) {
        return 0;
    }

    for (row = 0; row < height; ++row) {
        const unsigned char* source =
            rgba_bottom_up + (size_t)(height - 1 - row) * (size_t)width * 4u;
        unsigned char* target = filtered + (size_t)row * filtered_stride;

        target[0] = 0u;
        memcpy(target + 1u, source, (size_t)width * 4u);
    }

    if (!png_build_uncompressed_zlib(filtered, filtered_size, &zlib_data, &zlib_size)) {
        free(filtered);
        return 0;
    }

    file = fopen(path, "wb");
    if (!file) {
        free(zlib_data);
        free(filtered);
        return 0;
    }

    memset(ihdr, 0, sizeof(ihdr));
    ihdr[0] = (unsigned char)(((unsigned int)width >> 24) & 0xFFu);
    ihdr[1] = (unsigned char)(((unsigned int)width >> 16) & 0xFFu);
    ihdr[2] = (unsigned char)(((unsigned int)width >> 8) & 0xFFu);
    ihdr[3] = (unsigned char)((unsigned int)width & 0xFFu);
    ihdr[4] = (unsigned char)(((unsigned int)height >> 24) & 0xFFu);
    ihdr[5] = (unsigned char)(((unsigned int)height >> 16) & 0xFFu);
    ihdr[6] = (unsigned char)(((unsigned int)height >> 8) & 0xFFu);
    ihdr[7] = (unsigned char)((unsigned int)height & 0xFFu);
    ihdr[8] = 8u;
    ihdr[9] = 6u;

    ok = fwrite(signature, 1u, sizeof(signature), file) == sizeof(signature) &&
         png_write_chunk(file, "IHDR", ihdr, sizeof(ihdr)) &&
         png_write_chunk(file, "IDAT", zlib_data, zlib_size) &&
         png_write_chunk(file, "IEND", NULL, 0u);

    if (fclose(file) != 0) {
        ok = 0;
    }

    free(zlib_data);
    free(filtered);
    return ok;
}

/**
 * @brief Create the rendering system and initialize GPU resources.
 * @param window Already-initialized platform window.
 * @return Renderer instance on success, `NULL` on failure.
 */
RenderSystem* render_system_create(PlatformWindow* window)
{
    RenderSystem* renderer = (RenderSystem*)calloc(1, sizeof(*renderer));

    if (!renderer) {
        return NULL;
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        free(renderer);
        return NULL;
    }

    renderer->debug_callback_enabled = 0;

#if !defined(NDEBUG)
    {
        int has_debug_support = 0;
        typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKPROC_LOCAL)(GLDEBUGPROC callback, const void* user_param);
        PFNGLDEBUGMESSAGECALLBACKPROC_LOCAL debug_message_callback = NULL;

#ifdef GLAD_GL_KHR_debug
        has_debug_support = has_debug_support || GLAD_GL_KHR_debug;
#endif
#ifdef GLAD_GL_VERSION_4_3
        has_debug_support = has_debug_support || GLAD_GL_VERSION_4_3;
#endif

        if (has_debug_support) {
            debug_message_callback = (PFNGLDEBUGMESSAGECALLBACKPROC_LOCAL)glfwGetProcAddress("glDebugMessageCallback");
            if (!debug_message_callback) {
                debug_message_callback = (PFNGLDEBUGMESSAGECALLBACKPROC_LOCAL)glfwGetProcAddress("glDebugMessageCallbackKHR");
            }
        }

        if (debug_message_callback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            debug_message_callback(render_debug_callback, NULL);
            renderer->debug_callback_enabled = 1;
        }
    }
#endif

    renderer->logical_width = window->width;
    renderer->logical_height = window->height;
    renderer->framebuffer_width = window->framebuffer_width;
    renderer->framebuffer_height = window->framebuffer_height;
    renderer->framebuffer_scale_x =
        (window->width > 0) ? ((float)window->framebuffer_width / (float)window->width) : 1.0f;
    renderer->framebuffer_scale_y =
        (window->height > 0) ? ((float)window->framebuffer_height / (float)window->height) : 1.0f;
    renderer->program = load_program("shaders/basic.vert", "shaders/basic.frag");
    if (!renderer->program) {
        free(renderer);
        return NULL;
    }

    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glUseProgram(renderer->program);
    renderer->screen_size_loc = glGetUniformLocation(renderer->program, "uScreenSize");
    if (renderer->screen_size_loc >= 0) {
        glUniform2f(renderer->screen_size_loc,
                    (float)renderer->framebuffer_width,
                    (float)renderer->framebuffer_height);
    }
    glUseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return renderer;
}

/**
 * @brief Destroy the rendering system and release GPU/heap resources.
 * @param renderer Renderer instance.
 * @return No return value.
 */
void render_system_destroy(RenderSystem* renderer)
{
    if (!renderer) {
        return;
    }

    glDeleteBuffers(1, &renderer->vbo);
    glDeleteVertexArrays(1, &renderer->vao);
    glDeleteProgram(renderer->program);
    free(renderer->vertex_buffer);
    free(renderer->path_buffer);
    free(renderer);
}

/**
 * @brief Handle window resize events.
 * @param renderer Renderer instance.
 * @param width New width in pixels.
 * @param height New height in pixels.
 * @return No return value.
 */
void render_system_resize(RenderSystem* renderer,
                          int logical_width,
                          int logical_height,
                          int framebuffer_width,
                          int framebuffer_height)
{
    if (!renderer) {
        return;
    }

    renderer->logical_width = logical_width;
    renderer->logical_height = logical_height;
    renderer->framebuffer_width = framebuffer_width;
    renderer->framebuffer_height = framebuffer_height;
    renderer->framebuffer_scale_x =
        (logical_width > 0) ? ((float)framebuffer_width / (float)logical_width) : 1.0f;
    renderer->framebuffer_scale_y =
        (logical_height > 0) ? ((float)framebuffer_height / (float)logical_height) : 1.0f;
    glViewport(0, 0, framebuffer_width, framebuffer_height);
    glUseProgram(renderer->program);
    if (renderer->screen_size_loc >= 0) {
        glUniform2f(renderer->screen_size_loc,
                    (float)framebuffer_width,
                    (float)framebuffer_height);
    }
    glUseProgram(0);
}

/**
 * @brief Render full canvas frame.
 * @param renderer [in,out] Renderer.
 * @param document [in] Document object collection.
 * @param canvas [in] Canvas view.
 * @param overlay_object [in] Tool overlay preview object (may be `NULL`).
 * @return No return value.
 *
 * Why preserve scissor state:
 * - UI and renderer may share context state; previous scissor state is restored
 *   to avoid leaking render-state changes into other draw passes.
 */
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const SelectionSet* selection,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object)
{
    int i = 0;
    RenderPassState pass_state;
    GLint canvas_scissor_box[4] = {0, 0, 0, 0};
    RectF viewport;
    int has_canvas_area = 0;

    if (!renderer || !document || !canvas) {
        return;
    }

    viewport = canvas_view_viewport(canvas);
    has_canvas_area = render_canvas_scissor_box(&viewport,
                                                renderer->framebuffer_scale_x,
                                                renderer->framebuffer_scale_y,
                                                renderer->framebuffer_width,
                                                renderer->framebuffer_height,
                                                canvas_scissor_box);

    render_capture_pass_state(&pass_state);
    render_prepare_pass_state(renderer);

    if (!has_canvas_area) {
        render_restore_pass_state(&pass_state);
        return;
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(canvas_scissor_box[0], canvas_scissor_box[1], canvas_scissor_box[2], canvas_scissor_box[3]);
    glClearColor(canvas->background.r, canvas->background.g, canvas->background.b, canvas->background.a);
    glClear(GL_COLOR_BUFFER_BIT);
    log_gl_error_if_any("render_system_draw:after_clear");

    glUseProgram(renderer->program);
    glBindVertexArray(renderer->vao);
    begin_frame_batch(renderer);

    if (canvas->show_grid) {
        draw_grid(renderer, canvas);
    }

    for (i = 0; i < document->count; ++i) {
        const GraphicObject* object = document->objects[i];
        int selected = selection_set_contains(selection, object->id);
        draw_object(renderer, canvas, object, selected);
    }

    if (overlay_object) {
        draw_object(renderer, canvas, overlay_object, 0);
    }

    end_frame_batch(renderer);
    log_gl_error_if_any("render_system_draw:after_batch_flush");
    log_gl_error_if_any("render_system_draw:before_finalize");

    renderer->debug_frame_counter++;
    render_log_frame_stats(renderer);

    render_restore_pass_state(&pass_state);
}

int render_system_export_png(RenderSystem* renderer,
                             const CanvasView* canvas,
                             const char* path)
{
    RectF viewport;
    GLint canvas_box[4] = {0, 0, 0, 0};
    unsigned char* pixels = NULL;
    GLint previous_pack_alignment = 4;
    int pixel_count = 0;
    int ok = 0;

    if (!renderer || !canvas || !path || path[0] == '\0') {
        return 0;
    }

    viewport = canvas_view_viewport(canvas);
    if (!render_canvas_scissor_box(&viewport,
                                   renderer->framebuffer_scale_x,
                                   renderer->framebuffer_scale_y,
                                   renderer->framebuffer_width,
                                   renderer->framebuffer_height,
                                   canvas_box)) {
        return 0;
    }

    if (canvas_box[2] <= 0 || canvas_box[3] <= 0) {
        return 0;
    }

    pixel_count = canvas_box[2] * canvas_box[3];
    pixels = (unsigned char*)malloc((size_t)pixel_count * 4u);
    if (!pixels) {
        return 0;
    }

    glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(canvas_box[0],
                 canvas_box[1],
                 canvas_box[2],
                 canvas_box[3],
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);

    if (glGetError() == GL_NO_ERROR) {
        ok = png_write_rgba_file(path, pixels, canvas_box[2], canvas_box[3]);
    }

    free(pixels);
    return ok;
}
