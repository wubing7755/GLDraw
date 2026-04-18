@echo off
setlocal

set ARG=%1

if "%ARG%"=="clean" (
    rmdir /s /q build 2>nul
    echo Build folder cleaned.
    exit /b 0
)

if "%ARG%"=="debug" (
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -S . -B build/Debug
    cmake --build build/Debug --parallel
    echo.
    echo ============================================================
    echo   Build complete!
    echo   Run: %CD%\build\Debug\bin\GLDraw.exe
    echo ============================================================
    exit /b 0
)

if "%ARG%"=="release" (
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B build/Release
    cmake --build build/Release --parallel
    echo.
    echo ============================================================
    echo   Build complete!
    echo   Run: %CD%\build\Release\bin\GLDraw.exe
    echo ============================================================
    exit /b 0
)

rem Default: build Release
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B build/Release
cmake --build build/Release --parallel

echo.
echo ============================================================
echo   Build complete!
echo   Run: %CD%\build\Release\bin\GLDraw.exe
echo ============================================================
