# =============================================================================
# GLDraw - CMake 构建配置
# =============================================================================
#
# 支持的平台和构建方式:
#   +----------+------------------+----------------------------------+
#   | 平台     | 编译器           | 构建命令                         |
#   +----------+------------------+----------------------------------+
#   | Windows  | MinGW / MSYS2    | cmake -G "MinGW Makefiles" ..   |
#   |          |                  | cmake --build .                  |
#   +----------+------------------+----------------------------------+
#   | Windows  | MSVC (VS 2022)   | cmake -G "Visual Studio 17 2022"|
#   |          |                  | cmake --build . --config Release |
#   +----------+------------------+----------------------------------+
#   | Linux    | GCC / Clang      | cmake ..                         |
#   |          |                  | cmake --build .                   |
#   +----------+------------------+----------------------------------+
#   | macOS    | Apple Clang      | cmake ..                         |
#   |          |                  | cmake --build .                   |
#   +----------+------------------+----------------------------------+
#
# 快速开始:
#   1. mkdir build && cd build
#   2. cmake -G "MinGW Makefiles" ..   # 根据平台选择生成器
#   3. cmake --build .
#   4. ./bin/GLDraw
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# 项目信息
# =============================================================================
project(GLDraw
    VERSION 1.0.0
    DESCRIPTION "A teaching-friendly OpenGL example with GLFW and Nuklear"
    LANGUAGES C
)

# =============================================================================
# 导出 compile_commands.json (IDE IntelliSense)
# =============================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# 构建类型配置
# =============================================================================
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# =============================================================================
# C 语言标准
# =============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# =============================================================================
# FetchContent - GLFW
# =============================================================================
include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)

FetchContent_MakeAvailable(glfw)

# =============================================================================
# GLAD - Pre-committed loader (glad.c in src/, glad.h in include/)
# glad.c and glad.h are committed to the repo.
# DO NOT regenerate during CMake configure.
# =============================================================================
set(GLAD_EXTRA_SOURCE "${CMAKE_SOURCE_DIR}/src/glad.c")

# =============================================================================
# 查找 OpenGL
# =============================================================================
find_package(OpenGL REQUIRED)

# =============================================================================
# 源文件
# =============================================================================
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nuklear_ui.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app_state.c
)

# =============================================================================
# 创建可执行文件
# =============================================================================
# glad.c uses ISO C-forbidden void*→function* casts (required for OpenGL loaders)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/glad.c
    PROPERTIES COMPILE_FLAGS "-Wno-pedantic"
)
add_executable(${PROJECT_NAME} ${SOURCES} ${GLAD_EXTRA_SOURCE})

# =============================================================================
# 头文件目录
# =============================================================================
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        "${glfw_SOURCE_DIR}/include"
)

# =============================================================================
# 着色器复制到输出目录
# =============================================================================
# 复制着色器到 bin/ 目录（VSCode 调试时 cwd=bin/）
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

# 复制着色器到 build/ 目录（命令行从 build/ 运行 ./bin/GLDraw）
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "${CMAKE_BINARY_DIR}/shaders"
)

# =============================================================================
# 链接库
# =============================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)

# =============================================================================
# 编译器特定选项
# =============================================================================
if(MSVC)
    message(STATUS "[INFO] Using MSVC compiler")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4             # 警告级别 4
        /permissive-    # 严格标准模式
        /utf-8          # UTF-8 源码
    )
    # Release 优化
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
    )
    # Debug 信息
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/Od /Zi>
    )

elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "[INFO] Using GCC/Clang compiler")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )
    # Release 优化
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-O2 -DNDEBUG>
    )
    # Debug 信息
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

# =============================================================================
# 平台特定链接
# =============================================================================
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()

# =============================================================================
# 输出目录配置
# =============================================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# =============================================================================
# 安装规则
# =============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# =============================================================================
# 打印构建信息
# =============================================================================
message(STATUS "")
message(STATUS "============================================================")
message(STATUS "  GLDraw - Build Configuration")
message(STATUS "============================================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Platform:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:       ${CMAKE_C_COMPILER_ID} (${CMAKE_C_COMPILER})")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard:     C${CMAKE_C_STANDARD}")
message(STATUS "  Source Dir:     ${CMAKE_SOURCE_DIR}")
message(STATUS "  Build Dir:      ${CMAKE_BINARY_DIR}")
message(STATUS "============================================================")
message(STATUS "")
