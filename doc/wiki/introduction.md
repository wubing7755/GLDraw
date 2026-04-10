# Introduction

## Overview

GLDraw is a small 2D editor used to demonstrate how a graphics application should be split into:

- document data
- canvas view state
- tool interaction
- rendering
- UI shell

It is no longer a single `window/input/renderer/shape` example. The current codebase is organized around a reusable editor foundation.

## Current Capabilities

- Infinite-style canvas with zoom and pan
- Line, rectangle, and ellipse objects
- Selection, shift-multi-select, move, delete
- Property editing through a right-side inspector
- Grid and origin axis rendering

## Technology Stack

| Component | Technology |
|---|---|
| Graphics API | OpenGL 3.3 Core Profile |
| Window / Input | GLFW 3.3.9 |
| OpenGL loader | GLAD |
| UI | Nuklear |
| Build system | CMake 3.15+ |
| Language | C11 |

## Design Goal

The project aims to mirror how mainstream canvas tools are structured:

- the window is a host
- the canvas is a view
- graphic objects live in document space
- tools translate pointer input into document changes

That makes later additions like undo/redo, layers, saving, and multiple views much easier.
