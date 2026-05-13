#ifndef GLDRAW_RENDER_SHADER_MANAGER_H
#define GLDRAW_RENDER_SHADER_MANAGER_H

#include <stddef.h>

typedef struct RenderShaderProgram {
    unsigned int handle;
} RenderShaderProgram;

typedef struct RenderShaderProgramDesc {
    const char* vertex_path;
    const char* fragment_path;
    const char* debug_label;
} RenderShaderProgramDesc;

typedef struct ShaderManager {
    unsigned int* program_handles;
    size_t program_count;
    size_t program_capacity;
} ShaderManager;

void shader_manager_init(ShaderManager* manager);
void shader_manager_shutdown(ShaderManager* manager);
int shader_manager_load_program(ShaderManager* manager,
                                const RenderShaderProgramDesc* desc,
                                RenderShaderProgram* out_program);
unsigned int shader_program_handle(const RenderShaderProgram* program);
int shader_program_uniform_location(const RenderShaderProgram* program, const char* uniform_name);

#endif /* GLDRAW_RENDER_SHADER_MANAGER_H */
