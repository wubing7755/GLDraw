GLDraw
======================================

[![C Version](https://img.shields.io/badge/C-C11-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![EN](https://img.shields.io/badge/English-blue)](../README.md)



目录：

     I.   概述
     II.  快速开始
    III. 项目结构
     IV.  构建方法
      V.  调试
     VI.  技术概述
     VII. 依赖库



I. 概述

GLDraw 是一个基于 OpenGL 3.3 的最小化示例，使用 GLFW 和 Nuklear GUI 构建。

核心特点：
- 代码模块化，职责分离清晰
- OpenGL 3.3 Core Profile
- 跨平台支持（Windows、Linux、macOS）
- Nuklear 即时模式 GUI 用于运行时控制
- 基于 CMake 的依赖管理



II. 快速开始

     $ git clone https://github.com/wubing7755/GLDraw.git
     $ cd GLDraw
     $ mkdir build && cd build
     $ cmake -G "MinGW Makefiles" ..
     $ cmake --build . --parallel

     # 运行
     $ ./bin/GLDraw



III. 项目结构

     GLDraw/
     ├── CMakeLists.txt            # 构建配置
     ├── LICENSE.txt               # MIT 许可证
     ├── README.md                 # 英文说明
     ├── include/                  # 头文件目录
     │   ├── core/                 # 项目头文件
     │   └── nuklear/              # Nuklear GUI 库（header-only）
     ├── src/                      # 源代码
     │   ├── main.c
     │   ├── ...
     │   ├── window.c
     │   └── nuklear_ui.c
     └── shaders/                  # GLSL 着色器
         ├── basic.vert
         └── basic.frag



IV. 构建方法

**前置要求：** CMake 3.15+、C11 编译器、Python 3（用于 GLAD 生成）

Windows (MinGW/MSYS2)：

     $ cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B build
     $ cmake --build build --parallel

Windows (Visual Studio)：

     $ cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
     $ cmake --build build --config Release

Linux / macOS：

     $ cmake -S . -B build
     $ cmake --build build --parallel

VSCode 用户：

     按 F5 调试，或 Ctrl+Shift+P -> 运行任务 -> CMake: Build



V. 调试

VSCode 调试配置（launch.json）：

     - GLDraw (Debug)     : 调试模式，可设断点
     - GLDraw (No Debug)  : 发布模式，直接运行



VI. 技术概述

C 语言：
     - 跨平台条件编译
     - 模块化架构（window / renderer / shader / input / ui）

OpenGL：
     - OpenGL 3.3 Core Profile
     - VAO / VBO / EBO 几何数据
     - Uniform 运行时参数控制
     - 事件驱动渲染循环



VII. 依赖库

| 依赖库 | 版本 | 来源 | 管理方式 |
|---|---|---|---|
| GLFW | 3.3.9 | github.com/glfw/glfw | CMake FetchContent |
| GLAD | 2.x (生成) | github.com/Dav1dde/glad | CMake 配置阶段 Python 生成 |
| Nuklear | master | github.com/Immediate-Mode-UI/Nuklear | 本地保留（header-only） |

所有第三方依赖均在 CMake 配置阶段自动获取或生成。

     如有问题或建议，请在仓库中提交 Issue。

     欢迎贡献。详见源代码了解实现细节。
