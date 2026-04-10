#include <render/render_system.h>

#include <base/math2d.h>

#include <glad/glad.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct RenderSystem {
    GLuint program;
    GLuint vao;
    GLuint vbo;
    int width;
    int height;
    float* vertex_buffer;
    size_t vertex_buffer_capacity;
    Vec2* path_buffer;
    size_t path_buffer_capacity;
};

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

static GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    GLint success = 0;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(shader, (GLsizei)sizeof(info), NULL, info);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint load_program(const char* vertex_path, const char* fragment_path)
{
    char* vertex_source = read_text_file(vertex_path);
    char* fragment_source = read_text_file(fragment_path);
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint program = 0;
    GLint success = 0;

    if (!vertex_source || !fragment_source) {
        free(vertex_source);
        free(fragment_source);
        return 0;
    }

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
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
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static int ensure_vertex_capacity(RenderSystem* renderer, size_t point_count)
{
    size_t required_vertex_capacity = point_count * 6u;

    if (required_vertex_capacity > renderer->vertex_buffer_capacity) {
        float* new_buffer = (float*)realloc(renderer->vertex_buffer, required_vertex_capacity * sizeof(float));
        if (!new_buffer) {
            return 0;
        }
        renderer->vertex_buffer = new_buffer;
        renderer->vertex_buffer_capacity = required_vertex_capacity;
    }

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

static void upload_points(RenderSystem* renderer, const CanvasView* canvas, const Vec2* points, int count, Color color)
{
    int i = 0;
    float* cursor = renderer->vertex_buffer;

    for (i = 0; i < count; ++i) {
        Vec2 screen = canvas_view_world_to_screen(canvas, points[i]);
        *cursor++ = screen.x;
        *cursor++ = screen.y;
        *cursor++ = color.r;
        *cursor++ = color.g;
        *cursor++ = color.b;
        *cursor++ = color.a;
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * 6 * sizeof(float)), renderer->vertex_buffer, GL_DYNAMIC_DRAW);
}

static void draw_polyline(RenderSystem* renderer,
                          const CanvasView* canvas,
                          const Vec2* points,
                          int count,
                          Color color,
                          float line_width)
{
    if (count <= 1 || !ensure_vertex_capacity(renderer, (size_t)count)) {
        return;
    }

    upload_points(renderer, canvas, points, count, color);
    glLineWidth((line_width > 0.5f) ? line_width : 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, count);
}

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
    Vec2 line[2];
    float x = 0.0f;
    float y = 0.0f;

    for (x = start_x; x <= end_x; x += spacing) {
        line[0] = vec2_make(x, visible.y);
        line[1] = vec2_make(x, visible.y + visible.h);
        draw_polyline(renderer, canvas, line, 2, grid_color, 1.0f);
    }

    for (y = start_y; y <= end_y; y += spacing) {
        line[0] = vec2_make(visible.x, y);
        line[1] = vec2_make(visible.x + visible.w, y);
        draw_polyline(renderer, canvas, line, 2, grid_color, 1.0f);
    }

    line[0] = vec2_make(visible.x, 0.0f);
    line[1] = vec2_make(visible.x + visible.w, 0.0f);
    draw_polyline(renderer, canvas, line, 2, axis_color, 1.5f);

    line[0] = vec2_make(0.0f, visible.y);
    line[1] = vec2_make(0.0f, visible.y + visible.h);
    draw_polyline(renderer, canvas, line, 2, axis_color, 1.5f);
}

static void draw_object(RenderSystem* renderer,
                        const CanvasView* canvas,
                        const GraphicObject* object,
                        int selected)
{
    int point_count = object_get_path_point_count(object);
    Color highlight = {0.98f, 0.86f, 0.24f, 1.0f};

    if (point_count <= 1 || !ensure_vertex_capacity(renderer, (size_t)point_count)) {
        return;
    }

    object_write_path_points(object, renderer->path_buffer);
    if (selected) {
        draw_polyline(renderer, canvas, renderer->path_buffer, point_count, highlight, object->style.stroke_width + 3.0f);
    }
    draw_polyline(renderer, canvas, renderer->path_buffer, point_count, object->style.stroke_color, object->style.stroke_width);
}

RenderSystem* render_system_create(PlatformWindow* window)
{
    RenderSystem* renderer = (RenderSystem*)calloc(1, sizeof(*renderer));
    GLint screen_size_loc = -1;

    if (!renderer) {
        return NULL;
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        free(renderer);
        return NULL;
    }

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
    screen_size_loc = glGetUniformLocation(renderer->program, "uScreenSize");
    glUniform2f(screen_size_loc, (float)renderer->width, (float)renderer->height);
    glUseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return renderer;
}

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

void render_system_resize(RenderSystem* renderer, int width, int height)
{
    GLint screen_size_loc = -1;

    if (!renderer) {
        return;
    }

    renderer->width = width;
    renderer->height = height;
    glViewport(0, 0, width, height);
    glUseProgram(renderer->program);
    screen_size_loc = glGetUniformLocation(renderer->program, "uScreenSize");
    glUniform2f(screen_size_loc, (float)width, (float)height);
    glUseProgram(0);
}

void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object)
{
    int i = 0;

    if (!renderer || !document || !canvas) {
        return;
    }

    glViewport(0, 0, renderer->width, renderer->height);
    glClearColor(canvas->background.r, canvas->background.g, canvas->background.b, canvas->background.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer->program);
    glBindVertexArray(renderer->vao);

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

    glLineWidth(1.0f);
    glBindVertexArray(0);
    glUseProgram(0);
}
