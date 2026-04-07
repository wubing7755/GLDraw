@echo off
setlocal

set BUILD_TYPE=Release
set ARG=%1

if "%ARG%"=="clean" (
    rmdir /s /q build 2>nul
    echo Build folder cleaned.
    exit /b 0
)

if "%ARG%"=="configure" (
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -S . -B build
    exit /b 0
)

if "%ARG%"=="debug" (
    set BUILD_TYPE=Debug
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -S . -B build
    cmake --build build --parallel
    goto :done
)

rem Default: clean, configure and build
rmdir /s /q build 2>nul
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -S . -B build
cmake --build build --parallel

:done

echo.
echo ============================================================
echo   Build complete!
echo   Run: %CD%\build\bin\GLDraw.exe
echo ============================================================
