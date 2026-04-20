/**
 * @file log.h
 * @brief 轻量级日志输出宏与实现。
 *
 * 日志统一输出到 `stderr`，并带有级别、文件与行号信息。
 */
#ifndef GLDRAW_BASE_LOG_H
#define GLDRAW_BASE_LOG_H

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief 内部日志实现函数（单行输出到 `stderr`）。
 * @param level 日志级别字符串。
 * @param file 调用位置文件名。
 * @param line 调用位置行号。
 * @param fmt `printf` 风格格式串。
 * @param ... 与 `fmt` 匹配的可变参数。
 * @return 无。
 * @note 调用方必须保证格式串与参数类型一致。
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
 * @brief Debug 日志宏（仅在非 `NDEBUG` 构建启用）。
 */
#ifndef NDEBUG
#define LOG_DEBUG(fmt, ...) log_write_impl("DEBUG", __FILE__, __LINE__, fmt, __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

/** @def LOG_INFO
 * @brief Info 日志宏。
 */
#define LOG_INFO(fmt, ...) log_write_impl("INFO", __FILE__, __LINE__, fmt, __VA_ARGS__)
/** @def LOG_WARN
 * @brief Warn 日志宏。
 */
#define LOG_WARN(fmt, ...) log_write_impl("WARN", __FILE__, __LINE__, fmt, __VA_ARGS__)
/** @def LOG_ERROR
 * @brief Error 日志宏。
 */
#define LOG_ERROR(fmt, ...) log_write_impl("ERROR", __FILE__, __LINE__, fmt, __VA_ARGS__)

#endif /* GLDRAW_BASE_LOG_H */
