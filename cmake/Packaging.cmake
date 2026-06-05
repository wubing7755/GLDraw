install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
    DESTINATION share/gldraw/shaders
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/themes")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/themes/"
        DESTINATION share/gldraw/themes
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/"
        DESTINATION share/gldraw/scripts
    )
endif()
