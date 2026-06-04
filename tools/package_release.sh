#!/usr/bin/env bash
set -euo pipefail

VERSION=""
GENERATOR="Unix Makefiles"
BUILD_DIR="build/Release"
DIST_DIR="dist"
PLATFORM=""
SKIP_BUILD=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            VERSION="${2:-}"
            shift 2
            ;;
        --generator)
            GENERATOR="${2:-}"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --dist-dir)
            DIST_DIR="${2:-}"
            shift 2
            ;;
        --platform)
            PLATFORM="${2:-}"
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

if [[ -z "$VERSION" ]]; then
    VERSION="$(sed -nE '/project[[:space:]]*\([[:space:]]*GLDraw/,/\)/s/.*VERSION[[:space:]]+([0-9A-Za-z._-]+).*/\1/p' CMakeLists.txt | head -n 1)"
fi

if [[ -z "$VERSION" ]]; then
    echo "Could not find project version in CMakeLists.txt." >&2
    exit 1
fi

if [[ -z "$PLATFORM" ]]; then
    case "$(uname -s)" in
        Linux*) PLATFORM="linux-x64" ;;
        Darwin*) PLATFORM="macos" ;;
        *) PLATFORM="unknown" ;;
    esac
fi

mkdir -p "$(dirname "$BUILD_DIR")"
BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
mkdir -p "$DIST_DIR"
DIST_DIR="$(cd "$DIST_DIR" && pwd)"

STAGE_ROOT="$DIST_DIR/stage"
PACKAGE_NAME="GLDraw-v$VERSION-$PLATFORM"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
ARCHIVE_PATH="$DIST_DIR/$PACKAGE_NAME.tar.gz"

cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -S "$REPO_ROOT" -B "$BUILD_DIR"

if [[ "$SKIP_BUILD" -eq 0 ]]; then
    cmake --build "$BUILD_DIR" --parallel
fi

rm -rf "$STAGE_ROOT"
mkdir -p "$PACKAGE_ROOT"

cmake --install "$BUILD_DIR" --prefix "$PACKAGE_ROOT"

for file in LICENSE.txt README.md README.zh-CN.md doc/build.md doc/controls.md; do
    if [[ -f "$file" ]]; then
        cp "$file" "$PACKAGE_ROOT/"
    fi
done

tar -czf "$ARCHIVE_PATH" -C "$STAGE_ROOT" "$PACKAGE_NAME"

echo ""
echo "Release package created:"
echo "  $ARCHIVE_PATH"
