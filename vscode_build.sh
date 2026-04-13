#!/bin/bash

ARG=${1:-""}

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    GENERATOR="Unix Makefiles"
    EXE_EXT=""
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    GENERATOR="Unix Makefiles"
    EXE_EXT=""
else
    # Windows with MSYS/MinGW
    GENERATOR="MinGW Makefiles"
    EXE_EXT=".exe"
fi

case "$ARG" in
    clean)
        rm -rf build
        echo "Build folder cleaned."
        ;;
    debug)
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Debug -S . -B build/Debug
        cmake --build build/Debug --parallel
        echo ""
        echo "============================================================"
        echo "  Build complete!"
        echo "  Run: ./build/Debug/bin/GLDraw$EXE_EXT"
        echo "============================================================"
        ;;
    release)
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -S . -B build/Release
        cmake --build build/Release --parallel
        echo ""
        echo "============================================================"
        echo "  Build complete!"
        echo "  Run: ./build/Release/bin/GLDraw$EXE_EXT"
        echo "============================================================"
        ;;
    *)
        # Default: build Release
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -S . -B build/Release
        cmake --build build/Release --parallel
        echo ""
        echo "============================================================"
        echo "  Build complete!"
        echo "  Run: ./build/Release/bin/GLDraw$EXE_EXT"
        echo "============================================================"
        ;;
esac
