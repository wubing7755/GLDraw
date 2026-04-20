/**
 * @file workspace.h
 * @brief 编辑器运行时共享工作区定义。
 */
#ifndef GLDRAW_APP_WORKSPACE_H
#define GLDRAW_APP_WORKSPACE_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/history.h>
#include <tools/tool_controller.h>

struct Workspace;

/**
 * @typedef WorkspaceCommandFn
 * @brief 工作区命令回调签名（如保存/加载）。
 * @param workspace 目标工作区。
 * @param user_data 调用方透传上下文。
 * @return 成功返回非零，失败返回 0。
 */
typedef int (*WorkspaceCommandFn)(struct Workspace* workspace, void* user_data);

/**
 * @struct WorkspaceLayout
 * @brief UI 计算出的布局快照。
 *
 * @member window_bounds 窗口边界。
 * @member canvas_content_bounds 画布可用内容区。
 * @member appbar_bounds 顶部工具条区域。
 * @member rail_bounds 左侧工具轨区域。
 * @member panel_bounds 右侧面板区域。
 * @member status_bounds 底部状态栏区域。
 * @member layout_revision 布局版本号（用于跨系统同步）。
 */
typedef struct WorkspaceLayout {
    RectF window_bounds;
    RectF canvas_content_bounds;
    RectF appbar_bounds;
    RectF rail_bounds;
    RectF panel_bounds;
    RectF status_bounds;
    unsigned int layout_revision;
} WorkspaceLayout;

/**
 * @struct Workspace
 * @brief 运行时核心状态容器。
 *
 * @member document 当前文档对象与选择状态。
 * @member history 撤销/重做历史。
 * @member canvas 画布视图状态。
 * @member tools 工具控制器。
 * @member layout UI 布局信息。
 * @member current_document_path 当前文档路径（空串表示未命名文档）。
 * @member status_message 状态栏消息。
 * @member saved_revision 最近一次保存对应的文档修订号。
 * @member document_dirty 文档是否脏（非零表示有未保存改动）。
 * @member save_document 保存回调。
 * @member load_document 加载回调。
 * @member command_user_data 回调上下文。
 */
typedef struct Workspace {
    Document document;
    DocumentHistory history;
    CanvasView canvas;
    ToolController tools;
    WorkspaceLayout layout;
    char current_document_path[260];
    char status_message[256];
    unsigned int saved_revision;
    int document_dirty;
    WorkspaceCommandFn save_document;
    WorkspaceCommandFn load_document;
    void* command_user_data;
} Workspace;

/**
 * @brief 将当前文档修订号标记为“已保存”。
 * @param workspace 目标工作区，为 `NULL` 时无操作。
 * @return 无。
 */
static inline void workspace_mark_saved(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->saved_revision = workspace->document.revision;
    workspace->document_dirty = 0;
}

/**
 * @brief 根据 `saved_revision` 与当前修订号同步脏标记。
 * @param workspace 目标工作区，为 `NULL` 时无操作。
 * @return 无。
 */
static inline void workspace_sync_document_dirty(Workspace* workspace)
{
    if (!workspace) {
        return;
    }

    workspace->document_dirty = (workspace->saved_revision != workspace->document.revision);
}

#endif /* GLDRAW_APP_WORKSPACE_H */
