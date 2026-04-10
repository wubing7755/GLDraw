# Core Systems Overview

The current editor is built from six primary systems:

## 1. Document

- stores all `GraphicObject` instances
- assigns stable `ObjectId` values
- manages the selection set

## 2. CanvasView

- stores zoom, pan center, and viewport
- converts between world coordinates and screen coordinates
- provides picking tolerance in world units

## 3. Tools

- receive normalized editor events
- read and modify the document through a `ToolContext`
- provide temporary overlay geometry while drawing

## 4. Rendering

- converts object paths into screen-space line strips
- draws the background grid and origin axes
- draws selection highlight before the object stroke

## 5. UI

- builds editor controls using Nuklear
- exposes tool switching and property editing
- blocks tool input while the pointer is over UI panels

## 6. Platform/Application

- owns the GLFW window
- handles resize, pointer, key, and wheel callbacks
- advances the frame loop
