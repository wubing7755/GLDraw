/**
 * @file log.h
 * @brief Lightweight log output macros and implementation.
 *
 * All logs are output to `stderr` with level, file, and line number information.
 */
#ifndef GLDRAW_BASE_LOG_H
#define GLDRAW_BASE_LOG_H

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Internal log implementation function (single line to `stderr`).
 * @param level Log level string.
 * @param file Caller's file name.
 * @param line Caller's line number.
 * @param fmt `printf`-style format string.
 * @param ... Variable arguments matching `fmt`.
 * @return No return value.
 * @note Callers must ensure the format string matches the argument types.
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

/** @def LOG_DEBUG
 * @brief Debug log macro (enabled only in non-`NDEBUG` builds).
 */
#ifndef NDEBUG
#define LOG_DEBUG(fmt, ...) log_write_impl("DEBUG", __FILE__, __LINE__, fmt, __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

/** @def LOG_INFO
 * @brief Info log macro.
 */
#define LOG_INFO(fmt, ...) log_write_impl("INFO", __FILE__, __LINE__, fmt, __VA_ARGS__)
/** @def LOG_WARN
 * @brief Warning log macro.
 */
#define LOG_WARN(fmt, ...) log_write_impl("WARN", __FILE__, __LINE__, fmt, __VA_ARGS__)
/** @def LOG_ERROR
 * @brief Error log macro.
 */
#define LOG_ERROR(fmt, ...) log_write_impl("ERROR", __FILE__, __LINE__, fmt, __VA_ARGS__)

#endif /* GLDRAW_BASE_LOG_H */
