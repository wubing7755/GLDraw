# GLDraw

一个以画布为中心的 OpenGL 绘图编辑器，使用 C11 编写。

## 快速开始

请选择平台：

| 平台 | 脚本 | 模式 |
|---|---|---|
| Linux / macOS | `./build.sh` | `release`（默认）、`debug`、`clean` |
| Windows（MinGW/MSYS2） | `./build.bat` | `release`（默认）、`debug`、`clean` |

<details>
<summary>Linux / macOS</summary>

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
<summary>Windows（MinGW/MSYS2，CMD）</summary>

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
<summary>Windows（Visual Studio 2022 x64，手动 CMake）</summary>

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

运行：

```bat
build\bin\Release\GLDraw.exe
```
</details>

## 编译环境

请选择平台：

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

## 核心快捷键

`V` 选择，`H` 平移，`L` 线段，`R` 矩形，`E` 椭圆  
`Ctrl+Z` 撤销，`Ctrl+Y`/`Ctrl+Shift+Z` 重做  
`Ctrl+S` 保存，`Ctrl+O` 打开

## 文档

- Wiki 首页：[wiki](wiki/)
- 架构说明：[wiki/architecture.md](wiki/architecture.md)
- 扩展指南：[wiki/extending.md](wiki/extending.md)

## 许可证

MIT
