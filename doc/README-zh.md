GLDraw
======================================

一个以画布为中心的 OpenGL 绘图编辑器，使用 C11 编写。

构建与运行
-----------

Linux / macOS：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/GLDraw
```

Windows（MinGW / MSYS2）：

```sh
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
./build-mingw/bin/GLDraw.exe
```

Windows（Visual Studio 2022）：

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/bin/Release/GLDraw.exe
```

或使用通用构建脚本：

```sh
./build.sh          # Release 构建
./build.sh debug    # Debug 构建
./build.sh clean    # 清理构建产物
```

快捷键
--------

| 快捷键 | 功能 |
|--------|------|
| V | 选择工具 |
| H | 手型平移工具 |
| L | 线段工具 |
| R | 矩形工具 |
| E | 椭圆工具 |
| Shift+点击 | 切换多选 |
| Ctrl+Z | 撤销 |
| Ctrl+Y / Ctrl+Shift+Z | 重做 |
| Ctrl+S | 保存文档 |
| Ctrl+O | 加载文档 |
| Delete / Backspace | 删除选择 |
| 鼠标滚轮 | 以光标为中心缩放 |
| Esc | 清除工具状态 |

文档
-------------

详细文档：[../doc/wiki/](../doc/wiki/)

许可证：MIT
