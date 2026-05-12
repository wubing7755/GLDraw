#include <render/shader_manager.h>

#include <base/file_utils.h>
#include <base/log.h>

#include <glad/glad.h>

#include <stdlib.h>

static int shader_manager_track_program(ShaderManager* manager, unsigned int handle)
{
    unsigned int* handles = NULL;
    size_t capacity = 0u;

    if (!manager || !handle) {
        return 0;
    }
    if (manager->program_count >= manager->program_capacity) {
        capacity = manager->program_capacity > 0u ? manager->program_capacity * 2u : 4u;
        handles =
            (unsigned int*)realloc(manager->program_handles, capacity * sizeof(handles[0]));
        if (!handles) {
            return 0;
        }

        manager->program_handles = handles;
        manager->program_capacity = capacity;
    }

    manager->program_handles[manager->program_count++] = handle;
    return 1;
}

static GLuint shader_manager_compile_shader(GLenum type, const char* source, const char* label)
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

void shader_manager_init(ShaderManager* manager)
{
    if (!manager) {
        return;
    }

    manager->program_handles = NULL;
    manager->program_count = 0u;
    manager->program_capacity = 0u;
}

void shader_manager_shutdown(ShaderManager* manager)
{
    size_t i = 0u;

    if (!manager) {
        return;
    }

    for (i = 0u; i < manager->program_count; ++i) {
        if (manager->program_handles[i] != 0u) {
            glDeleteProgram(manager->program_handles[i]);
        }
    }

    free(manager->program_handles);
    shader_manager_init(manager);
}

int shader_manager_load_program(ShaderManager* manager,
                                const RenderShaderProgramDesc* desc,
                                RenderShaderProgram* out_program)
{
    char* vertex_source = NULL;
    char* fragment_source = NULL;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint program = 0;
    GLint success = 0;

    if (!manager || !desc || !desc->vertex_path || !desc->fragment_path || !out_program) {
        return 0;
    }

    vertex_source = file_utils_read_text_file(desc->vertex_path);
    fragment_source = file_utils_read_text_file(desc->fragment_path);
    if (!vertex_source || !fragment_source) {
        free(vertex_source);
        free(fragment_source);
        return 0;
    }

    vertex_shader =
        shader_manager_compile_shader(GL_VERTEX_SHADER, vertex_source, desc->vertex_path);
    fragment_shader =
        shader_manager_compile_shader(GL_FRAGMENT_SHADER, fragment_source, desc->fragment_path);
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
        LOG_ERROR("Program link failed for %s: %s",
                  desc->debug_label ? desc->debug_label : "unnamed program",
                  info);
        glDeleteProgram(program);
        return 0;
    }

    if (!shader_manager_track_program(manager, program)) {
        glDeleteProgram(program);
        return 0;
    }

    out_program->handle = program;
    return 1;
}

unsigned int shader_program_handle(const RenderShaderProgram* program)
{
    return program ? program->handle : 0u;
}

int shader_program_uniform_location(const RenderShaderProgram* program, const char* uniform_name)
{
    unsigned int handle = shader_program_handle(program);

    if (!handle || !uniform_name) {
        return -1;
    }

    return glGetUniformLocation(handle, uniform_name);
}
