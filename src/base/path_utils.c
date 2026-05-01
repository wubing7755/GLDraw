/**
 * @file path_utils.c
 * @brief Shared path and filename helpers.
 */
#include <base/path_utils.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int path_utils_copy_literal(const char* text, char* buffer, size_t buffer_size)
{
    int written = 0;

    if (!text || !buffer || buffer_size == 0u) {
        return 0;
    }

    written = snprintf(buffer, buffer_size, "%s", text);
    return written >= 0 && (size_t)written < buffer_size;
}

const char* path_utils_basename_or_default(const char* path, const char* fallback)
{
    const char* basename = path;
    const char* cursor = NULL;

    if (!fallback || fallback[0] == '\0') {
        fallback = "";
    }
    if (!path || path[0] == '\0') {
        return fallback;
    }

    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            basename = cursor + 1;
        }
    }

    return (basename && basename[0] != '\0') ? basename : fallback;
}

int path_utils_dirname(const char* path, char* buffer, size_t buffer_size)
{
    const char* cursor = NULL;
    const char* last_separator = NULL;
    size_t length = 0u;

    if (!buffer || buffer_size == 0u) {
        return 0;
    }

    buffer[0] = '\0';
    if (!path || path[0] == '\0') {
        return path_utils_copy_literal(".", buffer, buffer_size);
    }

    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            last_separator = cursor;
        }
    }

    if (!last_separator) {
        return path_utils_copy_literal(".", buffer, buffer_size);
    }

    length = (size_t)(last_separator - path);
    if (length == 0u) {
        length = 1u;
    } else if (length == 2u && path[1] == ':') {
        length = 3u;
    }
    if (length >= buffer_size) {
        return 0;
    }

    memcpy(buffer, path, length);
    buffer[length] = '\0';
    return 1;
}

int path_utils_has_extension(const char* path, const char* extension)
{
    size_t path_length = 0u;
    size_t extension_length = 0u;
    size_t i = 0u;

    if (!path || !extension || extension[0] == '\0') {
        return 0;
    }

    path_length = strlen(path);
    extension_length = strlen(extension);
    if (path_length < extension_length) {
        return 0;
    }

    path += path_length - extension_length;
    for (i = 0u; i < extension_length; ++i) {
        if (tolower((unsigned char)path[i]) !=
            tolower((unsigned char)extension[i])) {
            return 0;
        }
    }

    return 1;
}

int path_utils_copy_trimmed(const char* input, char* output, size_t output_size)
{
    const char* start = input;
    const char* end = NULL;
    size_t length = 0u;

    if (!input || !output || output_size == 0u) {
        return 0;
    }

    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    length = (size_t)(end - start);
    if (length == 0u || length >= output_size) {
        output[0] = '\0';
        return 0;
    }

    memcpy(output, start, length);
    output[length] = '\0';
    return 1;
}

int path_utils_is_safe_filename(const char* filename)
{
    const char* cursor = NULL;

    if (!filename || filename[0] == '\0') {
        return 0;
    }
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        return 0;
    }

    for (cursor = filename; *cursor != '\0'; ++cursor) {
        unsigned char ch = (unsigned char)*cursor;
        if (ch < 32u || strchr("/\\:*?\"<>|", *cursor)) {
            return 0;
        }
    }

    return 1;
}

int path_utils_join_same_directory(const char* base_path,
                                   const char* filename,
                                   const char* default_extension,
                                   char* output,
                                   size_t output_size)
{
    const char* cursor = NULL;
    const char* last_separator = NULL;
    char final_filename[GLDRAW_PATH_MAX];
    size_t filename_length = 0u;
    size_t directory_length = 0u;
    size_t extension_length = default_extension ? strlen(default_extension) : 0u;

    if (!base_path || !filename || !output || output_size == 0u) {
        return 0;
    }

    if (extension_length > 0u && !path_utils_has_extension(filename, default_extension)) {
        if (strlen(filename) + extension_length + 1u > sizeof(final_filename)) {
            return 0;
        }
        snprintf(final_filename, sizeof(final_filename), "%s%s", filename, default_extension);
    } else {
        if (strlen(filename) + 1u > sizeof(final_filename)) {
            return 0;
        }
        snprintf(final_filename, sizeof(final_filename), "%s", filename);
    }

    for (cursor = base_path; cursor && *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            last_separator = cursor;
        }
    }

    filename_length = strlen(final_filename);
    if (!last_separator) {
        if (filename_length + 1u > output_size) {
            return 0;
        }
        snprintf(output, output_size, "%s", final_filename);
        return 1;
    }

    directory_length = (size_t)(last_separator - base_path) + 1u;
    if (directory_length + filename_length + 1u > output_size) {
        return 0;
    }

    memcpy(output, base_path, directory_length);
    memcpy(output + directory_length, final_filename, filename_length + 1u);
    return 1;
}
