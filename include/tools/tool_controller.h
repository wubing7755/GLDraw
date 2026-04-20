/**
 * @file tool_controller.h
 * @brief 活动工具管理与输入分发接口。
 */
#ifndef GLDRAW_TOOLS_TOOL_CONTROLLER_H
#define GLDRAW_TOOLS_TOOL_CONTROLLER_H

#include <tools/tool.h>

/**
 * @struct ToolController
 * @brief 工具控制器状态。
 *
 * @member tools 工具实例数组。
 * @member active_kind 当前激活工具类型。
 * @member pointer_captured 指针是否被工具捕获。
 * @member last_screen 最近一次屏幕坐标。
 * @member last_world 最近一次世界坐标。
 */
typedef struct {
    Tool tools[TOOL_KIND_COUNT];
    ToolKind active_kind;
    int pointer_captured;
    Vec2 last_screen;
    Vec2 last_world;
} ToolController;

/**
 * @brief 初始化工具控制器。
 * @param controller 控制器实例。
 * @return 无。
 */
void tool_controller_init(ToolController* controller);

/**
 * @brief 关闭工具控制器并释放工具状态。
 * @param controller 控制器实例。
 * @return 无。
 */
void tool_controller_shutdown(ToolController* controller);

/**
 * @brief 切换当前激活工具。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param kind 目标工具类型。
 * @return 无。
 */
void tool_controller_set_active(ToolController* controller, ToolContext* context, ToolKind kind);

/**
 * @brief 获取当前激活工具。
 * @param controller 控制器实例。
 * @return 激活工具指针；参数非法时返回 `NULL`。
 */
Tool* tool_controller_get_active(ToolController* controller);

/**
 * @brief 获取当前激活工具标签文本。
 * @param controller 控制器实例。
 * @return 工具标签字符串。
 */
const char* tool_controller_active_label(const ToolController* controller);

/**
 * @brief 获取当前激活工具的叠加预览对象。
 * @param controller 控制器实例。
 * @return 叠加对象指针；无预览时返回 `NULL`。
 */
GraphicObject* tool_controller_overlay_object(const ToolController* controller);

/**
 * @brief 分发鼠标按下事件。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param event 输入事件。
 * @return 无。
 */
void tool_controller_pointer_down(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief 分发鼠标移动事件。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param event 输入事件。
 * @return 无。
 */
void tool_controller_pointer_move(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief 分发鼠标释放事件。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param event 输入事件。
 * @return 无。
 */
void tool_controller_pointer_up(ToolController* controller, ToolContext* context, const ToolEvent* event);

/**
 * @brief 分发键盘按下事件。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param key GLFW 键值。
 * @param mods 修饰键掩码。
 * @return 无。
 */
void tool_controller_key_down(ToolController* controller, ToolContext* context, int key, int mods);

/**
 * @brief 处理滚轮缩放事件。
 * @param controller 控制器实例。
 * @param context 工具上下文。
 * @param screen_pos 当前屏幕坐标锚点。
 * @param yoffset 滚轮 Y 增量。
 * @return 无。
 */
void tool_controller_scroll(ToolController* controller, ToolContext* context, Vec2 screen_pos, float yoffset);

#endif /* GLDRAW_TOOLS_TOOL_CONTROLLER_H */
