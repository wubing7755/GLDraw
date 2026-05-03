# GLDraw C 语言贡献者指南（开发者 / 开源协作者）

本指南面向准备阅读、修改、扩展 GLDraw 的开发者与开源贡献者。  
目标不是覆盖全部 C 语法，而是帮助你快速掌握“这个仓库真正会用到的语法和实践”。

语言版本：
- 中文（当前文件）
- English: [c-language-must-know-for-gldraw.en.md](./c-language-must-know-for-gldraw.en.md)

## 适用读者

- 想给 GLDraw 提交 PR 的新贡献者
- 能读基础 C 代码，但对工程化写法不熟
- 需要快速理解本项目的模块边界与改动入口

## 贡献前建议先读

1. `doc/wiki/architecture.md`
2. `doc/wiki/data-flow.md`
3. `src/app/application.c`
4. `src/tools/tool_controller.c`
5. `src/document/object.c`

先建立主链路认知：`GLFW 回调 -> ToolController -> Document/CanvasView -> RenderSystem`。

## 必会 C 语法（按项目优先级）

下表中每一项都能在仓库中直接找到对应代码。

| 语法/概念 | 为什么必须会 | 典型文件 |
|---|---|---|
| `struct` + `typedef` | 组织应用状态和模块边界 | `include/app/workspace.h` |
| 指针 `*`、取址 `&`、成员访问 `.`/`->` | 贯穿所有模块调用 | `src/app/application.c` |
| 传值 vs 传指针 | 避免状态拷贝，支持原地修改 | `src/document/document.c` |
| `enum` + `switch` | 工具/对象状态分发 | `include/tools/tool.h`, `src/tools/tool_controller.c` |
| 定长数组与边界检查 | 对象池和选择集安全 | `include/document/document.h` |
| 函数指针与 vtable | 对象系统与工具系统多态 | `include/document/object.h`, `src/document/object.c` |
| 回调与 `void*` 用户指针 | 从 GLFW 窗口取回 `Application*` | `src/app/application.c` |
| 动态内存管理 | 避免泄漏与悬空引用 | `src/commands/command.c` |
| `const` / `static` / 作用域 | 控制可见性与只读约束 | 各模块 `.h/.c` |
| 预处理与平台分支 | Windows 与非 Windows 路径 | `src/document/persistence.c` |
| 位运算掩码 | 键盘修饰键处理 | `src/tools/tool_controller.c` |
| 字符串与安全格式化 | 路径、状态栏文本、日志 | `src/app/application.c` |

## 项目中的关键语法样式

### 1) 回调桥接（必须理解）

```c
glfwSetWindowUserPointer(app->window.handle, app);
Application* app = (Application*)glfwGetWindowUserPointer(handle);
```

意义：GLFW 回调只提供 `GLFWwindow*`，通过 user pointer 才能获取当前窗口对应的应用状态。

### 2) vtable 多态调用

```c
struct GraphicObjectVTable {
    void (*translate)(GraphicObject* object, Vec2 delta);
    int (*hit_test)(const GraphicObject* object, Vec2 point, float tolerance);
};
```

意义：新增对象类型时，不改大量 `if/else`，只需实现一组函数并挂到 vtable。

### 3) 统一空指针保护

```c
if (!app) {
    return;
}
```

意义：减少崩溃风险，是项目代码风格的一部分。

## 贡献者改动入口（按任务类型）

### 新增图元类型

1. 在 `include/document/object.h` 添加 `GraphicObjectType`
2. 在 `src/document/object.c` 实现构造和 vtable 回调
3. 确保 hit test、path points、scalar get/set 都完整
4. 如需要，补充工具创建入口与 UI 编辑项

### 新增工具

1. 在 `include/tools/tool.h` 增加 `ToolKind`
2. 在 `src/tools/tool_controller.c` 增加状态结构与回调实现
3. 在 `tool_controller_init()` 注册工具
4. 补充工具栏按钮和快捷键映射

### 修改属性面板行为

1. 改 `src/ui/ui_system.c`
2. 修改对象属性时保证通过 `command_executor_execute()` 提交命令
3. 检查 `workspace_sync_document_dirty()` 是否同步

## 提交 PR 前检查（推荐）

1. 能成功构建并运行应用
2. 功能路径至少手动测试一次（创建、选择、移动、撤销/重做）
3. 不引入明显内存泄漏（分配和释放配对）
4. 不破坏数组边界与空指针防护
5. 文档或注释与代码改动保持一致

## 新贡献者最常见问题

### 问题 1：把 `.` 和 `->` 混用

- 结构体变量用 `.`
- 指针用 `->`

### 问题 2：改了对象数据但没进历史栈

症状：修改后无法 `Ctrl+Z`。  
排查：是否通过 `command_executor_execute()` 正确提交了命令。

### 问题 3：只改了渲染，没改数据模型

症状：画面能看见，但保存/加载后丢失。  
排查：是否同步更新了 `Document` 与持久化逻辑。

### 问题 4：忘记处理平台分支

症状：Windows 通过、Linux/macOS 失败（或相反）。  
排查：`#ifdef _WIN32` 分支是否完整。

## 建议的学习与贡献路径

1. 先做只读导览：主循环、事件路由、渲染调用链
2. 再做小改动：新增快捷键、状态栏文本、工具切换逻辑
3. 然后做中等改动：新增简单对象属性或工具行为
4. 最后再做结构性改造：新图元类型、历史系统增强、持久化格式扩展

## 最低能力门槛（可独立贡献）

满足以下条件即可开始提交中小型 PR：

1. 能看懂 `Application*` 在回调链路中的流转
2. 能在不破坏边界检查的前提下修改 `Document` 数据
3. 能为对象或工具补齐 vtable/回调实现
4. 能定位并修复一个内存管理问题
5. 能完成一次从代码修改到本地运行验证的闭环
