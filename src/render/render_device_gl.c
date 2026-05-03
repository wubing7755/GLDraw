#include "render_device_gl.h"

#include <base/file_utils.h>
#include <base/log.h>
#include <base/math2d.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GL_RENDER_VERTEX_STRIDE_FLOATS 6u

typedef struct GLRenderDevice {
    RenderDevice base;
    GLuint program;
    GLuint vao;
    GLuint vbo;
    GLint screen_size_loc;
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    float scale_x;
    float scale_y;
    float* vertex_buffer;
    size_t vertex_capacity;
    Color current_color;
    RenderTransform transform;
    Color clear_color;
    GLint clip_box[4];
} GLRenderDevice;

static GLuint gl_render_compile_shader(GLenum type, const char* source, const char* label)
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

static GLuint gl_render_load_program(const char* vertex_path, const char* fragment_path)
{
    char* vertex_source = file_utils_read_text_file(vertex_path);
    char* fragment_source = file_utils_read_text_file(fragment_path);
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint program = 0;
    GLint success = 0;

    if (!vertex_source || !fragment_source) {
        free(vertex_source);
        free(fragment_source);
        return 0;
    }

    vertex_shader = gl_render_compile_shader(GL_VERTEX_SHADER, vertex_source, vertex_path);
    fragment_shader = gl_render_compile_shader(GL_FRAGMENT_SHADER, fragment_source, fragment_path);
    free(vertex_source);
    free(fragment_source);

    if (!vertex_shader || !fragment_shader) {
        if (vertex_shader) {
            glDeleteShader(vertex_shader);
        }
        if (fragment_shader) {
            glDeleteShader(fragment_shader);
        }
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
        LOG_ERROR("Program link failed: %s", info);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static int gl_render_reserve_vertices(GLRenderDevice* device, size_t vertex_count)
{
    float* buffer = NULL;
    size_t required = vertex_count * GL_RENDER_VERTEX_STRIDE_FLOATS;

    if (!device) {
        return 0;
    }
    if (required <= device->vertex_capacity) {
        return 1;
    }

    buffer = (float*)realloc(device->vertex_buffer, required * sizeof(device->vertex_buffer[0]));
    if (!buffer) {
        return 0;
    }

    device->vertex_buffer = buffer;
    device->vertex_capacity = required;
    glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(required * sizeof(device->vertex_buffer[0])),
                 NULL,
                 GL_DYNAMIC_DRAW);
    return 1;
}

static Vec2 gl_render_transform_point(const GLRenderDevice* device, Vec2 point)
{
    Vec2 out = point;

    if (!device) {
        return out;
    }

    out.x *= device->transform.scale_x;
    out.y *= device->transform.scale_y;
    return out;
}

static void gl_render_write_vertex(float** cursor, Vec2 point, Color color)
{
    *(*cursor)++ = point.x;
    *(*cursor)++ = point.y;
    *(*cursor)++ = color.r;
    *(*cursor)++ = color.g;
    *(*cursor)++ = color.b;
    *(*cursor)++ = color.a;
}

static int gl_render_upload_and_draw(GLRenderDevice* device,
                                     const Vec2* points,
                                     int point_count,
                                     RenderPathMode mode,
                                     float line_width)
{
    int vertex_count = 0;
    GLenum primitive = GL_LINES;
    float* cursor = NULL;
    int i = 0;

    if (!device || !points || point_count <= 1) {
        return 0;
    }

    if (mode == RENDER_PATH_LINE_STRIP) {
        primitive = GL_LINES;
        vertex_count = (point_count - 1) * 2;
    } else {
        primitive = GL_LINES;
        vertex_count = point_count;
    }

    if (!gl_render_reserve_vertices(device, (size_t)vertex_count)) {
        return 0;
    }

    cursor = device->vertex_buffer;
    if (mode == RENDER_PATH_LINE_STRIP) {
        for (i = 0; i < point_count - 1; ++i) {
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, points[i]),
                                   device->current_color);
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, points[i + 1]),
                                   device->current_color);
        }
    } else {
        for (i = 0; i < point_count; ++i) {
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, points[i]),
                                   device->current_color);
        }
    }

    glUseProgram(device->program);
    glBindVertexArray(device->vao);
    glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    (GLsizeiptr)(vertex_count * GL_RENDER_VERTEX_STRIDE_FLOATS *
                                 sizeof(device->vertex_buffer[0])),
                    device->vertex_buffer);
    glLineWidth((line_width > 0.5f) ? line_width : 1.0f);
    glDrawArrays(primitive, 0, vertex_count);
    return 1;
}

