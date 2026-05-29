/**
 * @file resource_path.c
 * @brief Runtime lookup helpers for bundled application resources.
 */
#include <base/resource_path.h>

#include <base/path_utils.h>

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

#include <sys/stat.h>

#ifndef GLDRAW_RESOURCE_ROOT
#define GLDRAW_RESOURCE_ROOT "."
#endif

static int resource_path_exists(const char* path)
{
    struct stat info;

    if (!path || path[0] == '\0') {
        return 0;
    }

    return stat(path, &info) == 0;
}

static int resource_path_copy(const char* path, char* output, size_t output_size)
{
    int written = 0;

    if (!path || !output || output_size == 0u) {
        return 0;
    }

    written = snprintf(output, output_size, "%s", path);
    return written >= 0 && (size_t)written < output_size;
}

static int resource_path_join(const char* base,
                              const char* relative_path,
                              char* output,
                              size_t output_size)
{
    size_t base_length = 0u;
    const char* separator = "/";
    int written = 0;

    if (!base || base[0] == '\0' || !relative_path || !output || output_size == 0u) {
        return 0;
    }

    base_length = strlen(base);
    if (base_length > 0u && (base[base_length - 1u] == '/' || base[base_length - 1u] == '\\')) {
        separator = "";
    }

    written = snprintf(output, output_size, "%s%s%s", base, separator, relative_path);
    return written >= 0 && (size_t)written < output_size;
}

static int resource_path_executable_dir(char* output, size_t output_size)
{
    char executable_path[GLDRAW_PATH_MAX];

    if (!output || output_size == 0u) {
        return 0;
    }

#if defined(_WIN32)
    {
        DWORD length = GetModuleFileNameA(NULL, executable_path, (DWORD)sizeof(executable_path));
        if (length == 0u || length >= sizeof(executable_path)) {
            return 0;
        }
        executable_path[length] = '\0';
    }
#elif defined(__APPLE__)
    {
        uint32_t length = (uint32_t)sizeof(executable_path);
        if (_NSGetExecutablePath(executable_path, &length) != 0) {
            return 0;
        }
    }
#else
    {
        ssize_t length = readlink("/proc/self/exe", executable_path, sizeof(executable_path) - 1u);
        if (length <= 0 || (size_t)length >= sizeof(executable_path)) {
            return 0;
        }
        executable_path[length] = '\0';
    }
#endif

    return path_utils_dirname(executable_path, output, output_size);
}

static int resource_path_try_candidate(const char* path, char* output, size_t output_size)
{
    if (!path || !resource_path_exists(path)) {
        return 0;
    }

    return resource_path_copy(path, output, output_size);
}

int resource_path_resolve(const char* relative_path, char* output, size_t output_size)
{
    char executable_dir[GLDRAW_PATH_MAX];
    char candidate[GLDRAW_PATH_MAX];
    char install_root[GLDRAW_PATH_MAX];

    if (!relative_path || relative_path[0] == '\0' || !output || output_size == 0u) {
        return 0;
    }

    output[0] = '\0';

    if (resource_path_try_candidate(relative_path, output, output_size)) {
        return 1;
    }

    if (resource_path_executable_dir(executable_dir, sizeof(executable_dir))) {
        if (resource_path_join(executable_dir, relative_path, candidate, sizeof(candidate)) &&
            resource_path_try_candidate(candidate, output, output_size)) {
            return 1;
        }
        if (resource_path_join(executable_dir, "../share/gldraw", install_root, sizeof(install_root)) &&
            resource_path_join(install_root, relative_path, candidate, sizeof(candidate)) &&
            resource_path_try_candidate(candidate, output, output_size)) {
            return 1;
        }
    }

    if (resource_path_join(GLDRAW_RESOURCE_ROOT, relative_path, candidate, sizeof(candidate)) &&
        resource_path_try_candidate(candidate, output, output_size)) {
        return 1;
    }

    resource_path_copy(relative_path, output, output_size);
    return 0;
}
