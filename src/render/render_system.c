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

#include <base/log.h>
#include <base/math2d.h>

#include <glad/glad.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
    int width;
    int height;
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
 * @brief log_gl_error_if_any 函数。
 *
 * @param stage 参数 `stage`。
 * @return 无。
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
 * @brief render_debug_callback 函数。
 *
 * @param source 参数 `source`。
 * @param type 参数 `type`。
 * @param id 参数 `id`。
 * @param severity 参数 `severity`。
 * @param length 参数 `length`。
 * @param message 参数 `message`。
 * @param user_param 参数 `user_param`。
 * @return 函数返回值。
 */
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

#if !defined(NDEBUG)
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    LOG_ERROR("[render][debug] severity=0x%04X id=%u message=%s",
              (unsigned int)severity,
              (unsigned int)id,
              message ? (const char*)message : "(null)");
#else
    (void)severity;
    (void)message;
#endif
}

/**
 * @brief render_log_frame_stats 函数。
 *
 * @param renderer 参数 `renderer`。
 * @return 无。
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
 * @brief read_text_file 函数。
 *
 * @param path 参数 `path`。
 * @return 函数返回值。
 */
static char* read_text_file(const char* path)
{
    FILE* file = fopen(path, "rb");
    char* buffer = NULL;
    long length = 0;
    size_t read_count = 0;

    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = (char*)malloc((size_t)length + 1u);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    read_count = fread(buffer, 1u, (size_t)length, file);
    buffer[read_count] = '\0';
    fclose(file);
    return buffer;
}

/**
 * @brief compile_shader 函数。
 *
 * @param type 参数 `type`。
 * @param source 参数 `source`。
 * @param label 参数 `label`。
 * @return 函数返回值。
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
 * @brief load_program 函数。
 *
 * @param vertex_path 参数 `vertex_path`。
 * @param fragment_path 参数 `fragment_path`。
 * @return 函数返回值。
 */
