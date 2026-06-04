#!/usr/bin/env bash
set -euo pipefail

VERSION=""
PLATFORM="linux-x64"
DIST_DIR="dist"
STAGE_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            VERSION="${2:-}"
            shift 2
            ;;
        --platform)
            PLATFORM="${2:-}"
            shift 2
            ;;
        --dist-dir)
            DIST_DIR="${2:-}"
            shift 2
            ;;
        --stage-dir)
            STAGE_DIR="${2:-}"
            shift 2
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
    echo "--version is required." >&2
    exit 1
fi

DIST_DIR="$(mkdir -p "$DIST_DIR" && cd "$DIST_DIR" && pwd)"
PACKAGE_NAME="GLDraw-v$VERSION-$PLATFORM"

if [[ -z "$STAGE_DIR" ]]; then
    STAGE_DIR="$DIST_DIR/stage/$PACKAGE_NAME"
fi

if [[ ! -x "$STAGE_DIR/bin/GLDraw" ]]; then
    echo "Missing staged GLDraw executable: $STAGE_DIR/bin/GLDraw" >&2
    exit 1
fi

APPDIR="$DIST_DIR/appimage/$PACKAGE_NAME.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/gldraw" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/scalable/apps"

cp "$STAGE_DIR/bin/GLDraw" "$APPDIR/usr/bin/GLDraw"
if [[ -d "$STAGE_DIR/share/gldraw" ]]; then
    cp -R "$STAGE_DIR/share/gldraw/." "$APPDIR/usr/share/gldraw/"
fi

cp "packaging/linux/GLDraw.desktop" "$APPDIR/usr/share/applications/GLDraw.desktop"
cp "packaging/linux/gldraw.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/gldraw.svg"
cp "packaging/linux/GLDraw.desktop" "$APPDIR/GLDraw.desktop"
cp "packaging/linux/gldraw.svg" "$APPDIR/gldraw.svg"

LINUXDEPLOY="$DIST_DIR/linuxdeploy-x86_64.AppImage"
if [[ ! -x "$LINUXDEPLOY" ]]; then
    curl -L -o "$LINUXDEPLOY" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

APPIMAGE_EXTRACT_AND_RUN=1 "$LINUXDEPLOY" --appdir "$APPDIR" --output appimage
mv "GLDraw-"*.AppImage "$DIST_DIR/$PACKAGE_NAME.AppImage"

echo ""
echo "AppImage created:"
echo "  $DIST_DIR/$PACKAGE_NAME.AppImage"
