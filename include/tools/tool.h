/**
 * @file tool.h
 * @brief 工具系统抽象接口与事件数据结构。
 */
#ifndef GLDRAW_TOOLS_TOOL_H
#define GLDRAW_TOOLS_TOOL_H

#include <base/types.h>
#include <document/object.h>

struct Workspace;
struct Document;
struct CanvasView;
struct DocumentHistory;

/**
 * @enum ToolKind
 * @brief 内置工具类型。
 */
typedef enum {
    TOOL_KIND_SELECT = 0,
    TOOL_KIND_PAN,
    TOOL_KIND_LINE,
    TOOL_KIND_RECT,
    TOOL_KIND_ELLIPSE,
    TOOL_KIND_COUNT
} ToolKind;

/**
 * @struct ToolEvent
 * @brief 传递给工具的标准化输入事件。
 *
 * @member screen_pos 当前屏幕坐标。
 * @member world_pos 当前世界坐标。
 * @member delta_screen 相对上次事件的屏幕位移。
 * @member delta_world 相对上次事件的世界位移。
 * @member button 鼠标按键编号（移动事件通常为 `-1`）。
 * @member mods 修饰键位掩码。
 * @member wheel_y 滚轮 Y 方向增量。
 */
typedef struct {
    Vec2 screen_pos;
    Vec2 world_pos;
    Vec2 delta_screen;
    Vec2 delta_world;
    int button;
    int mods;
    float wheel_y;
} ToolEvent;

/**
 * @struct ToolContext
 * @brief 工具操作上下文。
 *
 * @member workspace 工作区。
 * @member document 当前文档。
 * @member history 历史栈。
 * @member canvas 画布视图。
 */
typedef struct {
    struct Workspace* workspace;
    struct Document* document;
    struct DocumentHistory* history;
    struct CanvasView* canvas;
} ToolContext;

typedef struct Tool Tool;
typedef struct ToolVTable ToolVTable;

/**
 * @struct ToolVTable
 * @brief 工具多态回调表。
 *
 * @member label 返回工具显示名称。
 * @member activate 工具激活回调。
 * @member deactivate 工具停用回调。
 * @member pointer_down 鼠标按下回调。
 * @member pointer_move 鼠标移动回调。
 * @member pointer_up 鼠标释放回调。
 * @member key_down 键盘按下回调。
 */
struct ToolVTable {
    const char* (*label)(const Tool* tool);
    void (*activate)(Tool* tool, ToolContext* context);
    void (*deactivate)(Tool* tool, ToolContext* context);
    void (*pointer_down)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_move)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*pointer_up)(Tool* tool, ToolContext* context, const ToolEvent* event);
    void (*key_down)(Tool* tool, ToolContext* context, int key, int mods);
};

/**
 * @struct Tool
 * @brief 工具运行时实例。
 *
 * @member kind 工具类型。
 * @member vtable 回调表。
 * @member state 工具私有状态。
 * @member overlay_object 工具预览图元。
 */
struct Tool {
    ToolKind kind;
    const ToolVTable* vtable;
    void* state;
    GraphicObject* overlay_object;
};

#endif /* GLDRAW_TOOLS_TOOL_H */
