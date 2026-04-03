#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <core/shader.h>

static GLuint s_shader_program = 0;

char* read_shader_file(const char* path)
{
    FILE* file = fopen(path, "r");
    if (!file)
    {
        printf("[Shader] Cannot open file: %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = (char*)malloc((size_t)size + 1);
    if (!source)
    {
        printf("[Shader] Memory allocation failed\n");
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
    if (shader == 0)
    {
        printf("[Shader] Failed to create shader object\n");
        return 0;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("[Shader] Compilation failed:\n%s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }

    printf("[Shader] Compiled successfully (type: %s)\n",
           type == GL_VERTEX_SHADER ? "vertex" : "fragment");
    return shader;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    if (program == 0)
    {
        printf("[Shader] Failed to create program object\n");
        return 0;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("[Shader] Linking failed:\n%s\n", info_log);
        glDeleteProgram(program);
        return 0;
    }

    printf("[Shader] Linked successfully (program ID: %u)\n", program);
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
    if (!vertex_source)
    {
        goto cleanup;
    }

    fragment_source = read_shader_file(fragment_path);
    if (!fragment_source)
    {
        goto cleanup;
    }

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0)
    {
        goto cleanup;
    }

    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0)
    {
        goto cleanup;
    }

    program = link_program(vertex_shader, fragment_shader);
    if (program == 0)
    {
        goto cleanup;
    }

    /* Cache program handle */
    s_shader_program = program;
    printf("[Shader] Program ready: %u\n", program);

cleanup:
    /* Always free sources regardless of success/failure */
    free(vertex_source);
    free(fragment_source);

    /* Delete shaders after linking (they're part of the program now) */
    if (vertex_shader != 0)
    {
        glDeleteShader(vertex_shader);
    }
    if (fragment_shader != 0)
    {
        glDeleteShader(fragment_shader);
    }

    return program;
}

void shader_use(void)
{
    if (s_shader_program != 0) {
        glUseProgram(s_shader_program);
    }
}

void cleanup_shaders(void)
{
    if (s_shader_program != 0) {
        glDeleteProgram(s_shader_program);
        s_shader_program = 0;
        printf("[Shader] Cleaned up\n");
    }
}
