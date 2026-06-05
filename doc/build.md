Build and Run GLDraw
====================

GLDraw is a C11 OpenGL drawing editor built with CMake.

The recommended path is to use the repository build scripts. Manual CMake
commands and optional CMake presets are available when a specific generator or
output layout is needed.


Requirements
------------

* CMake 3.15 or newer
* CMake 3.21 or newer for optional CMake presets
* A C11 compiler
* OpenGL runtime and development files
* Git and network access for the first CMake configure
* GLFW 3.3.9 is fetched by CMake
* GLAD is committed in the repository
* Nuklear is header-only and included in the source tree


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


Build Scripts
-------------

Linux, macOS, or MSYS-like shells:

```sh
./build.sh
./build.sh debug
./build.sh clean
```

Windows CMD with MinGW/MSYS2:

```bat
build.bat
build.bat debug
build.bat clean
```

Script output paths:

* Release: build/Release/bin/GLDraw or build\Release\bin\GLDraw.exe
* Debug: build/Debug/bin/GLDraw or build\Debug\bin\GLDraw.exe


CMake Presets
-------------

Contributors with CMake 3.21 or newer can use shared presets:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Common presets:

* debug
* release
* coverage
* asan
* mingw-debug
* mingw-release
* msvc-release

Presets are used by CI, but the build scripts remain the recommended simple
entry point for local users.


Manual CMake
------------

Linux and macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/GLDraw
```

Windows with MinGW Makefiles:

```bat
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw --parallel
build-mingw\bin\GLDraw.exe
```

Windows with Visual Studio 2022:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\bin\Release\GLDraw.exe
```


Testing
-------

CTest is used by the project test suite. It is currently expected in
Linux/macOS CI environments.

```sh
ctest --test-dir build/Release --output-on-failure
```

For manual CMake builds, replace `build/Release` with the build directory used
during configure.


Release Packaging
-----------------

On Windows with MinGW/MSYS2, create a distributable zip package with:

```bat
release.bat
```

After creating the Windows zip package, create a Windows installer when Inno
Setup 6 is installed:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package_windows_installer.ps1 -Version 0.0.3 -Platform windows-x64
```

On Linux, create a distributable tarball with:

```sh
bash ./release.sh
```

After creating the Linux tarball, create an AppImage:

```sh
bash ./tools/package_appimage.sh --version 0.0.3 --platform linux-x64
```

The package is written to `dist/` and contains the application binary, bundled
resources, README files, license text, and detected non-system runtime DLLs. The
installed layout used by current releases is:

```text
bin/GLDraw.exe
share/gldraw/shaders/
share/gldraw/themes/
share/gldraw/scripts/
```

Override the generated version or platform label when needed:

```bat
release.bat -Version 0.0.3 -Platform windows-x64
```

```sh
bash ./release.sh --version 0.0.3 --platform linux-x64
```

GitHub Actions also has a release workflow. Pushing a tag such as `v0.0.3`, or
running the workflow manually with version `0.0.3`, builds and uploads both
Windows and Linux packages to the matching GitHub Release:

```text
GLDraw-v0.0.3-windows-x64-setup.exe
GLDraw-v0.0.3-windows-x64.zip
GLDraw-v0.0.3-linux-x64.AppImage
GLDraw-v0.0.3-linux-x64.tar.gz
```

Use the Windows setup installer for normal installation, the Windows zip package
for portable use, the Linux AppImage for a desktop-style portable app, or the
Linux tarball for manual extraction.

For older tags that predate the current release scripts, run the `Legacy
Release` workflow with the desired version and target ref. It checks out the
target ref for building while using current packaging tools to upload zip,
setup, tarball, and AppImage assets to the matching GitHub Release.


CMake Project Layout
--------------------

`CMakeLists.txt` is the project entry point. Supporting CMake modules live in
`cmake/`:

* `CompilerOptions.cmake` - compiler warnings, sanitizers, source-specific
  warning suppressions, and target defaults.
* `Dependencies.cmake` - GLFW, OpenGL, and optional scripting dependencies.
* `Sources.cmake` - source lists grouped by target.
* `Tests.cmake` - test targets and CTest registration.
* `Packaging.cmake` - install rules used by packaging scripts.

Release notes should use this short format:

```text
GLDraw vX.Y.Z <one-sentence release summary>.

Highlights:
- ...

Downloads:
- GLDraw-vX.Y.Z-windows-x64-setup.exe
- GLDraw-vX.Y.Z-windows-x64.zip
- GLDraw-vX.Y.Z-linux-x64.AppImage
- GLDraw-vX.Y.Z-linux-x64.tar.gz

Target commit:
- <short-sha> <commit title>
```


Troubleshooting
---------------

* If the first configure fails while fetching GLFW, check Git and network
  access, then rerun CMake.
* If the app is not at the documented path, check whether the build used the
  script layout or a manual CMake layout.
* If Windows cannot find `mingw32-make`, verify that the MinGW/MSYS2 toolchain
  is installed and available on PATH.
