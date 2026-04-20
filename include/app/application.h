/**
 * @file application.h
 * @brief 应用层入口接口。
 */
#ifndef GLDRAW_APP_APPLICATION_H
#define GLDRAW_APP_APPLICATION_H

/**
 * @brief 启动并运行 GLDraw 主循环。
 *
 * 该函数负责初始化窗口、渲染系统、UI 系统与工作区，
 * 在退出时完成资源释放与收尾。
 *
 * @return 正常退出返回 `0`；初始化失败或运行期不可恢复错误返回 `-1`。
 */
int app_run(void);

#endif /* GLDRAW_APP_APPLICATION_H */
