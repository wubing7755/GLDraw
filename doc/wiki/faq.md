# FAQ & Tips

## General

### What is GLDraw?

GLDraw is a minimal OpenGL 3.3 shape drawing application designed as a teaching example for graphics programming concepts.

### What technologies does it use?

- OpenGL 3.3 Core Profile
- GLFW 3.3.9 for window management
- Nuklear for immediate-mode GUI
- C11 standard

### Can I use this as a base for my project?

Yes, GLDraw is MIT licensed. It's designed to be a clean starting point for OpenGL learning.

## Building

### CMake can't find GLFW

CMake should download GLFW automatically via FetchContent. If it fails, check your internet connection and CMake version (3.15+ required).

### Shaders fail to load

The working directory must be `build/bin/` when running. Shader paths are relative to the current working directory.

### OpenGL version not supported

Ensure your graphics driver supports OpenGL 3.3. Update your graphics drivers.

## Development

### How do I add a new shape?

See [Extending the Project](extending) for a step-by-step tutorial.

### How do I change the window size?

Modify `DEFAULT_WIDTH` and `DEFAULT_HEIGHT` in `src/window.c`, or add command-line argument parsing.

### How does the vtable pattern work?

Shapes and Tools use function pointers for polymorphism. Each concrete type (Line, Circle, Rect) implements the same interface. See [Shape System](core-systems/shape-system) for details.

### Why use a registry pattern?

The registry allows adding new shape types without modifying `ShapeManager` or `Renderer`. See [Shape System](core-systems/shape-system).

## Troubleshooting

### Shapes not appearing

1. Check if shapes were actually added (try drawing with mouse)
2. Verify shader compiled successfully (check console output)
3. Ensure vertex buffer is being uploaded

### Selection not working

1. Click directly on a shape
2. Check that SelectTool is active (press S)
3. Verify SelectionManager is initialized

### UI blocking mouse input

Click outside the property panel area (x=580 to x=800, y=50 to y=450).

### Colors look wrong

Verify alpha blending is enabled in renderer. Check shape color values are in [0, 1] range.

## Tips

1. **Start with small changes** - Modify existing code before adding new features
2. **Use LOG_DEBUG macros** - Enable debug logging in `macros.h` to trace execution
3. **Read shader.c first** - Understanding the shader pipeline helps debugging
4. **Use Visual Studio Code** - The repo includes IntelliSense configuration in `.vscode/`
5. **Test on multiple platforms** - GLFW makes cross-platform testing straightforward
