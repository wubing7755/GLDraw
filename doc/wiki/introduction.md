# Introduction

## Project Overview

**GLDraw** is a minimal, teaching-friendly OpenGL 3.3 example application. It demonstrates core graphics programming concepts including VAO/VBO rendering, shader programming, immediate-mode GUI integration, and event-driven tool systems.

The application is a 2D shape drawing editor supporting multiple shape types (Line, Circle, Rectangle) with selection, movement, and property editing capabilities.

## Key Features

- **OpenGL 3.3 Core Profile** - Modern VAO/VBO rendering pipeline
- **Multiple Shape Types** - Line, Circle, Rectangle with polymorphic vtable architecture
- **Tool-Based Input System** - Extensible tool pattern for draw and select operations
- **Nuklear Immediate-Mode GUI** - Property panel for color and line width editing
- **Shape Registry Pattern** - Easy addition of new shape types without modifying core systems
- **Cross-Platform** - Windows, Linux, macOS support via GLFW
- **C11 Standard** - Modern C with clear module boundaries

## Target Users

- Developers learning OpenGL graphics programming
- Students studying game development or computer graphics
- Contributors looking to understand the codebase before extending it

## Technology Stack

| Component | Technology |
|-----------|------------|
| Graphics API | OpenGL 3.3 Core Profile |
| Window Management | GLFW 3.3.9 |
| OpenGL Loader | GLAD 2.x |
| GUI Framework | Nuklear (header-only) |
| Build System | CMake 3.15+ |
| Language | C11 |

## License

MIT License - see [LICENSE.txt](../../LICENSE.txt)
