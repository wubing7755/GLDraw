/**
 * @file resource_path.h
 * @brief Runtime lookup helpers for bundled application resources.
 */
#ifndef GLDRAW_BASE_RESOURCE_PATH_H
#define GLDRAW_BASE_RESOURCE_PATH_H

#include <stddef.h>

/**
 * Resolve a bundled resource path.
 *
 * Search order is current working directory, executable directory, installed
 * share directory next to the executable prefix, then the configured source
 * resource root. The output is always populated with either a found path or the
 * original relative path fallback.
 */
int resource_path_resolve(const char* relative_path, char* output, size_t output_size);

#endif /* GLDRAW_BASE_RESOURCE_PATH_H */
