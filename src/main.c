#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <core/window.h>
#include <core/shader.h>
#include <core/renderer.h>
#include <core/input.h>
#include <core/nuklear_ui.h>
#include <core/shape_manager.h>
#include <core/shape_registry.h>
#include <core/tool_manager.h>
#include <core/draw_tool.h>
#include <core/select_tool.h>

static double get_time_seconds(void)
{
    return glfwGetTime();
}

int main(void)
{
    int exit_code = -1;
    int window_initialized = 0;
    int shaders_loaded = 0;
    int shape_manager_initialized = 0;
    int shape_registry_initialized = 0;
    int renderer_initialized = 0;
    int toolmanager_initialized = 0;
    int nuklear_initialized = 0;
    Tool* draw_tool = NULL;
    Tool* select_tool = NULL;

    printf("===========================================\n");
    printf("  GLDraw - Phase 2: Multi-Shape Support\n");
    printf("  OpenGL 3.3 Core Profile\n");
    printf("===========================================\n\n");

    /* Phase 1: Initialization */

    printf("[Main] Initializing window...\n");
    if (init_window() != 0) {
        printf("[Main] Window initialization failed\n");
        goto cleanup;
    }
    window_initialized = 1;

    printf("[Main] Loading OpenGL functions...\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("[Main] Failed to load OpenGL functions\n");
        goto cleanup;
    }
    printf("[Main] OpenGL loaded: %s\n", glGetString(GL_VERSION));

    printf("[Main] Loading shaders...\n");
    if (load_shader_program("shaders/basic.vert",
                            "shaders/basic.frag") == 0) {
        printf("[Main] Shader loading failed\n");
        goto cleanup;
    }
    shaders_loaded = 1;

    printf("[Main] Initializing ShapeManager...\n");
    sm_init();
    shape_manager_initialized = 1;

    printf("[Main] Initializing ShapeRegistry...\n");
    shape_registry_init();
    shape_register_all();
    shape_registry_initialized = 1;

    printf("[Main] Initializing renderer...\n");
    if (init_renderer() != 0) {
        printf("[Main] Renderer initialization failed\n");
        goto cleanup;
    }
    renderer_initialized = 1;

    printf("[Main] Initializing input...\n");
    init_input(window_get_handle());

    printf("[Main] Initializing ToolManager...\n");
    toolmanager_init();
    toolmanager_initialized = 1;

    /* Create tools */
    draw_tool = draw_tool_create("LINE");
    select_tool = select_tool_create();
    if (!draw_tool || !select_tool) {
        printf("[Main] Failed to create tools\n");
        goto cleanup;
    }

    /* Initialize input with tools */
    input_init_tools(draw_tool, select_tool, draw_tool);

    printf("[Main] Initializing Nuklear UI...\n");
    if (init_nuklear_ui(window_get_handle()) != 0) {
        printf("[Main] Nuklear initialization failed\n");
        goto cleanup;
    }
    nuklear_initialized = 1;

    printf("\n[Main] Initialization complete!\n");
    printf("===========================================\n");
    printf("Controls:\n");
    printf("  Left mouse drag — draw line\n");
    printf("  Ctrl+Z — delete last line\n");
    printf("  ESC — exit\n");
    printf("===========================================\n\n");

    /* Phase 2: Main Loop */
    double last_time = get_time_seconds();

    while (!window_should_close()) {
        double current_time = get_time_seconds();
        float delta_time = (float)(current_time - last_time);
        (void)delta_time;
        last_time = current_time;

        poll_events();
        process_input();

        /* Render all shapes from ShapeManager */
        render_frame();

        /* Nuklear UI (for future toolbar/property panel) */
        nuklear_new_frame();
        nuklear_build_ui();
        nuklear_render();

        swap_buffers();
    }

    /* Cleanup */
    printf("\n[Main] Cleaning up...\n");
    exit_code = 0;

cleanup:
    if (nuklear_initialized) {
        shutdown_nuklear_ui();
    }
    if (toolmanager_initialized) {
        toolmanager_shutdown();
    }
    if (draw_tool) {
        draw_tool_destroy(draw_tool);
    }
    if (select_tool) {
        select_tool_destroy(select_tool);
    }
    if (shape_registry_initialized) {
        shape_registry_shutdown();
    }
    if (shape_manager_initialized) {
        sm_shutdown();
    }
    if (renderer_initialized) {
        cleanup_renderer();
    }
    if (shaders_loaded) {
        cleanup_shaders();
    }
    if (window_initialized) {
        shutdown_window();
    }

    printf("[Main] Done!\n");
    return exit_code;
}
