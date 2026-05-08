# GLDraw

[English](README.md) | 简体中文

一个使用 C11 编写、以画布为中心的 OpenGL 绘图编辑器。

![Language](https://img.shields.io/badge/language-C11-00599C)
![Build](https://img.shields.io/badge/build-CMake-064F8C)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%203.3-5586A4)
![UI](https://img.shields.io/badge/UI-Nuklear-222222)
![License](https://img.shields.io/badge/license-MIT-green)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=for-the-badge&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/wubing7755/GLDraw)

![GLDraw 预览图](assets/preview.png)

动画占位图：
![GLDraw 演示占位图](assets/demo.gif)

## 项目状态

持续开发中。

当前重点包括：

- 编辑工作流细化
- 扩展系统继续演进
- 渲染与交互体验打磨
- 命令系统与 workspace 行为的测试覆盖增强

## 项目亮点

- 以画布为核心的二维编辑工作流，文档几何数据以 world space 存储
- 基于 GLFW、GLAD、Nuklear 和 OpenGL 3.3 Core Profile 的 C11 工程
- 基于命令系统的撤销/重做，支持 merge 和 transaction
- 基于 descriptor 的对象与工具扩展模型
- 支持 JSON 文档持久化与 PNG 导出
- 支持图层可见性、锁定、重命名、重排等编辑流

## 快速开始

### Linux / macOS

```sh
./build.sh
./build/Release/bin/GLDraw
```

### Windows（MinGW/MSYS2，CMD）

```bat
build.bat
build\Release\bin\GLDraw.exe
```

更多构建方式和平台说明：
[doc/user/getting-started.md](doc/user/getting-started.md)

## 当前功能

- 线段、矩形、椭圆工具
- 选择、移动、平移、缩放、缩放适配
- 基于 Inspector 的属性编辑
- 图层创建、激活、显隐、锁定、重命名和重排
- 基于 JSON 的保存与加载
- 通过 `CommandExecutor` 实现撤销/重做
- 从当前渲染路径导出 PNG

## 快捷键

`V` 选择，`H` 平移，`L` 线段，`R` 矩形，`E` 椭圆  
`Ctrl+Z` 撤销，`Ctrl+Y` / `Ctrl+Shift+Z` 重做  
`Ctrl+S` 保存，`Ctrl+O` 打开

完整快捷键说明：
[doc/user/controls.md](doc/user/controls.md)

## 架构概览

GLDraw 以 `Workspace` 为运行时中心：

```text
Workspace
  -> EditorCore
  -> EditorSession
  -> EditorServices
```

几个关键点：

- `EditorCore` 持有 `Document`、`CommandExecutor`、`CanvasView`、`ToolController`
- 耐久编辑路径通过 command 流转，而不是零散直接修改文档
- 对象和工具通过 manifest 与 descriptor 元数据注册
- UI 读取 workspace/view model 并发出 action，不作为编辑真相来源

更多说明：

- [Architecture Overview](doc/architecture/overview.md)
- [Core Systems](doc/architecture/core-systems.md)
- [Data Flow](doc/architecture/data-flow.md)
- [Extension Model](doc/architecture/extension-model.md)

## 文档入口

- [Documentation Index](doc/README.md)
- [Getting Started](doc/user/getting-started.md)
- [Controls](doc/user/controls.md)
- [Contributing Overview](doc/contributing/overview.md)
- [GitHub Collaboration Guidelines](doc/contributing/github-collaboration-guidelines.md)
- [GitHub Templates](doc/contributing/github-templates.md)

## 参与贡献

请先看：

- [Contributing Overview](doc/contributing/overview.md)
- [C 贡献者指南](doc/contributing/c-contributor-guide.zh.md)
- [GitHub Templates](doc/contributing/github-templates.md)

## 许可证

MIT