static int gl_render_resize(RenderDevice* render_device,
                            int logical_width,
                            int logical_height,
                            int framebuffer_width,
                            int framebuffer_height)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device) {
        return 0;
    }

    device->logical_width = logical_width;
    device->logical_height = logical_height;
    device->framebuffer_width = framebuffer_width;
    device->framebuffer_height = framebuffer_height;
    device->scale_x = (logical_width > 0) ? ((float)framebuffer_width / (float)logical_width)
                                          : 1.0f;
    device->scale_y = (logical_height > 0) ? ((float)framebuffer_height / (float)logical_height)
                                           : 1.0f;

    glViewport(0, 0, framebuffer_width, framebuffer_height);
    glUseProgram(device->program);
    if (device->screen_size_loc >= 0) {
        glUniform2f(device->screen_size_loc,
                    (float)framebuffer_width,
                    (float)framebuffer_height);
    }
    glUseProgram(0);
    return 1;
}

static int gl_render_begin_frame(RenderDevice* render_device, const RenderFrameDesc* frame_desc)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device || !frame_desc) {
        return 0;
    }

    gl_render_resize(render_device,
                     frame_desc->logical_width,
                     frame_desc->logical_height,
                     frame_desc->framebuffer_width,
                     frame_desc->framebuffer_height);
    device->clear_color = frame_desc->clear_color;
    device->current_color = (Color){1.0f, 1.0f, 1.0f, 1.0f};
    device->transform.scale_x = device->scale_x;
    device->transform.scale_y = device->scale_y;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(device->program);
    glBindVertexArray(device->vao);
    glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
    return 1;
}

static void gl_render_set_transform(RenderDevice* render_device, const RenderTransform* transform)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device || !transform) {
        return;
    }

    device->transform = *transform;
}

static void gl_render_set_clip_rect(RenderDevice* render_device, RectF clip_rect)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;
    int x = 0;
    int y_top = 0;
    int w = 0;
    int h = 0;
    int y = 0;

    if (!device) {
        return;
    }

    x = (int)floorf(clip_rect.x * device->scale_x);
    y_top = (int)floorf(clip_rect.y * device->scale_y);
    w = (int)ceilf(clip_rect.w * device->scale_x);
    h = (int)ceilf(clip_rect.h * device->scale_y);
    y = device->framebuffer_height - y_top - h;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > device->framebuffer_width) {
        w = device->framebuffer_width - x;
    }
    if (y + h > device->framebuffer_height) {
        h = device->framebuffer_height - y;
    }
    if (w <= 0 || h <= 0) {
        return;
    }

    device->clip_box[0] = x;
    device->clip_box[1] = y;
    device->clip_box[2] = w;
    device->clip_box[3] = h;

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
    glClearColor(device->clear_color.r,
                 device->clear_color.g,
                 device->clear_color.b,
                 device->clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void gl_render_set_color(RenderDevice* render_device, Color color)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device) {
        return;
    }

    device->current_color = color;
}

static void gl_render_draw_line(RenderDevice* render_device,
                                Vec2 from,
                                Vec2 to,
                                float line_width)
{
    Vec2 points[2];

    points[0] = from;
    points[1] = to;
    gl_render_upload_and_draw((GLRenderDevice*)render_device,
                              points,
                              2,
                              RENDER_PATH_LINES,
                              line_width);
}

