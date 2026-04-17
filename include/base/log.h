/**
 * @file log.h
 * @brief Lightweight stderr logging helpers.
 *
 * Role in project:
 * - Provides file/line scoped debug/info/warn/error logging macros.
 * - Keeps logging dependency-free for low-level modules.
 *
 * Module relationships:
 * - Used by app, canvas, document, render, tools, and UI subsystems.
 */
#ifndef GLDRAW_BASE_LOG_H
#define GLDRAW_BASE_LOG_H

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Internal varargs logger writing one line to `stderr`.
 * @param level [in] Log level tag string.
 * @param file [in] Source file path from caller.
 * @param line [in] Source line number from caller.
 * @param fmt [in] `printf`-style format string.
 * @param ... [in] Format arguments.
 *
 * Note:
 * - Return values of `fprintf`/`vfprintf` are intentionally ignored.
 * - Caller must ensure `fmt` and varargs match.
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