static GLuint load_program(const char* vertex_path, const char* fragment_path)
{
    char* vertex_source = read_text_file(vertex_path);
    char* fragment_source = read_text_file(fragment_path);
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
 * @brief ensure_screen_vertex_capacity 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param vertex_count 参数 `vertex_count`。
 * @return 函数返回值。
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
 * @brief ensure_path_capacity 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param point_count 参数 `point_count`。
 * @return 函数返回值。
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
 * @brief ensure_gpu_vertex_capacity 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param vertex_count 参数 `vertex_count`。
 * @return 函数返回值。
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
 * @brief write_screen_vertex 函数。
 *
 * @param cursor 参数 `cursor`。
 * @param screen 参数 `screen`。
 * @param color 参数 `color`。
 * @return 无。
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
 * @brief batch_primitive_for 函数。
 *
 * @param primitive 参数 `primitive`。
 * @return 函数返回值。
 */
static GLenum batch_primitive_for(GLenum primitive)
{
    return (primitive == GL_LINE_STRIP) ? GL_LINES : primitive;
}

/**
 * @brief batch_vertex_count_for 函数。
 *
 * @param primitive 参数 `primitive`。
 * @param point_count 参数 `point_count`。
 * @return 函数返回值。
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
 * @brief upload_vertices 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param vertex_count 参数 `vertex_count`。
 * @return 函数返回值。
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
 * @brief begin_frame_batch 函数。
 *
 * @param renderer 参数 `renderer`。
 * @return 无。
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
 * @brief can_append_line_batch 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param primitive 参数 `primitive`。
 * @param line_width 参数 `line_width`。
 * @param append_vertices 参数 `append_vertices`。
 * @return 函数返回值。
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
 * @brief 提交并绘制当前缓存批次。
 * @param renderer [in,out] 渲染器。
 * @return 无。
 *
 * @note 仅当 `batched_vertex_count > 0` 时触发上传与绘制，随后重置批次状态。
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
 * @brief append_line_vertices 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param canvas 参数 `canvas`。
 * @param points 参数 `points`。
 * @param count 参数 `count`。
 * @param color 参数 `color`。
 * @param primitive 参数 `primitive`。
 * @param line_width 参数 `line_width`。
 * @return 函数返回值。
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
            write_screen_vertex(&cursor, canvas_view_world_to_screen(canvas, points[i]), color);
            write_screen_vertex(&cursor, canvas_view_world_to_screen(canvas, points[i + 1]), color);
        }
    } else {
        for (i = 0; i < count; ++i) {
            write_screen_vertex(&cursor, canvas_view_world_to_screen(canvas, points[i]), color);
        }
    }

    renderer->batched_vertex_count += append_vertices;
    return 1;
}

/**
 * @brief end_frame_batch 函数。
 *
 * @param renderer 参数 `renderer`。
 * @return 无。
 */
static void end_frame_batch(RenderSystem* renderer)
{
    flush_batch(renderer);
}

/**
 * @brief 绘制一条折线/线带路径。
 * @param renderer [in,out] 渲染器。
 * @param canvas [in] 画布视图。
 * @param points [in] 路径点数组。
 * @param count [in] 点数量。
 * @param color [in] 线颜色。
 * @param line_width [in] 线宽。
 * @return 无。
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
 * @brief count_grid_lines 函数。
 *
 * @param start 参数 `start`。
 * @param end 参数 `end`。
 * @param spacing 参数 `spacing`。
 * @return 函数返回值。
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
 * @brief 绘制当前可视区域的网格与坐标轴。
 * @param renderer [in,out] 渲染器。
 * @param canvas [in] 画布视图。
 * @return 无。
 *
 * 算法步骤：
 * 1. 计算可视世界矩形与网格起止坐标；
 * 2. 批量生成垂直/水平网格线段端点；
 * 3. 追加到 line batch；
 * 4. 额外绘制 X/Y 轴高亮线。
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
 * @brief draw_object 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param canvas 参数 `canvas`。
 * @param object 参数 `object`。
 * @param selected 参数 `selected`。
 * @return 无。
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
 * @brief render_canvas_scissor_box 函数。
 *
 * @param viewport 参数 `viewport`。
 * @param framebuffer_width 参数 `framebuffer_width`。
 * @param framebuffer_height 参数 `framebuffer_height`。
 * @param out_scissor 参数 `out_scissor`。
 * @return 函数返回值。
 */
static int render_canvas_scissor_box(const RectF* viewport, int framebuffer_width, int framebuffer_height, GLint out_scissor[4])
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

    x = (int)floorf(viewport->x);
    y_top = (int)floorf(viewport->y);
    w = (int)ceilf(viewport->w);
    h = (int)ceilf(viewport->h);
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

/**
 * @brief render_system_create 函数。
 *
 * @param window 参数 `window`。
 * @return 函数返回值。
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

    renderer->width = window->width;
    renderer->height = window->height;
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
        glUniform2f(renderer->screen_size_loc, (float)renderer->width, (float)renderer->height);
    }
    glUseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return renderer;
}

/**
 * @brief render_system_destroy 函数。
 *
 * @param renderer 参数 `renderer`。
 * @return 无。
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
 * @brief render_system_resize 函数。
 *
 * @param renderer 参数 `renderer`。
 * @param width 参数 `width`。
 * @param height 参数 `height`。
 * @return 无。
 */
void render_system_resize(RenderSystem* renderer, int width, int height)
{
    if (!renderer) {
        return;
    }

    renderer->width = width;
    renderer->height = height;
    glViewport(0, 0, width, height);
    glUseProgram(renderer->program);
    if (renderer->screen_size_loc >= 0) {
        glUniform2f(renderer->screen_size_loc, (float)width, (float)height);
    }
    glUseProgram(0);
}

/**
 * @brief Render full canvas frame.
 * @param renderer [in,out] 渲染器。
 * @param document [in] 文档对象集合。
 * @param canvas [in] 画布视图。
 * @param overlay_object [in] 工具叠加预览对象（可为 `NULL`）。
 * @return 无。
 *
 * Why preserve scissor state:
 * - UI and renderer may share context state; previous scissor state is restored
 *   to avoid leaking render-state changes into other draw passes.
 */
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object)
{
    int i = 0;
    GLboolean scissor_was_enabled = GL_FALSE;
    GLint previous_scissor_box[4] = {0, 0, 0, 0};
    GLint canvas_scissor_box[4] = {0, 0, 0, 0};
    RectF viewport;
    int has_canvas_area = 0;

    if (!renderer || !document || !canvas) {
        return;
    }

    viewport = canvas_view_viewport(canvas);
    has_canvas_area = render_canvas_scissor_box(&viewport,
                                                renderer->width,
                                                renderer->height,
                                                canvas_scissor_box);

    glViewport(0, 0, renderer->width, renderer->height);
    scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, previous_scissor_box);

    if (!has_canvas_area) {
        if (!scissor_was_enabled) {
            glDisable(GL_SCISSOR_TEST);
        }
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
        int selected = document_selection_contains(document, object->id);
        draw_object(renderer, canvas, object, selected);
    }

    if (overlay_object) {
        draw_object(renderer, canvas, overlay_object, 0);
    }

    end_frame_batch(renderer);
    log_gl_error_if_any("render_system_draw:after_batch_flush");
    glLineWidth(1.0f);
    glBindVertexArray(0);
    glUseProgram(0);
    log_gl_error_if_any("render_system_draw:before_finalize");

    renderer->debug_frame_counter++;
    render_log_frame_stats(renderer);

    if (scissor_was_enabled) {
        glScissor(previous_scissor_box[0],
                  previous_scissor_box[1],
                  previous_scissor_box[2],
                  previous_scissor_box[3]);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}
