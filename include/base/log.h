/**
 * @file log.h
 * @brief Lightweight stderr logging helpers.
 *
 * Role in project:
 * - Provides file/line scoped debug/info/warn/error logging macros.
 * - Keeps logging dependency-free for low-level modules.
 *
 * Module relationships:
 * - Used by app/document/persistence and other runtime subsystems.
 */
#ifndef GLDRAW_BASE_LOG_H
#define GLDRAW_BASE_LOG_H

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Internal varargs logger writing one line to `stderr`.
 * @param level [in] Log level tag.
 * @param file [in] Source file path.
 * @param line [in] Source line number.
 * @param fmt [in] `printf`-style format string.
 * @param ... [in] Format arguments.
 * @return None.
 *
 * Edge cases:
 * - Caller must pass a valid format string and matching varargs.
 * - Return values of `fprintf`/`vfprintf` are intentionally ignored.
 *
 * Time complexity: `O(L)` where `L` is formatted output length.
 */
static inline void log_write_impl(const char* level,
                                  const char* file,
                                  int line,
                                  const char* fmt,
                                  ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[%s] [%s:%d] ", level, file, line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#ifndef NDEBUG
#define LOG_DEBUG(fmt, ...) log_write_impl("DEBUG", __FILE__, __LINE__, fmt, __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#define LOG_INFO(fmt, ...) log_write_impl("INFO", __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) log_write_impl("WARN", __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_write_impl("ERROR", __FILE__, __LINE__, fmt, __VA_ARGS__)

#endif /* GLDRAW_BASE_LOG_H */
