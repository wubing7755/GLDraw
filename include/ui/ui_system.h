/**
 * @file ui_system.h
 * @brief UI 系统生命周期与布局查询接口。
 */
#ifndef GLDRAW_UI_UI_SYSTEM_H
#define GLDRAW_UI_UI_SYSTEM_H

#include <base/types.h>
#include <platform/window.h>

struct Workspace;

typedef struct UiSystem UiSystem;

/**
 * @brief 创建 UI 系统实例。
 * @param window 已初始化的平台窗口。
 * @return 创建成功返回 `UiSystem*`，失败返回 `NULL`。
 */
UiSystem* ui_system_create(PlatformWindow* window);

/**
 * @brief 销毁 UI 系统并释放资源。
 * @param ui UI 系统实例。
 * @return 无。
 */
void ui_system_destroy(UiSystem* ui);

/**
 * @brief 开始一帧 UI 构建。
 * @param ui UI 系统实例。
 * @return 无。
 */
void ui_system_begin_frame(UiSystem* ui);

/**
 * @brief 构建整帧 UI 并回写工作区状态。
 * @param ui UI 系统实例。
 * @param workspace 工作区。
 * @return 无。
 */
void ui_system_build(UiSystem* ui, struct Workspace* workspace);

/**
 * @brief 提交 UI 绘制命令。
 * @param ui UI 系统实例。
 * @return 无。
 */
void ui_system_render(UiSystem* ui);

/**
 * @brief 查询 UI 是否存在进行中的交互。
 * @param ui UI 系统实例。
 * @return 有交互返回非零，否则返回 0。
 */
int ui_system_has_active_interaction(const UiSystem* ui);

/**
 * @brief 判断屏幕点是否被 UI 区域拦截。
 * @param ui UI 系统实例。
 * @param screen_pos 屏幕坐标点。
 * @return 被 UI 区域拦截返回非零，否则返回 0。
 */
int ui_system_blocks_pointer(const UiSystem* ui, Vec2 screen_pos);

/**
 * @brief 获取窗口命中测试边界。
 * @param ui UI 系统实例。
 * @return 窗口边界矩形。
 */
RectF ui_system_window_bounds(const UiSystem* ui);

/**
 * @brief 获取画布内容区边界。
 * @param ui UI 系统实例。
 * @return 内容区矩形。
 */
RectF ui_system_content_bounds(const UiSystem* ui);

/**
 * @brief 判断点是否位于画布内容区。
 * @param ui UI 系统实例。
 * @param screen_pos 屏幕坐标点。
 * @return 位于画布区域返回非零，否则返回 0。
 */
int ui_system_point_in_canvas(const UiSystem* ui, Vec2 screen_pos);

/**
 * @brief 获取当前主题的画布背景色。
 * @param ui UI 系统实例。
 * @return 画布背景色。
 */
Color ui_system_canvas_background(const UiSystem* ui);

#endif /* GLDRAW_UI_UI_SYSTEM_H */