static void gl_render_draw_rect(RenderDevice* render_device,
                                RectF rect,
                                float line_width)
{
    Vec2 points[5];

    points[0] = vec2_make(rect.x, rect.y);
    points[1] = vec2_make(rect.x + rect.w, rect.y);
    points[2] = vec2_make(rect.x + rect.w, rect.y + rect.h);
    points[3] = vec2_make(rect.x, rect.y + rect.h);
    points[4] = points[0];
    gl_render_upload_and_draw((GLRenderDevice*)render_device,
                              points,
                              5,
                              RENDER_PATH_LINE_STRIP,
                              line_width);
}

static void gl_render_draw_path(RenderDevice* render_device,
                                const Vec2* points,
                                int point_count,
                                RenderPathMode mode,
                                float line_width)
{
    gl_render_upload_and_draw((GLRenderDevice*)render_device,
                              points,
                              point_count,
                              mode,
                              line_width);
}

static int gl_render_read_pixels(RenderDevice* render_device,
                                 RectF rect,
                                 unsigned char* out_rgba,
                                 int stride_bytes)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;
    GLint previous_pack_alignment = 4;
    int x = 0;
    int y_top = 0;
    int w = 0;
    int h = 0;
    int y = 0;

    if (!device || !out_rgba || stride_bytes <= 0) {
        return 0;
    }
    (void)stride_bytes;

    x = (int)floorf(rect.x * device->scale_x);
    y_top = (int)floorf(rect.y * device->scale_y);
    w = (int)ceilf(rect.w * device->scale_x);
    h = (int)ceilf(rect.h * device->scale_y);
    y = device->framebuffer_height - y_top - h;
    if (w <= 0 || h <= 0) {
        return 0;
    }

    glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba);
    glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);
    return glGetError() == GL_NO_ERROR;
}

static void gl_render_end_frame(RenderDevice* render_device)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device) {
        return;
    }
    glDisable(GL_SCISSOR_TEST);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void gl_render_destroy(RenderDevice* render_device)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;

    if (!device) {
        return;
    }

    glDeleteBuffers(1, &device->vbo);
    glDeleteVertexArrays(1, &device->vao);
    glDeleteProgram(device->program);
    free(device->vertex_buffer);
    free(device);
}

static const RenderDeviceVTable GL_RENDER_DEVICE_VTABLE = {
    gl_render_resize,
    gl_render_begin_frame,
    gl_render_set_transform,
    gl_render_set_clip_rect,
    gl_render_set_color,
    gl_render_draw_line,
    gl_render_draw_rect,
    gl_render_draw_path,
    gl_render_read_pixels,
    gl_render_end_frame,
    gl_render_destroy
};

RenderDevice* gl_render_device_create(PlatformWindow* window)
{
    GLRenderDevice* device = NULL;

    if (!window) {
        return NULL;
    }
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return NULL;
    }

    device = (GLRenderDevice*)calloc(1u, sizeof(*device));
    if (!device) {
        return NULL;
    }

    device->base.vtable = &GL_RENDER_DEVICE_VTABLE;
    device->program = gl_render_load_program("shaders/basic.vert", "shaders/basic.frag");
    if (!device->program) {
        gl_render_destroy(&device->base);
        return NULL;
    }

    glGenVertexArrays(1, &device->vao);
    glGenBuffers(1, &device->vbo);
    glBindVertexArray(device->vao);
    glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    device->screen_size_loc = glGetUniformLocation(device->program, "uScreenSize");
    if (!gl_render_resize(&device->base,
                          window->width,
                          window->height,
                          window->framebuffer_width,
                          window->framebuffer_height)) {
        gl_render_destroy(&device->base);
        return NULL;
    }

    return &device->base;
}
