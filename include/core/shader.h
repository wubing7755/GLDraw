#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

GLuint load_shader_program(const char* vertex_path, const char* fragment_path);
void cleanup_shaders(void);
void shader_use(void);

#endif /* SHADER_H */
