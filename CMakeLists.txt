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

set(GLDRAW_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/file_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/path_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/object.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/document.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/history.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document/persistence.c
)

set(GLDRAW_IMAGE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image/png_writer.c
)

set(GLDRAW_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/command_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/workspace_dialogs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/file_dialog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/window.c
    ${GLDRAW_CORE_SOURCES}
    ${GLDRAW_IMAGE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas/canvas_view.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/key_chord.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/keymap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/input_router.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/tool_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/render_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_context_menu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_dialog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_def.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menu_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_menubar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/src/glad.c
        PROPERTIES COMPILE_FLAGS "-Wno-pedantic"
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/src/ui/ui_system.c
        PROPERTIES COMPILE_FLAGS "-Wno-c23-extensions"
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

if(EXISTS "${CMAKE_SOURCE_DIR}/themes")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/themes"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/themes"
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)

gldraw_apply_compile_options(${PROJECT_NAME})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32 comdlg32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(gldraw_document_core_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_document_core.c
    ${GLDRAW_CORE_SOURCES}
)

target_include_directories(gldraw_document_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

gldraw_apply_compile_options(gldraw_document_core_tests)

if(UNIX AND NOT APPLE)
    target_link_libraries(gldraw_document_core_tests PRIVATE m)
endif()

set_target_properties(gldraw_document_core_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_document_core_tests COMMAND $<TARGET_FILE:gldraw_document_core_tests>)

add_executable(gldraw_png_writer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_png_writer.c
    ${GLDRAW_IMAGE_SOURCES}
)

target_include_directories(gldraw_png_writer_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

gldraw_apply_compile_options(gldraw_png_writer_tests)

set_target_properties(gldraw_png_writer_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_png_writer_tests COMMAND $<TARGET_FILE:gldraw_png_writer_tests>)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
