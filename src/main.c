#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <core/window.h>
#include <core/shader.h>
#include <core/renderer.h>
#include <core/input.h>
#include <core/nuklear_ui.h>
#include <core/app_state.h>

static double get_time_seconds(void)
{
    return glfwGetTime();
}

int main(void)
{
    printf("===========================================\n");
    printf("  GLDraw - Modern OpenGL 3.3 Core Profile\n");
    printf("  Modular graphics implementation in C\n");
    printf("===========================================\n\n");

    /* Phase 1: Initialization */

    printf("[Main] Initializing window...\n");
    if (init_window() != 0)
    {
        printf("[Main] Window initialization failed\n");
        return -1;
    }

    printf("[Main] Loading OpenGL functions...\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("[Main] Failed to load OpenGL functions\n");
        shutdown_window();
        return -1;
    }
    printf("[Main] OpenGL loaded: %s\n", glGetString(GL_VERSION));

    printf("[Main] Loading shaders...\n");
    if (load_shader_program("shaders/basic.vert",
                            "shaders/basic.frag") == 0)
    {
        printf("[Main] Shader loading failed\n");
        shutdown_window();
        return -1;
    }

    printf("[Main] Initializing renderer...\n");
    if (init_renderer() != 0)
    {
        printf("[Main] Renderer initialization failed\n");
        cleanup_shaders();
        shutdown_window();
        return -1;
    }

    printf("[Main] Initializing input...\n");
    init_input(g_window);

    printf("[Main] Initializing Nuklear UI...\n");
    if (init_nuklear_ui(g_window) != 0)
    {
        printf("[Main] Nuklear initialization failed\n");
        cleanup_renderer();
        cleanup_shaders();
        shutdown_window();
        return -1;
    }

    printf("\n[Main] Initialization complete!\n");
    printf("===========================================\n");
    printf("Controls:\n");
    printf("  ESC - Exit\n");
    printf("  W/S - Move triangle up/down\n");
    printf("===========================================\n\n");

    /* Phase 2: Main Loop */
    double last_time = get_time_seconds();

    while (!window_should_close())
    {
        double current_time = get_time_seconds();
        float delta_time = (float)(current_time - last_time);
        (void)delta_time;
        last_time = current_time;

        poll_events();
        process_input();

        render_frame(&g_app_state);

        nuklear_new_frame();
        nuklear_build_ui();
        nuklear_render();

        swap_buffers();
    }

    /* Phase 3: Cleanup */
    printf("\n[Main] Cleaning up...\n");

    shutdown_nuklear_ui();
    cleanup_renderer();
    cleanup_shaders();
    shutdown_window();

    printf("[Main] Done!\n");
    return 0;
}
