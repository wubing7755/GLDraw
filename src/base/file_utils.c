/**
 * @file file_utils.c
 * @brief Shared file/path helpers for small text and temp-file workflows.
 */
#include <base/file_utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

char* file_utils_read_text_file(const char* path)
{
    FILE* file = NULL;
    char* buffer = NULL;
    long size = 0;
    size_t read_size = 0u;

    if (!path || path[0] == '\0') {
        return NULL;
    }

    file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char*)malloc((size_t)size + 1u);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1u, (size_t)size, file);
    if (read_size != (size_t)size && ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[read_size] = '\0';
    fclose(file);
    return buffer;
}

char* file_utils_duplicate_path_with_suffix(const char* path, const char* suffix)
{
    size_t path_length = 0u;
    size_t suffix_length = 0u;
    char* result = NULL;

    if (!path || !suffix) {
        return NULL;
    }

    path_length = strlen(path);
    suffix_length = strlen(suffix);
    result = (char*)malloc(path_length + suffix_length + 1u);
    if (!result) {
        return NULL;
    }

    memcpy(result, path, path_length);
    memcpy(result + path_length, suffix, suffix_length);
    result[path_length + suffix_length] = '\0';
    return result;
}

int file_utils_path_exists(const char* path)
{
    FILE* file = NULL;

    if (!path || path[0] == '\0') {
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    fclose(file);
    return 1;
}

int file_utils_replace_file_with_temp(const char* temp_path,
                                      const char* target_path)
{
    if (!temp_path || !target_path) {
        return 0;
    }

#ifdef _WIN32
    if (MoveFileExA(temp_path,
                    target_path,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return 1;
    }
#else
    if (rename(temp_path, target_path) == 0) {
        return 1;
    }
#endif

    remove(temp_path);
    return 0;
}
