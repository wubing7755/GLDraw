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

function(gldraw_apply_source_compile_options)
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
endfunction()

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
