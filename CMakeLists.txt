cmake_minimum_required(VERSION 3.15)

project(GLDraw
    VERSION 2.0.0
    DESCRIPTION "Canvas-oriented OpenGL drawing editor"
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
    )
    FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

set(GLDRAW_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/window.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/history.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas/canvas_view.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_def.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menubar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/src/glad.c
        PROPERTIES COMPILE_FLAGS "-Wno-pedantic"
    )
endif()

add_executable(${PROJECT_NAME} ${GLDRAW_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        "${glfw_SOURCE_DIR}/include"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /utf-8
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        $<$<CONFIG:Release>:-O2 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
