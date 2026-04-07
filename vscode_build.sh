#!/bin/bash

BUILD_TYPE="Release"
ARG=${1:-""}

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]] || [[ "$OSTYPE" == "linux-gnu"* ]]; then
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
    configure)
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -S . -B build
        ;;
    debug)
        BUILD_TYPE="Debug"
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -S . -B build
        cmake --build build --parallel
        ;;
    *)
        # Default: clean, configure and build
        rm -rf build
        cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -S . -B build
        cmake --build build --parallel
        echo ""
        echo "============================================================"
        echo "  Build complete!"
        echo "  Run: ./build/bin/GLDraw$EXE_EXT"
        echo "============================================================"
        ;;
esac
