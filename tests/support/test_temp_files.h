#ifndef GLDRAW_TEST_SUPPORT_TEST_TEMP_FILES_H
#define GLDRAW_TEST_SUPPORT_TEST_TEMP_FILES_H

#include <stddef.h>

#define TEST_TEMP_PATH_MAX 1024

int test_temp_make_path(char* buffer,
                        size_t buffer_size,
                        const char* prefix,
                        const char* suffix);
int test_temp_make_dir(char* buffer, size_t buffer_size, const char* prefix);
int test_temp_join_path(char* buffer,
                        size_t buffer_size,
                        const char* directory,
                        const char* filename);
int test_temp_remove_dir(const char* path);

#endif /* GLDRAW_TEST_SUPPORT_TEST_TEMP_FILES_H */
