/**
 * @file file_utils.h
 * @brief Shared file/path helpers for small text and temp-file workflows.
 */
#ifndef GLDRAW_BASE_FILE_UTILS_H
#define GLDRAW_BASE_FILE_UTILS_H

char* file_utils_read_text_file(const char* path);
char* file_utils_duplicate_path_with_suffix(const char* path, const char* suffix);
int file_utils_path_exists(const char* path);
int file_utils_replace_file_with_temp(const char* temp_path,
                                      const char* target_path);

#endif /* GLDRAW_BASE_FILE_UTILS_H */
