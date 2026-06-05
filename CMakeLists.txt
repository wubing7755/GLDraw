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

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" GLDRAW_RESOURCE_ROOT)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerOptions.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sources.cmake")

gldraw_apply_source_compile_options()

add_library(editor_model STATIC ${EDITOR_MODEL_SOURCES})
gldraw_target_defaults(editor_model)
target_compile_definitions(editor_model PUBLIC GLDRAW_RESOURCE_ROOT="${GLDRAW_RESOURCE_ROOT}")
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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Tests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake")
