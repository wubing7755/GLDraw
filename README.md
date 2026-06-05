# GLDraw

English | [简体中文](README.zh-CN.md)

GLDraw is a canvas-centered OpenGL drawing editor written in C11.

![Language](https://img.shields.io/badge/language-C11-00599C)
![Build](https://img.shields.io/badge/build-CMake-064F8C)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%203.3-5586A4)
![UI](https://img.shields.io/badge/UI-Nuklear-222222)
![License](https://img.shields.io/badge/license-MIT-green)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat-square&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/wubing7755/GLDraw)

![GLDraw preview](assets/preview.png)

Highlights
----------

* Draw lines, rectangles, and ellipses on a world-space canvas.
* Select, move, pan, zoom, and fit the canvas to the document.
* Edit object properties through the inspector.
* Manage layers with visibility, locking, renaming, and reordering.
* Save and load documents as JSON.
* Export the current render output as PNG.
* Use command-based undo and redo.


Quick Start
-----------

Linux and macOS:

```sh
./build.sh
./build/Release/bin/GLDraw
```

Windows with MinGW/MSYS2 from CMD:

```bat
build.bat
build\Release\bin\GLDraw.exe
```

More build options: [doc/build.md](doc/build.md)


Documentation
-------------

* Build and run: [doc/build.md](doc/build.md)
* Controls: [doc/controls.md](doc/controls.md)
* Contributing: [CONTRIBUTING.md](CONTRIBUTING.md)
* Testing: [doc/testing.md](doc/testing.md)
* Release process: [doc/release.md](doc/release.md)
* AI agent policy: [doc/ai-agents.md](doc/ai-agents.md)
* AI agent playbooks: [doc/agent-playbooks.md](doc/agent-playbooks.md)
* Changelog: [CHANGELOG.md](CHANGELOG.md)
* Security policy: [SECURITY.md](SECURITY.md)
* Local documentation index: [doc/README.md](doc/README.md)
* Architecture and source guide: https://zread.ai/wubing7755/GLDraw


License
-------

MIT. See [LICENSE.txt](LICENSE.txt).
