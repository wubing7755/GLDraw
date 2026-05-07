# GLDraw C 贡献者指南

> Audience: 会读基础 C、准备修改 GLDraw 的开发者
> Purpose: 快速建立项目级心智模型，而不是复习通用 C 语法
> Source of truth: 当前源码目录与 `architecture/` 文档
> Last reviewed with code: 2026-05-07
> Related: [overview.md](overview.md), [../architecture/extension-model.md](../architecture/extension-model.md)

## 这份文档解决什么问题

这不是一份完整的 C 语言教程。

它只回答三件事：

1. 在 GLDraw 里，核心抽象分别是什么。
2. 不同类型的改动应该从哪里下手。
3. 怎样避免改完能编译、但架构语义已经跑偏。

## 先建立主链路认知

建议先读这些文件和文档：

1. [../architecture/overview.md](../architecture/overview.md)
2. [../architecture/data-flow.md](../architecture/data-flow.md)
3. [../reference/file-map.md](../reference/file-map.md)
4. `src/app/application.c`
5. `src/app/command_dispatcher.c`
6. `src/tools/tool_runtime.c`
7. `src/commands/command_executor.c`

先把下面这条主链路看懂，再做实现改动：

`GLFW callbacks -> application runtime helpers -> tool/input dispatch -> command execution or canvas update -> render + ui`

## 你必须理解的项目抽象

### `Workspace`

`Workspace` 是运行时总容器，不是“随便塞状态”的地方。

- `EditorCore` 持有 `Document`、`CommandExecutor`、`CanvasView`、`ToolController`
- `EditorSession` 持有选择集、剪贴板、布局、脏状态、当前路径、状态栏消息
- `EditorServices` 持有保存、加载、导出等应用层回调

如果一个状态需要持久化到文档里，它通常不该只存在 `EditorSession`。

### `CommandExecutor`

所有耐久的文档修改路径都应该优先走命令系统。

要点：

- 负责 `undo/redo`
- 支持 merge
- 支持 transaction
- 管理历史内存预算

如果你直接改 `Document`，通常会带来以下问题：

- 无法撤销
- 脏状态不同步
- 选择预览和最终提交行为不一致

### `GraphicObjectDescriptor`

对象系统的扩展入口不是旧式的大型 `switch`，而是描述符注册。

当前模式是：

- 每种对象类型实现一组回调
- 通过 `register_object_type()` 注册
- 通过 `src/app/extension_manifest.c` 组装内置对象

先看这些文件：

- `include/document/object.h`
- `src/document/object_registry.c`
- `src/document/object_runtime.c`
- `src/document/object_line.c`
- `src/document/object_rect.c`
- `src/document/object_ellipse.c`
- `src/document/object_fake_star.c`

### `ToolDescriptor`

工具系统也已经切到描述符模式。

当前模式是：

- 每个工具提供创建、激活、指针处理、键盘处理、overlay 等回调
- 通过 `register_tool()` 或 `register_shape_tool()` 注册
- 通过 `src/app/tool_manifest.c` 组装内置工具

先看这些文件：

- `include/tools/tool.h`
- `src/tools/tool_registry.c`
- `src/tools/tool_runtime.c`
- `src/tools/tool_select.c`
- `src/tools/tool_pan.c`
- `src/tools/tool_shape.c`

### `CanvasView`

`CanvasView` 只负责空间上下文，不负责持久化编辑语义。

它主要解决：

- world/screen 坐标变换
- viewport
- zoom / pan
- picking 相关空间辅助

当你看到“坐标不对”“缩放中心不对”“拖动位移不对”，优先检查 `canvas/`。

## 常见改动从哪里开始

### 新增对象类型

推荐入口：

1. 参考 `src/document/object_line.c` 或 `src/document/object_fake_star.c`
2. 在新文件里实现对象 payload 和描述符回调
3. 用 `register_object_type()` 注册
4. 在 `src/app/extension_manifest.c` 接入 manifest
5. 如果对象需要新工具，再补工具注册
6. 补序列化、hit test、property schema、测试

不要再按“在一个公共 `object.c` 里加分支”的旧思路扩展。

### 新增工具

推荐入口：

1. 判断是否可以复用 `register_shape_tool()`
2. 如果不行，参考 `src/tools/tool_select.c` 或 `src/tools/tool_pan.c`
3. 实现 `ToolDescriptor`
4. 在 `src/app/tool_manifest.c` 注册
5. 检查快捷键、命令 ID、tooltip、icon 文案
6. 验证工具切换、pointer capture、overlay 和提交命令路径

### 修改属性面板或菜单行为

优先看：

- `src/ui/ui_inspector_panel.c`
- `src/ui/ui_chrome.c`
- `src/ui/ui_menu_def.c`
- `src/app/command_dispatcher.c`
- `src/app/command_registry.c`

规则：

- UI 更适合发出 action 或命令请求
- 业务语义尽量留在 workspace / command / tool 层
- 用户可见的耐久改动不要停留在 UI 临时状态里

### 修改快捷键

优先看：

- `src/input/keymap.c`
- `src/ui/ui_menu_def.c`
- `src/tools/tool_*.c` 中的 `default_shortcut`
- [../user/controls.md](../user/controls.md)

## 最常见的误区

### 误区 1：把“预览状态”当成“已提交状态”

例如选择拖动时，拖动过程中使用的是 session 预览偏移，真正落地是在 pointer up 时提交 move command。

如果你在 move 过程中直接写 `Document`，撤销和交互手感都会出问题。

### 误区 2：只改渲染，不改模型

症状：

- 画面看起来对
- 保存后丢失
- hit test 或选择行为不一致

这种问题通常说明你只改了 render path，没有改 `Document`、对象描述符或持久化逻辑。

### 误区 3：只改模型，不改扩展入口

新增对象或工具后，如果忘记接 manifest，编译可能通过，但运行时根本不可达。

### 误区 4：把历史文档当现状

仓库里保留了少量兼容跳转页和历史重构记录。

看实现时请以这些页面为准：

- `doc/architecture/overview.md`
- `doc/architecture/extension-model.md`
- `doc/reference/file-map.md`

## 提交前最低检查

1. 能本地构建。
2. 你的主用户路径至少手动走一遍。
3. 如果改了对象或工具，检查注册是否生效。
4. 如果改了文档结构、快捷键、扩展入口，同步更新 `doc/`。
5. 如果改了命令路径，至少确认 undo/redo 没断。
