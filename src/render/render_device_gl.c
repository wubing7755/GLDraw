#include "render_device_gl.h"

#include <base/log.h>
#include <base/math2d.h>
#include <base/path_utils.h>
#include <base/resource_path.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <render/buffer_pool.h>
#include <render/shader_manager.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GL_RENDER_VERTEX_STRIDE_FLOATS 6u

typedef struct GLRenderDevice {
    RenderDevice base;
    ShaderManager shader_manager;
    BufferPool buffer_pool;
    RenderShaderProgram basic_program;
    RenderVertexStream stroke_stream;
    GLint screen_size_loc;
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    float scale_x;
    float scale_y;
    float* vertex_buffer;
    size_t vertex_capacity;
    Color clear_color;
    RenderPass active_pass;
    GLint clip_box[4];
} GLRenderDevice;

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
    return buffer_pool_reserve(&device->buffer_pool,
                               &device->stroke_stream,
                               required * sizeof(device->vertex_buffer[0]));
}

static Vec2 gl_render_transform_point(const GLRenderDevice* device, Vec2 point)
{
    Vec2 out = point;

    if (!device) {
        return out;
    }

    out.x *= device->active_pass.transform.scale_x;
    out.y *= device->active_pass.transform.scale_y;
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
                                     const RenderGeometry* geometry,
                                     const RenderMaterial* material)
{
    int vertex_count = 0;
    GLenum primitive = GL_LINES;
    float* cursor = NULL;
    int i = 0;

    if (!device || !geometry || !geometry->points || geometry->point_count <= 1 || !material) {
        return 0;
    }

    if (geometry->primitive == RENDER_PRIMITIVE_LINE_STRIP) {
        primitive = GL_LINES;
        vertex_count = (geometry->point_count - 1) * 2;
    } else {
        primitive = GL_LINES;
        vertex_count = geometry->point_count;
    }

    if (!gl_render_reserve_vertices(device, (size_t)vertex_count)) {
        return 0;
    }

    cursor = device->vertex_buffer;
    if (geometry->primitive == RENDER_PRIMITIVE_LINE_STRIP) {
        for (i = 0; i < geometry->point_count - 1; ++i) {
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, geometry->points[i]),
                                   material->color);
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, geometry->points[i + 1]),
                                   material->color);
        }
    } else {
        for (i = 0; i < geometry->point_count; ++i) {
            gl_render_write_vertex(&cursor,
                                   gl_render_transform_point(device, geometry->points[i]),
                                   material->color);
        }
    }

    glUseProgram(shader_program_handle(&device->basic_program));
    buffer_pool_bind_vertex_stream(&device->stroke_stream);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    (GLsizeiptr)(vertex_count * GL_RENDER_VERTEX_STRIDE_FLOATS *
                                 sizeof(device->vertex_buffer[0])),
                    device->vertex_buffer);
    glLineWidth((material->line_width > 0.5f) ? material->line_width : 1.0f);
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
    glUseProgram(shader_program_handle(&device->basic_program));
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
    device->active_pass.transform.scale_x = device->scale_x;
    device->active_pass.transform.scale_y = device->scale_y;
    device->active_pass.clip_rect = (RectF){0.0f,
                                            0.0f,
                                            (float)frame_desc->logical_width,
                                            (float)frame_desc->logical_height};

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, frame_desc->framebuffer_width, frame_desc->framebuffer_height);
    glClearColor(frame_desc->clear_color.r,
                 frame_desc->clear_color.g,
                 frame_desc->clear_color.b,
                 frame_desc->clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(shader_program_handle(&device->basic_program));
    buffer_pool_bind_vertex_stream(&device->stroke_stream);
    return 1;
}

static int gl_render_begin_pass(RenderDevice* render_device, const RenderPass* pass)
{
    GLRenderDevice* device = (GLRenderDevice*)render_device;
    int x = 0;
    int y_top = 0;
    int w = 0;
    int h = 0;
    int y = 0;

    if (!device || !pass) {
        return 0;
    }

    device->active_pass = *pass;
    x = (int)floorf(pass->clip_rect.x * device->scale_x);
    y_top = (int)floorf(pass->clip_rect.y * device->scale_y);
    w = (int)ceilf(pass->clip_rect.w * device->scale_x);
    h = (int)ceilf(pass->clip_rect.h * device->scale_y);
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
        return 0;
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
    return 1;
}

static int gl_render_draw_geometry(RenderDevice* render_device,
                                   const RenderGeometry* geometry,
                                   const RenderMaterial* material)
{
    return gl_render_upload_and_draw((GLRenderDevice*)render_device, geometry, material);
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

    free(device->vertex_buffer);
    buffer_pool_shutdown(&device->buffer_pool);
    shader_manager_shutdown(&device->shader_manager);
    free(device);
}

static const RenderDeviceVTable GL_RENDER_DEVICE_VTABLE = {
    gl_render_resize,
    gl_render_begin_frame,
    gl_render_begin_pass,
    gl_render_draw_geometry,
    gl_render_read_pixels,
    gl_render_end_frame,
    gl_render_destroy
};

RenderDevice* gl_render_device_create(PlatformWindow* window)
{
    GLRenderDevice* device = NULL;
    char vertex_shader_path[GLDRAW_PATH_MAX];
    char fragment_shader_path[GLDRAW_PATH_MAX];
    RenderShaderProgramDesc basic_program_desc;
    static const RenderVertexAttribute STROKE_ATTRIBUTES[] = {
        {0u, 2, 0u},
        {1u, 4, 2u * sizeof(float)}
    };
    static const RenderVertexStreamDesc STROKE_STREAM_DESC = {
        GL_RENDER_VERTEX_STRIDE_FLOATS * sizeof(float),
        STROKE_ATTRIBUTES,
        sizeof(STROKE_ATTRIBUTES) / sizeof(STROKE_ATTRIBUTES[0]),
        0u
    };

    if (!window) {
        return NULL;
    }
    if (!resource_path_resolve("shaders/basic.vert",
                               vertex_shader_path,
                               sizeof(vertex_shader_path)) ||
        !resource_path_resolve("shaders/basic.frag",
                               fragment_shader_path,
                               sizeof(fragment_shader_path))) {
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
    shader_manager_init(&device->shader_manager);
    buffer_pool_init(&device->buffer_pool);

    basic_program_desc.vertex_path = vertex_shader_path;
    basic_program_desc.fragment_path = fragment_shader_path;
    basic_program_desc.debug_label = "basic";

    if (!shader_manager_load_program(&device->shader_manager,
                                     &basic_program_desc,
                                     &device->basic_program)) {
        gl_render_destroy(&device->base);
        return NULL;
    }

    if (!buffer_pool_create_vertex_stream(&device->buffer_pool,
                                          &STROKE_STREAM_DESC,
                                          &device->stroke_stream)) {
        gl_render_destroy(&device->base);
        return NULL;
    }

    device->screen_size_loc = shader_program_uniform_location(&device->basic_program,
                                                              "uScreenSize");
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
