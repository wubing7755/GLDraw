/**
 * @file ui_menubar.h
 * @brief 顶部菜单栏模块公开接口。
 */
#ifndef GLDRAW_UI_UI_MENUBAR_H
#define GLDRAW_UI_UI_MENUBAR_H

#include <stdbool.h>
#include <stddef.h>

struct nk_context;
struct Workspace;
struct UiThemeDescriptor;
typedef struct UiThemeDescriptor UiThemeDescriptor;

/**
 * @struct UiMenuBar
 * @brief 菜单栏运行时状态。
 *
 * @member ctx Nuklear 上下文（不拥有）。
 * @member show_inspector 检查器面板可见性。
 * @member menu_height 菜单栏高度。
 * @member themes 主题描述符数组（不拥有）。
 * @member theme_count 主题数量。
 * @member active_theme_index 当前激活主题索引。
 * @member requested_theme_index 待应用主题索引（-1 表示无请求）。
 * @member requested_theme_reload 是否请求重载外部主题。
 */
typedef struct UiMenuBar {
    struct nk_context* ctx;
    bool show_inspector;
    float menu_height;
    const UiThemeDescriptor* themes;
    int theme_count;
    int active_theme_index;
    int requested_theme_index;
    int requested_theme_reload;
} UiMenuBar;

/**
 * @brief 创建菜单栏实例。
 * @param ctx Nuklear 上下文指针。
 * @return 成功返回 `UiMenuBar*`，失败返回 `NULL`。
 */
UiMenuBar* ui_menubar_create(void* ctx);

/**
 * @brief 销毁菜单栏实例。
 * @param menubar 菜单栏对象。
 * @return 无。
 */
void ui_menubar_destroy(UiMenuBar* menubar);

/**
 * @brief 构建并绘制菜单栏。
 * @param menubar 菜单栏对象。
 * @param workspace 工作区对象。
 * @param window_width 当前窗口宽度。
 * @return 无。
 */
void ui_menubar_build(UiMenuBar* menubar, struct Workspace* workspace, int window_width);

/**
 * @brief 查询检查器面板是否可见。
 * @param menubar 菜单栏对象。
 * @return 可见返回 `true`，不可见返回 `false`。
 */
bool ui_menubar_inspector_visible(const UiMenuBar* menubar);

/**
 * @brief 获取菜单栏高度。
 * @param menubar 菜单栏对象。
 * @return 菜单栏高度（像素）。
 */
float ui_menubar_height(const UiMenuBar* menubar);

/**
 * @brief 设置菜单栏高度。
 * @param menubar 菜单栏对象。
 * @param height 新高度（像素）。
 * @return 无。
 */
void ui_menubar_set_height(UiMenuBar* menubar, float height);

/**
 * @brief 设置菜单栏可用主题列表。
 * @param menubar 菜单栏对象。
 * @param themes 主题描述符数组。
 * @param theme_count 数组长度。
 * @return 无。
 */
void ui_menubar_set_themes(UiMenuBar* menubar, const UiThemeDescriptor* themes, int theme_count);

/**
 * @brief 设置当前激活主题索引。
 * @param menubar 菜单栏对象。
 * @param theme_index 目标主题索引。
 * @return 无。
 */
void ui_menubar_set_active_theme_index(UiMenuBar* menubar, int theme_index);

/**
 * @brief 取出并清空待应用主题请求。
 * @param menubar 菜单栏对象。
 * @return 请求的主题索引；无请求时返回 `-1`。
 */
int ui_menubar_take_theme_request(UiMenuBar* menubar);

/**
 * @brief 取出并清空主题重载请求标记。
 * @param menubar 菜单栏对象。
 * @return 有请求返回 `1`，无请求返回 `0`。
 */
int ui_menubar_take_theme_reload_request(UiMenuBar* menubar);

#endif /* GLDRAW_UI_UI_MENUBAR_H */
