set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(GLDRAW_FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "FetchContent download directory")
set(GLDRAW_LOCAL_GLFW_DIR "${GLDRAW_FETCHCONTENT_BASE_DIR}/glfw-src" CACHE PATH "Local GLFW source directory")

if(EXISTS "${GLDRAW_LOCAL_GLFW_DIR}/CMakeLists.txt")
    add_subdirectory("${GLDRAW_LOCAL_GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw-local")
    set(glfw_SOURCE_DIR "${GLDRAW_LOCAL_GLFW_DIR}")
else()
    include(FetchContent)
    set(FETCHCONTENT_BASE_DIR "${GLDRAW_FETCHCONTENT_BASE_DIR}")
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.3.9.tar.gz
        URL_HASH SHA256=a7e7faef424fcb5f83d8faecf9d697a338da7f7a906fc1afbc0e1879ef31bd53
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

if(GLDRAW_ENABLE_SCRIPTING)
    find_package(Lua REQUIRED)
endif()
