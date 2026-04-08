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
#include <core/macros.h>

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

    LOG_INFO("Initializing window...");
    if (UNLIKELY(init_window() != 0)) {
        LOG_ERROR("Window initialization failed");
        goto cleanup;
    }
    window_initialized = 1;

    LOG_INFO("Loading OpenGL functions...");
    if (UNLIKELY(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))) {
        LOG_ERROR("Failed to load OpenGL functions");
        goto cleanup;
    }
    LOG_INFO_F("OpenGL loaded: %s", glGetString(GL_VERSION));

    LOG_INFO("Loading shaders...");
    if (UNLIKELY(load_shader_program("shaders/basic.vert",
                            "shaders/basic.frag") == 0)) {
        LOG_ERROR("Shader loading failed");
        goto cleanup;
    }
    shaders_loaded = 1;

    LOG_INFO("Initializing ShapeManager...");
    sm_init();
    shape_manager_initialized = 1;

    LOG_INFO("Initializing ShapeRegistry...");
    shape_registry_init();
    shape_register_all();
    shape_registry_initialized = 1;

    LOG_INFO("Initializing renderer...");
    if (UNLIKELY(init_renderer() != 0)) {
        LOG_ERROR("Renderer initialization failed");
        goto cleanup;
    }
    renderer_initialized = 1;

    LOG_INFO("Initializing input...");
    init_input(window_get_handle());

    LOG_INFO("Initializing ToolManager...");
    toolmanager_init();
    toolmanager_initialized = 1;

    /* Create tools */
    draw_tool = draw_tool_create("LINE");
    select_tool = select_tool_create();
    if (UNLIKELY(!draw_tool || !select_tool)) {
        LOG_ERROR("Failed to create tools");
        goto cleanup;
    }

    /* Initialize input with tools */
    input_init_tools(draw_tool, select_tool, draw_tool);

    LOG_INFO("Initializing Nuklear UI...");
    if (UNLIKELY(init_nuklear_ui(window_get_handle()) != 0)) {
        LOG_ERROR("Nuklear initialization failed");
        goto cleanup;
    }
    nuklear_initialized = 1;

    LOG_INFO("Initialization complete!");
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
    LOG_INFO("Cleaning up...");
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

    LOG_INFO("Done!");
    return exit_code;
}
