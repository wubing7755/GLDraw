# Getting Started

## Prerequisites

- **C Compiler** - MSVC, GCC, or Clang with C11 support
- **CMake** 3.15 or higher
- **OpenGL 3.3** capable graphics driver

## First Shape in 5 Minutes

1. **Clone and build**:
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

2. **Run** (from `build/bin/`):
   ```bash
   ./GLDraw      # Linux/macOS
   .\bin\Release\GLDraw.exe   # Windows
   ```

3. **Draw your first shape**:
   - Press `1` for LINE tool (default)
   - Click and drag on the canvas to draw a line
   - Release to finalize

4. **Edit properties**:
   - Press `S` to switch to SELECT tool
   - Click a shape to select it
   - Use the right panel to change color and line width

5. **Try more shapes**:
   - Press `2` for CIRCLE tool
   - Press `3` for RECT tool

## Dependencies

External dependencies are automatically downloaded by CMake:

| Dependency | Version | Purpose |
|------------|---------|---------|
| GLFW | 3.3.9 | Window and input management |
| GLAD | 2.x | OpenGL function loader |

Nuklear GUI is included as a header-only library in `include/nuklear/`.

## Build Instructions

### Windows (Visual Studio)

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022"

# Build
cmake --build . --config Release

# Run
.\bin\Release\GLDraw.exe
```

### Linux / macOS

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# Run
./bin/GLDraw
```

## Running the Application

After building, the executable is located at:
- Windows: `build/bin/Release/GLDraw.exe`
- Linux/macOS: `build/bin/GLDraw`

The working directory for running should be `build/bin/` (required for shader file loading).

## Project Layout

```
GLDraw/
├── include/          # Public headers
├── src/              # Source files
├── shaders/          # GLSL shaders
├── doc/              # Documentation
└── build/            # Build output
```

## Next Steps

- Read [Project Structure](project-structure.md) to understand the codebase
- Read [Architecture Overview](architecture.md) for high-level design
- Read [Core Systems](core-systems/README.md) for detailed subsystem documentation
