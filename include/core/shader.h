#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

GLuint load_shader_program(const char* vertex_path, const char* fragment_path);
void cleanup_shaders(void);

/* Uniform setters - opaque handle, shader module manages locations internally */
void shader_set_color(float r, float g, float b);
void shader_set_offset(float x, float y);

#endif /* SHADER_H */
