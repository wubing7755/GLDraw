/**
 * @file path_utils.h
 * @brief Shared path and filename helpers.
 */
#ifndef GLDRAW_BASE_PATH_UTILS_H
#define GLDRAW_BASE_PATH_UTILS_H

#include <stddef.h>

#define GLDRAW_PATH_MAX 260

/** Return the basename portion of a path, or fallback when the path is empty. */
const char* path_utils_basename_or_default(const char* path, const char* fallback);
/** Copy the directory portion of a path, or "." when the path has no directory. */
int path_utils_dirname(const char* path, char* buffer, size_t buffer_size);
/** Case-insensitive extension test. Extension should include the leading dot. */
int path_utils_has_extension(const char* path, const char* extension);
/** Copy input without leading/trailing whitespace. */
int path_utils_copy_trimmed(const char* input, char* output, size_t output_size);
/** Validate a simple filename without path separators or reserved characters. */
int path_utils_is_safe_filename(const char* filename);
/** Join filename into the same directory as base_path, adding default_extension when missing. */
int path_utils_join_same_directory(const char* base_path,
                                   const char* filename,
                                   const char* default_extension,
                                   char* output,
                                   size_t output_size);

#endif /* GLDRAW_BASE_PATH_UTILS_H */
