cmake_minimum_required(VERSION 3.15)

project(GLDraw
    VERSION 2.0.0
    DESCRIPTION "Canvas-oriented OpenGL drawing editor"
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(GLDRAW_ENABLE_SCRIPTING "Enable Lua-based scripting support" OFF)
option(GLDRAW_ENABLE_ASAN "Enable AddressSanitizer for supported compilers" OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(GLDRAW_LOCAL_GLFW_DIR "${CMAKE_SOURCE_DIR}/build/_deps/glfw-src")

if(EXISTS "${GLDRAW_LOCAL_GLFW_DIR}/CMakeLists.txt")
    add_subdirectory("${GLDRAW_LOCAL_GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw-local")
    set(glfw_SOURCE_DIR "${GLDRAW_LOCAL_GLFW_DIR}")
else()
    include(FetchContent)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/_deps")
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.3.9.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

function(gldraw_apply_compile_options target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
            $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
            $<$<CONFIG:Debug>:/Od /Zi>
        )
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            $<$<CONFIG:Release>:-O2 -DNDEBUG>
            $<$<CONFIG:Debug>:-g -O0>
        )
    endif()
endfunction()

function(gldraw_apply_sanitizers target_name)
    if(GLDRAW_ENABLE_ASAN AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()
endfunction()

set(EDITOR_MODEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/file_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/path_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/selection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/extension_manifest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/manifest_runner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_properties.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_line.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_rect.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_ellipse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object_fake_star.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document_objects.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document_layers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document_spatial_index.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence_layers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence_objects.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence_write.c
)

set(RENDER_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas/canvas_view.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/canvas_drawlist.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/canvas_renderer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_arena.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image/png_writer.c
)

set(RENDER_GLFW_GL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/buffer_pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_device_factory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_device_gl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/shader_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

set(EDITOR_COMMANDS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_create_object.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_delete_selection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_executor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_layer_ops.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_move_objects.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_object_checks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_object_ops.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_paste_objects.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_set_property.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands/command_transaction.c
)

set(EDITOR_TOOLS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/tool_manifest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_input_dispatch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_select.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_pan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_shape.c
)

set(EDITOR_RUNTIME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/editor_action.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/command_availability.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/command_catalog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/command_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/command_dispatcher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/editor_action_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/editor_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/editor_viewmodel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/registration_manifest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_clipboard.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_dialog_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_dialogs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_edit_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_file_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_tool_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_view_commands.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/key_chord.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/keymap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/input_router.c
)

if(GLDRAW_ENABLE_SCRIPTING)
    find_package(Lua REQUIRED)
    list(APPEND EDITOR_TOOLS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/script/script_runtime_lua.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/script_tool_lua.c
    )
endif()

set(PLATFORM_GLFW_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/window.c
)

set(UI_NUKLEAR_GLFW_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_chrome.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_context_menu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_context_menu_render.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_frame.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_inspector_panel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_layer_panel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_layout.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_dialog_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_dialog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_def.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menubar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menubar_render.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_apply.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_builtin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_external.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_settings.c
)

set(EDITOR_APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application_dialog_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application_file_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/file_dialog.c
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/src/glad.c
        PROPERTIES COMPILE_FLAGS "-Wno-pedantic"
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/src/ui/ui_runtime.c
        PROPERTIES COMPILE_FLAGS "-Wno-c23-extensions"
    )
endif()

function(gldraw_target_defaults target_name)
    target_include_directories(${target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    gldraw_apply_compile_options(${target_name})
    gldraw_apply_sanitizers(${target_name})
endfunction()

function(gldraw_target_glfw_defaults target_name)
    gldraw_target_defaults(${target_name})
    target_include_directories(${target_name}
        PRIVATE
            "${glfw_SOURCE_DIR}/include"
    )
endfunction()

add_library(editor_model STATIC ${EDITOR_MODEL_SOURCES})
gldraw_target_defaults(editor_model)
if(UNIX AND NOT APPLE)
    target_link_libraries(editor_model PUBLIC m)
endif()

add_library(render_core STATIC ${RENDER_CORE_SOURCES})
gldraw_target_defaults(render_core)
target_link_libraries(render_core PUBLIC editor_model)

add_library(platform_glfw STATIC ${PLATFORM_GLFW_SOURCES})
gldraw_target_glfw_defaults(platform_glfw)
target_link_libraries(platform_glfw
    PUBLIC
        editor_model
    PRIVATE
        glfw
        OpenGL::GL
)

add_library(render_glfw_gl STATIC ${RENDER_GLFW_GL_SOURCES})
gldraw_target_glfw_defaults(render_glfw_gl)
target_link_libraries(render_glfw_gl
    PUBLIC
        render_core
    PRIVATE
        glfw
        OpenGL::GL
)

add_library(editor_commands STATIC ${EDITOR_COMMANDS_SOURCES})
gldraw_target_defaults(editor_commands)
target_link_libraries(editor_commands PUBLIC editor_model)

add_library(editor_tools STATIC ${EDITOR_TOOLS_SOURCES})
gldraw_target_glfw_defaults(editor_tools)
target_link_libraries(editor_tools
    PUBLIC
        editor_commands
        render_core
    PRIVATE
        glfw
)
if(GLDRAW_ENABLE_SCRIPTING)
    target_include_directories(editor_tools PRIVATE ${LUA_INCLUDE_DIR})
    target_link_libraries(editor_tools PRIVATE ${LUA_LIBRARIES})
    target_compile_definitions(editor_tools PRIVATE GLDRAW_ENABLE_SCRIPTING=1)
endif()

add_library(editor_runtime STATIC ${EDITOR_RUNTIME_SOURCES})
gldraw_target_glfw_defaults(editor_runtime)
target_link_libraries(editor_runtime
    PUBLIC
        editor_commands
        editor_tools
    PRIVATE
        render_core
        glfw
)

add_library(ui_nuklear_glfw STATIC ${UI_NUKLEAR_GLFW_SOURCES})
gldraw_target_glfw_defaults(ui_nuklear_glfw)
target_link_libraries(ui_nuklear_glfw
    PUBLIC
        editor_runtime
        platform_glfw
)

add_library(editor_app STATIC ${EDITOR_APP_SOURCES})
gldraw_target_glfw_defaults(editor_app)
target_link_libraries(editor_app
    PUBLIC
        editor_runtime
        ui_nuklear_glfw
        render_glfw_gl
        platform_glfw
)
if(WIN32)
    target_link_libraries(editor_app PUBLIC gdi32 comdlg32)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(editor_app PUBLIC pthread dl)
endif()

add_library(gldraw_model ALIAS editor_model)
add_library(gldraw_render_gl ALIAS render_glfw_gl)
add_library(gldraw_commands ALIAS editor_runtime)
add_library(gldraw_ui_nuklear ALIAS ui_nuklear_glfw)
add_library(gldraw_app ALIAS editor_app)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE editor_app)

gldraw_apply_compile_options(${PROJECT_NAME})
gldraw_apply_sanitizers(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/themes")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/themes"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/themes"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/scripts"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(gldraw_document_core_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_document_core.c
)

target_include_directories(gldraw_document_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_document_core_tests PRIVATE editor_runtime)

gldraw_apply_compile_options(gldraw_document_core_tests)
gldraw_apply_sanitizers(gldraw_document_core_tests)

set_target_properties(gldraw_document_core_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_document_core_tests COMMAND $<TARGET_FILE:gldraw_document_core_tests>)

add_executable(gldraw_png_writer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_png_writer.c
)

target_include_directories(gldraw_png_writer_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_png_writer_tests PRIVATE render_core)

gldraw_apply_compile_options(gldraw_png_writer_tests)
gldraw_apply_sanitizers(gldraw_png_writer_tests)

set_target_properties(gldraw_png_writer_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_png_writer_tests COMMAND $<TARGET_FILE:gldraw_png_writer_tests>)

add_executable(gldraw_selection_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_selection.c
)

target_include_directories(gldraw_selection_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_selection_tests PRIVATE editor_model)

gldraw_apply_compile_options(gldraw_selection_tests)
gldraw_apply_sanitizers(gldraw_selection_tests)

set_target_properties(gldraw_selection_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_selection_tests COMMAND $<TARGET_FILE:gldraw_selection_tests>)

add_executable(gldraw_document_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_document.c
)

target_include_directories(gldraw_document_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_document_tests PRIVATE editor_model)

gldraw_apply_compile_options(gldraw_document_tests)
gldraw_apply_sanitizers(gldraw_document_tests)

set_target_properties(gldraw_document_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_document_tests COMMAND $<TARGET_FILE:gldraw_document_tests>)

add_executable(gldraw_command_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_commands.c
)

target_include_directories(gldraw_command_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_command_tests PRIVATE editor_runtime)

gldraw_apply_compile_options(gldraw_command_tests)
gldraw_apply_sanitizers(gldraw_command_tests)

set_target_properties(gldraw_command_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_command_tests COMMAND $<TARGET_FILE:gldraw_command_tests>)

add_executable(gldraw_registry_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_registry.c
)

target_include_directories(gldraw_registry_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_registry_tests PRIVATE ui_nuklear_glfw)

gldraw_apply_compile_options(gldraw_registry_tests)
gldraw_apply_sanitizers(gldraw_registry_tests)

set_target_properties(gldraw_registry_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_registry_tests COMMAND $<TARGET_FILE:gldraw_registry_tests>)

if(GLDRAW_ENABLE_SCRIPTING)
    add_executable(gldraw_script_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_script_runtime.c
    )

    target_include_directories(gldraw_script_runtime_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(gldraw_script_runtime_tests PRIVATE editor_tools)

    gldraw_apply_compile_options(gldraw_script_runtime_tests)
    gldraw_apply_sanitizers(gldraw_script_runtime_tests)

    set_target_properties(gldraw_script_runtime_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    add_test(NAME gldraw_script_runtime_tests COMMAND $<TARGET_FILE:gldraw_script_runtime_tests>)
endif()

add_executable(gldraw_fake_star_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_fake_star.c
)

target_include_directories(gldraw_fake_star_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_fake_star_tests PRIVATE render_core)

gldraw_apply_compile_options(gldraw_fake_star_tests)
gldraw_apply_sanitizers(gldraw_fake_star_tests)

set_target_properties(gldraw_fake_star_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_fake_star_tests COMMAND $<TARGET_FILE:gldraw_fake_star_tests>)

add_executable(gldraw_no_builtin_knowledge_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_no_builtin_knowledge.c
)

target_include_directories(gldraw_no_builtin_knowledge_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_no_builtin_knowledge_tests PRIVATE editor_model)

gldraw_apply_compile_options(gldraw_no_builtin_knowledge_tests)
gldraw_apply_sanitizers(gldraw_no_builtin_knowledge_tests)

set_target_properties(gldraw_no_builtin_knowledge_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_no_builtin_knowledge_tests COMMAND $<TARGET_FILE:gldraw_no_builtin_knowledge_tests>)

add_executable(gldraw_ui_logic_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ui_logic.c
)

target_include_directories(gldraw_ui_logic_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_ui_logic_tests PRIVATE editor_runtime)

gldraw_apply_compile_options(gldraw_ui_logic_tests)
gldraw_apply_sanitizers(gldraw_ui_logic_tests)

set_target_properties(gldraw_ui_logic_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_ui_logic_tests COMMAND $<TARGET_FILE:gldraw_ui_logic_tests>)

add_executable(gldraw_ui_theme_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_nuklear_impl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_apply.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_builtin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_external.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme_settings.c
)

target_include_directories(gldraw_ui_theme_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_ui_theme_tests PRIVATE editor_model)

gldraw_apply_compile_options(gldraw_ui_theme_tests)
gldraw_apply_sanitizers(gldraw_ui_theme_tests)

set_target_properties(gldraw_ui_theme_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_ui_theme_tests COMMAND $<TARGET_FILE:gldraw_ui_theme_tests>)

add_executable(gldraw_renderer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_renderer.c
)

target_include_directories(gldraw_renderer_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_renderer_tests PRIVATE render_core)

gldraw_apply_compile_options(gldraw_renderer_tests)
gldraw_apply_sanitizers(gldraw_renderer_tests)

set_target_properties(gldraw_renderer_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_renderer_tests COMMAND $<TARGET_FILE:gldraw_renderer_tests>)

add_executable(gldraw_workspace_service_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_workspace_service.c
)

target_include_directories(gldraw_workspace_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_workspace_service_tests PRIVATE editor_runtime)

gldraw_apply_compile_options(gldraw_workspace_service_tests)
gldraw_apply_sanitizers(gldraw_workspace_service_tests)

set_target_properties(gldraw_workspace_service_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_workspace_service_tests COMMAND $<TARGET_FILE:gldraw_workspace_service_tests>)

add_custom_target(gldraw_tests
    DEPENDS
        gldraw_document_core_tests
        gldraw_png_writer_tests
        gldraw_selection_tests
        gldraw_document_tests
        gldraw_command_tests
        gldraw_registry_tests
        gldraw_fake_star_tests
        gldraw_no_builtin_knowledge_tests
        gldraw_ui_logic_tests
        gldraw_ui_theme_tests
        gldraw_renderer_tests
        gldraw_workspace_service_tests
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
