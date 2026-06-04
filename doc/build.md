Build and Run GLDraw
====================

GLDraw is a C11 OpenGL drawing editor built with CMake.

The recommended path is to use the repository build scripts. Manual CMake
commands are available when a specific generator or output layout is needed.


Requirements
------------

* CMake 3.15 or newer
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


Troubleshooting
---------------

* If the first configure fails while fetching GLFW, check Git and network
  access, then rerun CMake.
* If the app is not at the documented path, check whether the build used the
  script layout or a manual CMake layout.
* If Windows cannot find `mingw32-make`, verify that the MinGW/MSYS2 toolchain
  is installed and available on PATH.
