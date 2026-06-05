#include "test_temp_files.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define test_temp_mkdir(path) _mkdir(path)
#define test_temp_rmdir(path) _rmdir(path)
#define test_temp_getpid() ((unsigned long)_getpid())
#define test_temp_access(path) _access((path), 0)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define test_temp_mkdir(path) mkdir((path), 0777)
#define test_temp_rmdir(path) rmdir(path)
#define test_temp_getpid() ((unsigned long)getpid())
#define test_temp_access(path) access((path), F_OK)
#endif

static unsigned int g_test_temp_counter = 0u;

static const char* test_temp_base_dir(void)
{
    const char* value = NULL;

#ifdef _WIN32
    value = getenv("TEMP");
    if (value && value[0] != '\0') {
        return value;
    }
    value = getenv("TMP");
#else
    value = getenv("TMPDIR");
#endif

    return (value && value[0] != '\0') ? value : ".";
}

static int test_temp_needs_separator(const char* path)
{
    size_t length = path ? strlen(path) : 0u;

    if (length == 0u) {
        return 0;
    }
    return path[length - 1u] != '/' && path[length - 1u] != '\\';
}

static int test_temp_path_exists(const char* path)
{
    if (!path) {
        return 0;
    }
    return test_temp_access(path) == 0;
}

int test_temp_make_path(char* buffer,
                        size_t buffer_size,
                        const char* prefix,
                        const char* suffix)
{
    const char* base_dir = test_temp_base_dir();
    const char* safe_prefix = (prefix && prefix[0] != '\0') ? prefix : "file";
    const char* safe_suffix = suffix ? suffix : "";
    const char separator = test_temp_needs_separator(base_dir) ? '/' : '\0';
    unsigned long pid = test_temp_getpid();
    int attempt = 0;

    if (!buffer || buffer_size == 0u) {
        return 0;
    }

    for (attempt = 0; attempt < 1000; ++attempt) {
        int written = 0;
        unsigned int sequence = ++g_test_temp_counter;

        if (separator != '\0') {
            written = snprintf(buffer,
                               buffer_size,
                               "%s%c%s-%lu-%u%s",
                               base_dir,
                               separator,
                               safe_prefix,
                               pid,
                               sequence,
                               safe_suffix);
        } else {
            written = snprintf(buffer,
                               buffer_size,
                               "%s%s-%lu-%u%s",
                               base_dir,
                               safe_prefix,
                               pid,
                               sequence,
                               safe_suffix);
        }

        if (written < 0 || (size_t)written >= buffer_size) {
            return 0;
        }
        if (!test_temp_path_exists(buffer)) {
            return 1;
        }
    }

    return 0;
}

int test_temp_make_dir(char* buffer, size_t buffer_size, const char* prefix)
{
    int attempt = 0;

    if (!buffer || buffer_size == 0u) {
        return 0;
    }

    for (attempt = 0; attempt < 1000; ++attempt) {
        if (!test_temp_make_path(buffer, buffer_size, prefix, "")) {
            return 0;
        }
        if (test_temp_mkdir(buffer) == 0) {
            return 1;
        }
        if (errno != EEXIST) {
            return 0;
        }
    }

    return 0;
}

int test_temp_join_path(char* buffer,
                        size_t buffer_size,
                        const char* directory,
                        const char* filename)
{
    int written = 0;
    const char separator = test_temp_needs_separator(directory) ? '/' : '\0';

    if (!buffer || buffer_size == 0u || !directory || !filename) {
        return 0;
    }

    if (separator != '\0') {
        written = snprintf(buffer,
                           buffer_size,
                           "%s%c%s",
                           directory,
                           separator,
                           filename);
    } else {
        written = snprintf(buffer, buffer_size, "%s%s", directory, filename);
    }

    return written >= 0 && (size_t)written < buffer_size;
}

int test_temp_remove_dir(const char* path)
{
    if (!path || path[0] == '\0') {
        return 0;
    }
    return test_temp_rmdir(path) == 0;
}
