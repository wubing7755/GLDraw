add_executable(gldraw_document_core_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/document/test_document_core.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/image/test_png_writer.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/document/test_selection.c
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

add_executable(gldraw_resource_path_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/base/test_resource_path.c
)

target_include_directories(gldraw_resource_path_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gldraw_resource_path_tests PRIVATE editor_model)

gldraw_apply_compile_options(gldraw_resource_path_tests)
gldraw_apply_sanitizers(gldraw_resource_path_tests)

set_target_properties(gldraw_resource_path_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME gldraw_resource_path_tests COMMAND $<TARGET_FILE:gldraw_resource_path_tests>)

add_executable(gldraw_document_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/document/test_document.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/commands/test_commands.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_registry.c
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/script/test_script_runtime.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/document/test_fake_star.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_no_builtin_knowledge.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui/test_ui_logic.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui/test_ui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/support/test_nuklear_impl.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/render/test_renderer.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_workspace_service.c
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
        gldraw_resource_path_tests
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
