# GLDraw

一个以画布为中心的 OpenGL 绘图编辑器，使用 C11 编写。

## 快速开始

推荐优先使用项目自带构建脚本：

| 平台 | 推荐命令 | 模式 |
|---|---|---|
| Linux / macOS | `./build.sh` | `release`（默认）、`debug`、`clean` |
| Windows（MinGW/MSYS2） | `./build.bat` | `release`（默认）、`debug`、`clean` |

<details>
<summary>通过构建脚本在 Linux / macOS 上编译</summary>

```sh
./build.sh           # release
./build.sh debug     # debug
./build.sh clean     # clean
```

运行：

```sh
./build/Release/bin/GLDraw
```
</details>

<details>
<summary>通过构建脚本在 Windows（MinGW/MSYS2，CMD）上编译</summary>

```bat
build.bat            rem release
build.bat debug      rem debug
build.bat clean      rem clean
```

运行：

```bat
build\Release\bin\GLDraw.exe
```
</details>

<details>
<summary>通过手动 CMake 在 Linux / macOS 上编译</summary>

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

运行：

```sh
./build/bin/GLDraw
```
</details>

<details>
<summary>通过手动 CMake 在 Windows（Visual Studio 2022 x64）上编译</summary>

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

运行：

```bat
build\bin\Release\GLDraw.exe
```
</details>

## 构建要求

GLDraw 基于 CMake、C11、OpenGL、GLFW、GLAD 和 Nuklear。

| 平台 | 环境 |
|---|---|
| Linux / macOS | CMake + C11 编译器 + OpenGL 开发环境 |
| Windows（MinGW/MSYS2） | CMake + MinGW-w64 + OpenGL 开发环境 |

<details>
<summary>Linux / macOS</summary>

- CMake 3.15+
- C11 编译器（`gcc` 或 `clang`）
- OpenGL 开发环境（头文件与运行库）
- Makefiles 构建工具（`make`）
- 首次配置需要 Git 与网络（用于拉取 GLFW 3.3.9）
- macOS：Xcode Command Line Tools
</details>

<details>
<summary>Windows（MinGW/MSYS2）</summary>

- CMake 3.15+
- MinGW-w64 C11 工具链（`gcc`）
- `mingw32-make` 在 `PATH` 中
- OpenGL 开发环境（头文件与运行库）
- 首次配置需要 Git 与网络（用于拉取 GLFW 3.3.9）
</details>

## 项目概览

- 以画布为核心的二维绘图流程，内置线段、矩形、椭圆工具
- 基于 GLFW、GLAD 与 Nuklear 的 OpenGL 渲染和界面栈
- 提供基于快照的撤销/重做，以及 JSON 文档读写
- 以 `Workspace`、`Document`、`CanvasView` 为核心组织交互与状态

## 核心快捷键

`V` 选择，`H` 平移，`L` 线段，`R` 矩形，`E` 椭圆  
`Ctrl+Z` 撤销，`Ctrl+Y`/`Ctrl+Shift+Z` 重做  
`Ctrl+S` 保存，`Ctrl+O` 打开

## 文档

- 入门说明：[wiki/getting-started.md](wiki/getting-started.md)
- Wiki 首页：[wiki](wiki/)
- 架构说明：[wiki/architecture.md](wiki/architecture.md)
- 扩展指南：[wiki/extending.md](wiki/extending.md)

## 许可证

MIT
