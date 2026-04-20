/**
 * @file ui_theme.h
 * @brief UI 主题令牌与加载/应用接口。
 */
#ifndef GLDRAW_UI_UI_THEME_H
#define GLDRAW_UI_UI_THEME_H

#include <stddef.h>

#ifndef NK_NUKLEAR_H_
typedef unsigned char nk_byte;
typedef int nk_bool;
struct nk_color { nk_byte r, g, b, a; };
struct nk_context;
#endif

/**
 * @struct UiThemeTokens
 * @brief 一组完整的 UI 主题令牌。
 *
 * @member primary 主色。
 * @member primary_hover 主色悬停态。
 * @member primary_active 主色激活态。
 * @member background 全局背景色。
 * @member panel 面板背景色。
 * @member panel_hover 面板悬停色。
 * @member canvas_background 画布背景色。
 * @member text 主文本色。
 * @member text_secondary 次级文本色。
 * @member text_disabled 禁用文本色。
 * @member border 边框色。
 * @member border_hover 边框悬停色。
 * @member success 成功状态色。
 * @member warning 警告状态色。
 * @member error 错误状态色。
 * @member row_height 行高。
 * @member panel_width 面板宽度。
 * @member menu_height 菜单栏高度。
 * @member status_height 状态栏高度。
 * @member tool_rail_width 工具栏宽度。
 * @member padding 内边距。
 * @member margin 外边距。
 * @member gap 间距。
 * @member border_radius 圆角半径。
 * @member enable_transitions 是否启用过渡动画。
 * @member transition_duration 过渡动画时长。
 */
typedef struct UiThemeTokens {
    /* Primary */
    struct nk_color primary;
    struct nk_color primary_hover;
    struct nk_color primary_active;

    /* Background */
    struct nk_color background;
    struct nk_color panel;
    struct nk_color panel_hover;
    struct nk_color canvas_background;

    /* Text */
    struct nk_color text;
    struct nk_color text_secondary;
    struct nk_color text_disabled;

    /* Border */
    struct nk_color border;
    struct nk_color border_hover;

    /* Status */
    struct nk_color success;
    struct nk_color warning;
    struct nk_color error;

    /* Size tokens */
    float row_height;
    float panel_width;
    float menu_height;
    float status_height;
    float tool_rail_width;

    /* Spacing tokens */
    float padding;
    float margin;
    float gap;

    /* Corners */
    float border_radius;

    /* Transitions */
    nk_bool enable_transitions;
    float transition_duration;
} UiThemeTokens;

/**
 * @struct UiThemeDescriptor
 * @brief 主题元信息。
 *
 * @member id 主题唯一 ID。
 * @member label 主题显示名称。
 */
typedef struct UiThemeDescriptor {
    const char* id;
    const char* label;
} UiThemeDescriptor;

/**
 * @brief 获取默认内置主题令牌。
 * @return 默认主题令牌。
 */
UiThemeTokens ui_theme_default_tokens(void);

/**
 * @brief 根据主题 ID 获取主题令牌。
 * @param theme_id 主题 ID。
 * @return 匹配主题令牌；找不到时返回默认主题。
 */
UiThemeTokens ui_theme_tokens_for_id(const char* theme_id);

/**
 * @brief 获取可用主题数量（内置 + 外部）。
 * @return 主题总数。
 */
int ui_theme_count(void);

/**
 * @brief 通过索引获取主题描述符。
 * @param index 主题索引。
 * @return 索引有效时返回描述符指针，否则返回 `NULL`。
 */
const UiThemeDescriptor* ui_theme_descriptor_at(int index);

/**
 * @brief 根据主题 ID 查询索引。
 * @param theme_id 主题 ID。
 * @return 找到返回索引，未找到返回 `-1`。
 */
int ui_theme_index_of_id(const char* theme_id);

/**
 * @brief 获取默认主题 ID。
 * @return 默认主题 ID 字符串。
 */
const char* ui_theme_default_id(void);

/**
 * @brief 重新加载指定目录下的外部主题。
 * @param directory_path 主题目录路径。
 * @return 成功加载的外部主题数量。
 */
int ui_theme_reload_external(const char* directory_path);

/**
 * @brief 计算外部主题目录签名。
 * @param directory_path 主题目录路径。
 * @return 目录签名值（用于变更检测）。
 */
unsigned long long ui_theme_external_signature(const char* directory_path);

/**
 * @brief 获取最近一次主题重载错误信息。
 * @return 错误字符串；无错误时返回空字符串。
 */
const char* ui_theme_last_reload_error(void);

/**
 * @brief 从设置文件加载当前主题 ID。
 * @param path 设置文件路径。
 * @param out_theme_id 主题 ID 输出缓冲区。
 * @param out_theme_id_size 输出缓冲区大小。
 * @return 成功返回非零，否则返回 0。
 */
int ui_theme_load_selected_id(const char* path, char* out_theme_id, size_t out_theme_id_size);

/**
 * @brief 将当前主题 ID 保存到设置文件。
 * @param path 设置文件路径。
 * @param theme_id 主题 ID。
 * @return 成功返回非零，否则返回 0。
 */
int ui_theme_save_selected_id(const char* path, const char* theme_id);

/**
 * @brief 将主题令牌应用到 Nuklear 样式表。
 * @param ctx Nuklear 上下文。
 * @param tokens 主题令牌。
 * @return 无。
 */
void ui_theme_apply(struct nk_context* ctx, const UiThemeTokens* tokens);

#endif /* GLDRAW_UI_UI_THEME_H */
