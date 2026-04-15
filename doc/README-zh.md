GLDraw
======================================

[![C Version](https://img.shields.io/badge/C-C11-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![EN](https://img.shields.io/badge/English-blue)](../README.md)

GLDraw 已重构为一个“以画布为中心”的 OpenGL 绘图编辑器。现在的分层方式接近常见图形工具：

- `Window`：窗口外壳与平台事件
- `Document`：图形对象与选择集
- `CanvasView`：缩放、平移、视口、坐标变换
- `ToolController`：输入路由与工具切换
- `RenderSystem`：文档与工具叠加层渲染
- `UiSystem`：工具栏、属性面板、状态栏

## 当前功能

- 单窗口、单文档、单画布
- 支持光标位置缩放与画布平移
- 线段、矩形、椭圆三种绘制工具
- 选择、Shift 多选、拖拽移动、删除选择
- 已支持创建、移动、删除、属性编辑的撤销 / 重做
- 已支持 JSON 文档保存 / 加载，默认路径为 `document.json`
- 属性面板可编辑描边颜色、线宽和基础几何
- 网格与坐标轴渲染
- 基于 GLFW、GLAD、Nuklear 的模块化 C11 架构

## 快速开始

Windows + MinGW / MSYS2：

```sh
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
./build-mingw/bin/GLDraw.exe
```

Windows + Visual Studio 2022：

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/bin/Release/GLDraw.exe
```

## 快捷键

- `V`：选择工具
- `H`：手型平移工具
- `L`：线段工具
- `R`：矩形工具
- `E`：椭圆工具
- `Shift + 点击`：切换多选
- `Ctrl+Z`：撤销
- `Ctrl+Y` 或 `Ctrl+Shift+Z`：重做
- `Ctrl+S`：保存当前文档 JSON
- `Ctrl+O`：加载当前文档 JSON
- `Delete` / `Backspace`：删除当前选择
- `鼠标滚轮`：以光标为中心缩放
- `Esc`：清除当前工具状态；如果已在选择工具则关闭窗口

## 项目结构

```text
GLDraw/
├── include/
│   ├── app/        # 应用装配与工作区
│   ├── base/       # 基础类型、数学与日志
│   ├── canvas/     # 画布视图与坐标变换
│   ├── document/   # 图形对象、文档、选择
│   ├── platform/   # GLFW 窗口封装
│   ├── render/     # OpenGL 渲染
│   ├── tools/      # 工具协议与控制器
│   ├── ui/         # Nuklear UI
│   ├── glad/
│   ├── KHR/
│   └── nuklear/
├── src/
│   ├── app/
│   ├── canvas/
│   ├── document/
│   ├── platform/
│   ├── render/
│   ├── tools/
│   ├── ui/
│   ├── glad.c
│   └── main.c
└── shaders/
```

## 架构说明

当前主链路为：

`Window -> Workspace -> Document + CanvasView + ToolController + UiSystem + RenderSystem`

关键职责边界：

- 图形对象以世界坐标保存在 `Document`
- `Document` 当前以 JSON 持久化，并包含选择集
- `CanvasView` 负责世界坐标与屏幕坐标转换
- 工具只通过带画布上下文的事件修改文档
- 渲染器读取文档和画布状态，但不持有编辑状态

## 依赖

| 依赖 | 用途 | 管理方式 |
|---|---|---|
| GLFW 3.3.9 | 窗口与输入 | CMake 使用本地缓存源码 |
| GLAD | OpenGL 加载器 | 已提交到仓库 |
| Nuklear | 即时模式 UI | 本地 header-only |

## 文档入口

- Wiki 首页：[wiki/Home.md](./wiki/Home.md)
- C 贡献者指南（中文）：[c-language-must-know-for-gldraw.md](./c-language-must-know-for-gldraw.md)
- C Contributor Guide (English)：[c-language-must-know-for-gldraw.en.md](./c-language-must-know-for-gldraw.en.md)

## 当前阶段

这是当前重构基线版本。新架构已经落地，旧的耦合式运行时已经从构建中移除，并已经具备基础历史记录与 JSON 持久化能力。后续可以继续扩展：

- 图层与编组
- 吸附与辅助线
- 多画布或多文档
