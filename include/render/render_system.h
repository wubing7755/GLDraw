/**
 * @file render_system.h
 * @brief OpenGL 渲染系统接口。
 */
#ifndef GLDRAW_RENDER_RENDER_SYSTEM_H
#define GLDRAW_RENDER_RENDER_SYSTEM_H

#include <canvas/canvas_view.h>
#include <document/document.h>
#include <platform/window.h>

typedef struct RenderSystem RenderSystem;

/**
 * @brief 创建渲染系统并初始化 GPU 资源。
 * @param window 已初始化的窗口与 OpenGL 上下文。
 * @return 创建成功返回渲染系统指针，失败返回 `NULL`。
 */
RenderSystem* render_system_create(PlatformWindow* window);

/**
 * @brief 销毁渲染系统并释放 GPU/堆资源。
 * @param renderer 渲染系统实例。
 * @return 无。
 */
void render_system_destroy(RenderSystem* renderer);

/**
 * @brief 处理窗口尺寸变化。
 * @param renderer 渲染系统实例。
 * @param width 新宽度（像素）。
 * @param height 新高度（像素）。
 * @return 无。
 */
void render_system_resize(RenderSystem* renderer, int width, int height);

/**
 * @brief 绘制一帧画布内容。
 *
 * 主要流程：清屏 -> 绘制网格/坐标轴 -> 绘制文档对象 -> 绘制工具叠加预览。
 *
 * @param renderer 渲染系统实例。
 * @param document 当前文档。
 * @param canvas 当前画布视图。
 * @param overlay_object 工具预览对象（可为 `NULL`）。
 * @return 无。
 */
void render_system_draw(RenderSystem* renderer,
                        const Document* document,
                        const CanvasView* canvas,
                        const GraphicObject* overlay_object);

#endif /* GLDRAW_RENDER_RENDER_SYSTEM_H */
