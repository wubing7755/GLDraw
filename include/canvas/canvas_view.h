/**
 * @file canvas_view.h
 * @brief 画布视口状态与坐标变换接口。
 */
#ifndef GLDRAW_CANVAS_CANVAS_VIEW_H
#define GLDRAW_CANVAS_CANVAS_VIEW_H

#include <base/types.h>
#include <document/document.h>

/**
 * @struct CanvasViewportState
 * @brief 画布相机的规范状态。
 *
 * @member viewport 屏幕空间视口矩形。
 * @member center 世界坐标系下视口中心。
 * @member zoom 缩放因子。
 */
typedef struct CanvasViewportState {
    RectF viewport;
    Vec2 center;
    float zoom;
} CanvasViewportState;

/**
 * @struct CanvasView
 * @brief 画布运行时状态。
 *
 * @member document 参与拾取与渲染的数据文档（不拥有）。
 * @member viewport_state 视口/中心/缩放状态。
 * @member show_grid 是否显示网格。
 * @member background 画布背景色。
 */
typedef struct CanvasView {
    Document* document;
    CanvasViewportState viewport_state;
    int show_grid;
    Color background;
} CanvasView;

/**
 * @brief 初始化画布状态并绑定文档。
 * @param canvas 画布实例输出地址。
 * @param document 初始绑定文档（可为 `NULL`）。
 * @param viewport 初始屏幕视口。
 * @return 无。
 */
void canvas_view_init(CanvasView* canvas, Document* document, RectF viewport);

/**
 * @brief 重新绑定文档。
 * @param canvas 画布实例。
 * @param document 新文档指针。
 * @return 无。
 */
void canvas_view_set_document(CanvasView* canvas, Document* document);

/**
 * @brief 设置画布屏幕视口。
 * @param canvas 画布实例。
 * @param viewport 视口矩形。
 * @return 无。
 */
void canvas_view_set_viewport(CanvasView* canvas, RectF viewport);

/**
 * @brief 设置画布中心点（世界坐标）。
 * @param canvas 画布实例。
 * @param center 新中心点。
 * @return 无。
 */
void canvas_view_set_center(CanvasView* canvas, Vec2 center);

/**
 * @brief 设置缩放因子。
 * @param canvas 画布实例。
 * @param zoom 新缩放值（实现中会进行有效范围夹取）。
 * @return 无。
 */
void canvas_view_set_zoom(CanvasView* canvas, float zoom);

/**
 * @brief 原子设置中心与缩放。
 * @param canvas 画布实例。
 * @param center 新中心点。
 * @param zoom 新缩放值。
 * @return 无。
 */
void canvas_view_set_center_zoom(CanvasView* canvas, Vec2 center, float zoom);

/**
 * @brief 获取当前屏幕视口。
 * @param canvas 画布实例。
 * @return 当前视口矩形。
 */
RectF canvas_view_viewport(const CanvasView* canvas);

/**
 * @brief 获取当前世界坐标中心。
 * @param canvas 画布实例。
 * @return 当前中心点。
 */
Vec2 canvas_view_center(const CanvasView* canvas);

/**
 * @brief 获取当前缩放值。
 * @param canvas 画布实例。
 * @return 当前缩放因子。
 */
float canvas_view_zoom(const CanvasView* canvas);

/**
 * @brief 将世界坐标转换为屏幕坐标。
 * @param canvas 画布实例。
 * @param world 世界坐标点。
 * @return 对应的屏幕坐标点。
 */
Vec2 canvas_view_world_to_screen(const CanvasView* canvas, Vec2 world);

/**
 * @brief 将屏幕坐标转换为世界坐标。
 * @param canvas 画布实例。
 * @param screen 屏幕坐标点。
 * @return 对应的世界坐标点。
 */
Vec2 canvas_view_screen_to_world(const CanvasView* canvas, Vec2 screen);

/**
 * @brief 按屏幕位移平移画布。
 * @param canvas 画布实例。
 * @param delta_screen 屏幕空间位移增量。
 * @return 无。
 */
void canvas_view_pan_screen_delta(CanvasView* canvas, Vec2 delta_screen);

/**
 * @brief 以指定屏幕锚点执行缩放。
 *
 * 算法要点：先计算缩放前锚点对应世界坐标，再缩放并计算缩放后坐标，
 * 最后用两者差值修正中心点，实现“光标处缩放不漂移”。
 *
 * @param canvas 画布实例。
 * @param factor 缩放倍率（大于 1 放大，小于 1 缩小）。
 * @param screen_anchor 屏幕锚点。
 * @return 无。
 */
void canvas_view_zoom_at_screen_point(CanvasView* canvas, float factor, Vec2 screen_anchor);

/**
 * @brief 将像素容差换算为世界坐标容差。
 * @param canvas 画布实例。
 * @param pixels 像素单位容差。
 * @return 世界坐标单位容差。
 */
float canvas_view_world_tolerance_for_pixels(const CanvasView* canvas, float pixels);

/**
 * @brief 获取当前可见世界矩形。
 * @param canvas 画布实例。
 * @return 视口在世界坐标系下覆盖的矩形。
 */
RectF canvas_view_visible_world_rect(const CanvasView* canvas);

/**
 * @brief 在屏幕点执行对象拾取。
 *
 * 算法要点：从对象数组末尾逆序遍历（后绘制对象优先命中），
 * 并将像素容差换算到世界坐标后调用对象命中测试。
 *
 * @param canvas 画布实例。
 * @param screen_point 屏幕空间待拾取点。
 * @param tolerance_pixels 像素容差。
 * @return 命中对象指针；未命中或参数非法时返回 `NULL`。
 */
GraphicObject* canvas_view_pick_object(const CanvasView* canvas, Vec2 screen_point, float tolerance_pixels);

#endif /* GLDRAW_CANVAS_CANVAS_VIEW_H */
