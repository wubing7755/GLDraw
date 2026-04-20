/**
 * @file window.h
 * @brief GLFW 窗口薄封装接口。
 */
#ifndef GLDRAW_PLATFORM_WINDOW_H
#define GLDRAW_PLATFORM_WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/**
 * @struct PlatformWindow
 * @brief 平台窗口状态。
 *
 * @member handle GLFW 原生窗口句柄。
 * @member width 期望窗口宽度。
 * @member height 期望窗口高度。
 * @member title 窗口标题字符串。
 */
typedef struct {
    GLFWwindow* handle;
    int width;
    int height;
    const char* title;
} PlatformWindow;

/**
 * @brief 初始化 GLFW 并创建窗口。
 * @param window 窗口结构体输出地址。
 * @param width 目标宽度。
 * @param height 目标高度。
 * @param title 窗口标题。
 * @return 成功返回 `0`，失败返回 `-1`。
 */
int platform_window_init(PlatformWindow* window, int width, int height, const char* title);

/**
 * @brief 销毁窗口并清理 GLFW。
 * @param window 目标窗口。
 * @return 无。
 */
void platform_window_shutdown(PlatformWindow* window);

/**
 * @brief 轮询窗口事件。
 * @return 无。
 */
void platform_window_poll_events(void);

/**
 * @brief 交换窗口前后缓冲。
 * @param window 目标窗口。
 * @return 无。
 */
void platform_window_swap_buffers(PlatformWindow* window);

/**
 * @brief 查询窗口是否应关闭。
 * @param window 目标窗口。
 * @return 应关闭返回非零，否则返回 0。
 */
int platform_window_should_close(const PlatformWindow* window);

#endif /* GLDRAW_PLATFORM_WINDOW_H */
