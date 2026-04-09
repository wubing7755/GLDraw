#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <core/shader.h>
#include <core/macros.h>

static GLuint s_shader_program = 0;

char* read_shader_file(const char* path)
{
    FILE* file = fopen(path, "r");
    if (UNLIKELY(!file)) {
        LOG_ERROR_F("Cannot open shader file: %s", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = (char*)malloc((size_t)size + 1);
    if (UNLIKELY(!source)) {
        LOG_ERROR_F("Memory allocation failed for shader: %s", path);
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(source, 1, (size_t)size, file);
    source[read_size] = '\0';

    fclose(file);
    return source;
}

static GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    if (UNLIKELY(shader == 0)) {
        LOG_ERROR_F("Failed to create shader object (type: %s)",
                  type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        return 0;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (UNLIKELY(!success)) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        LOG_ERROR_F("Shader compilation failed:\n%s", info_log);
        glDeleteShader(shader);
        return 0;
    }

    LOG_DEBUG_F("Shader compiled: %s",
              type == GL_VERTEX_SHADER ? "vertex" : "fragment");
    return shader;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    if (UNLIKELY(program == 0)) {
        LOG_ERROR("Failed to create program object");
        return 0;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (UNLIKELY(!success)) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        LOG_ERROR_F("Shader linking failed:\n%s", info_log);
        glDeleteProgram(program);
        return 0;
    }

    LOG_DEBUG_F("Shader linked (program ID: %u)", program);
    return program;
}

GLuint load_shader_program(const char* vertex_path, const char* fragment_path)
{
    char* vertex_source = NULL;
    char* fragment_source = NULL;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint program = 0;

    vertex_source = read_shader_file(vertex_path);
    ERR_PROPAGATE(!vertex_source);

    fragment_source = read_shader_file(fragment_path);
    ERR_PROPAGATE(!fragment_source);

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    ERR_PROPAGATE(!vertex_shader);

    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    ERR_PROPAGATE(!fragment_shader);

    program = link_program(vertex_shader, fragment_shader);
    ERR_PROPAGATE(!program);

    /* Cache program handle */
    s_shader_program = program;
    LOG_INFO_F("Shader program ready: %u", program);

cleanup:
    SAFE_FREE(vertex_source);
    SAFE_FREE(fragment_source);

    /* Delete shaders after linking (they're part of the program now) */
    if (vertex_shader != 0) glDeleteShader(vertex_shader);
    if (fragment_shader != 0) glDeleteShader(fragment_shader);

    return program;
}

void shader_use(void)
{
    if (UNLIKELY(s_shader_program == 0)) {
        LOG_ERROR("Shader program is 0!");
        return;
    }
    glUseProgram(s_shader_program);
}

void cleanup_shaders(void)
{
    if (s_shader_program != 0) {
        glDeleteProgram(s_shader_program);
        s_shader_program = 0;
        LOG_DEBUG("Shader program cleaned up");
    }
}

GLuint shader_get_program(void)
{
    return s_shader_program;
}
