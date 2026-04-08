#ifndef MACROS_H
#define MACROS_H

/*
 * C11 标准日志库 + 工程常用宏
 *
 * 特点：
 * - 完全符合 ISO C11（-std=c11 -pedantic 无警告）
 * - 日志分无参版 / 有参版（双宏设计）
 *     LOG_INFO("text")      — 无额外参数
 *     LOG_INFO_F("val: %d", x) — 有额外参数
 * - 支持日志级别控制
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

/* =============================================================================
 * 1. 日志分级宏
 * ============================================================================= */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

#ifndef LOG_ACTIVE_LEVEL
    #ifdef NDEBUG
        #define LOG_ACTIVE_LEVEL LOG_LEVEL_INFO
    #else
        #define LOG_ACTIVE_LEVEL LOG_LEVEL_DEBUG
    #endif
#endif

/* =============================================================================
 * 2. 日志内部实现（标准 C，va_list）
 * ============================================================================= */

static inline void log_impl(const char* level,
                           const char* file,
                           int line,
                           const char* fmt,
                           va_list args)
{
    fprintf(stderr, "[%s] [%s:%d] ", level, file, line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static inline void log_write(const char* level,
                            const char* file,
                            int line,
                            const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_impl(level, file, line, fmt, args);
    va_end(args);
}

/* =============================================================================
 * 3. 日志宏（C11 双宏：无参版 / 有参版）
 * ============================================================================= */

/* ---------- DEBUG ---------- */
#define LOG_DEBUG(msg) \
    do { \
        if (LOG_LEVEL_DEBUG >= LOG_ACTIVE_LEVEL) \
            log_write("DEBUG", __FILE__, __LINE__, "%s", msg); \
    } while (0)

#define LOG_DEBUG_F(fmt, ...) \
    do { \
        if (LOG_LEVEL_DEBUG >= LOG_ACTIVE_LEVEL) \
            log_write("DEBUG", __FILE__, __LINE__, fmt, __VA_ARGS__); \
    } while (0)

/* ---------- INFO ---------- */
#define LOG_INFO(msg) \
    do { \
        if (LOG_LEVEL_INFO >= LOG_ACTIVE_LEVEL) \
            log_write("INFO", __FILE__, __LINE__, "%s", msg); \
    } while (0)

#define LOG_INFO_F(fmt, ...) \
    do { \
        if (LOG_LEVEL_INFO >= LOG_ACTIVE_LEVEL) \
            log_write("INFO", __FILE__, __LINE__, fmt, __VA_ARGS__); \
    } while (0)

/* ---------- WARN ---------- */
#define LOG_WARN(msg) \
    do { \
        if (LOG_LEVEL_WARN >= LOG_ACTIVE_LEVEL) \
            log_write("WARN", __FILE__, __LINE__, "%s", msg); \
    } while (0)

#define LOG_WARN_F(fmt, ...) \
    do { \
        if (LOG_LEVEL_WARN >= LOG_ACTIVE_LEVEL) \
            log_write("WARN", __FILE__, __LINE__, fmt, __VA_ARGS__); \
    } while (0)

/* ---------- ERROR ---------- */
#define LOG_ERROR(msg) \
    do { \
        if (LOG_LEVEL_ERROR >= LOG_ACTIVE_LEVEL) \
            log_write("ERROR", __FILE__, __LINE__, "%s", msg); \
    } while (0)

#define LOG_ERROR_F(fmt, ...) \
    do { \
        if (LOG_LEVEL_ERROR >= LOG_ACTIVE_LEVEL) \
            log_write("ERROR", __FILE__, __LINE__, fmt, __VA_ARGS__); \
    } while (0)

/* =============================================================================
 * 4. 条件日志
 * ============================================================================= */

#define LOG_DEBUG_F_IF(cond, fmt, ...) \
    do { if (cond) LOG_DEBUG_F(fmt, __VA_ARGS__); } while (0)

#define LOG_INFO_F_IF(cond, fmt, ...) \
    do { if (cond) LOG_INFO_F(fmt, __VA_ARGS__); } while (0)

#define LOG_WARN_F_IF(cond, fmt, ...) \
    do { if (cond) LOG_WARN_F(fmt, __VA_ARGS__); } while (0)

#define LOG_ERROR_F_IF(cond, fmt, ...) \
    do { if (cond) LOG_ERROR_F(fmt, __VA_ARGS__); } while (0)

/* =============================================================================
 * 5. 基础 HERE / TRACE 宏
 * ============================================================================= */

#define HERE() \
    do { \
        fprintf(stderr, "执行到 %s:%d\n", __FILE__, __LINE__); \
    } while (0)

#define TRACE_ENTER() \
    do { \
        fprintf(stderr, ">>> 进入 %s [%s:%d]\n", __func__, __FILE__, __LINE__); \
    } while (0)

#define TRACE_EXIT() \
    do { \
        fprintf(stderr, "<<< 离开 %s [%s:%d]\n", __func__, __FILE__, __LINE__); \
    } while (0)

/* =============================================================================
 * 6. 字符串化宏 (# / ##)
 * ============================================================================= */

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

/* =============================================================================
 * 7. 编译时断言 (C11)
 * ============================================================================= */

#define STATIC_ASSERT(expr) _Static_assert(expr, STRINGIFY(expr))
#define STATIC_ASSERT_MSG(expr, msg) _Static_assert(expr, msg)

/* =============================================================================
 * 8. 数组大小宏
 * ============================================================================= */

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

/* =============================================================================
 * 9. Struct 字段偏移 / 大小宏
 * ============================================================================= */

#define OFFSETOF(type, field) offsetof(type, field)
#define SIZEOF_MEMBER(type, field) sizeof(((type *)0)->field)

/* =============================================================================
 * 10. 分支预测优化宏 (GCC / Clang)
 * ============================================================================= */

#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

/* =============================================================================
 * 11. 属性宏 (noreturn, unused, deprecated, packed, aligned)
 * ============================================================================= */

#if defined(__GNUC__) || defined(__clang__)
    #define NORETURN    __attribute__((noreturn))
    #define UNUSED      __attribute__((unused))
    #define DEPRECATED  __attribute__((deprecated))
    #define PACKED      __attribute__((packed))
    #define ALIGNED(n)  __attribute__((aligned(n)))
#elif defined(_MSC_VER)
    #define NORETURN    __declspec(noreturn)
    #define UNUSED
    #define DEPRECATED  __declspec(deprecated)
    #define PACKED
    #define ALIGNED(n)  __declspec(align(n))
#else
    #define NORETURN
    #define UNUSED
    #define DEPRECATED
    #define PACKED
    #define ALIGNED(n)
#endif

/* =============================================================================
 * 12. 位标志操作宏
 * ============================================================================= */

#define FLAG_SET(flags, bit)       ((flags) |= (bit))
#define FLAG_CLEAR(flags, bit)    ((flags) &= ~(bit))
#define FLAG_TOGGLE(flags, bit)   ((flags) ^= (bit))
#define FLAG_TEST(flags, bit)     ((flags) & (bit))

/* =============================================================================
 * 13. 错误传播宏
 * ============================================================================= */

#define ERR_PROPAGATE(err) \
    do { if ((err) != 0) return (err); } while (0)

#define ERR_RETURN(err, ret) \
    do { if ((err) != 0) return (ret); } while (0)

/* =============================================================================
 * 14. Safe delete / release 宏
 * ============================================================================= */

#define SAFE_FREE(ptr) \
    do { free(ptr); (ptr) = NULL; } while (0)

#define SAFE_DELETE(obj, destroy_fn) \
    do { if (obj) { destroy_fn(obj); (obj) = NULL; } } while (0)

#endif /* MACROS_H */
