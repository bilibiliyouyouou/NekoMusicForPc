#!/bin/bash
# Neko云音乐 macOS Intel x86 Build & Package Script
# Produces: Neko云音乐-{version}-macos-intel-x86_64.dmg

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build-mac-intel"
BUILD_TYPE="${1:-Release}"

echo "========================================="
echo "  Neko云音乐 macOS Intel x86 Build Script"
echo "========================================="
echo "Build type: $BUILD_TYPE"
echo "Architecture: x86_64 (Intel)"
echo ""

# Check for Qt6 installation
QT_DIR="$HOME/Qt/6.11.0/macos"
if [ ! -d "$QT_DIR" ]; then
    echo "ERROR: Qt6 not found at $QT_DIR"
    echo "Please install Qt6.11.0 for macOS"
    exit 1
fi

# Generate AppIcon.icns if not exists
if [ ! -f "src/resources/macos/AppIcon.icns" ]; then
    echo "Generating AppIcon.icns..."
    mkdir -p src/resources/macos
    ICONSET=$(mktemp -d)/AppIcon.iconset
    mkdir -p "$ICONSET"
    SRC="packaging/icons/hicolor/512x512/apps/nekomusic.png"
    for size in 16 32 64 128 256 512; do
        sips -z $size $size "$SRC" --out "$ICONSET/icon_${size}x${size}.png"
        sips -z $((size*2)) $((size*2)) "$SRC" --out "$ICONSET/icon_${size}x${size}@2x.png"
    done
    iconutil -c icns "$ICONSET" -o "src/resources/macos/AppIcon.icns"
    rm -rf "$ICONSET"
fi

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_OSX_ARCHITECTURES=x86_64

# Build
echo ""
echo "Building..."
cmake --build . -j"$(sysctl -n hw.ncpu)"

# Find the generated .app bundle
APP_PATH=$(find . -type d -name "*.app" | head -1)
if [ -z "$APP_PATH" ]; then
    echo "ERROR: Could not find .app bundle in build directory"
    exit 1
fi
APP_NAME=$(basename "$APP_PATH")

# Get version from CMake (first try CMakeCache.txt, then CMakeLists.txt)
VERSION=$(grep -A1 "project(NekoMusic" ../CMakeLists.txt | grep -o 'VERSION [0-9.]*' | grep -o '[0-9.]*' | head -1)
if [ -z "$VERSION" ]; then
    VERSION="0.0.0"
fi
SUFFIX=""
FULL_VERSION="${VERSION}${SUFFIX}"

echo ""
echo "App bundle: $APP_PATH"
echo "Version: $FULL_VERSION"

# Deploy Qt libraries and create DMG using macdeployqt
echo ""
echo "Creating DMG..."
MACDEPLOYQT="$QT_DIR/bin/macdeployqt"
if [ ! -f "$MACDEPLOYQT" ]; then
    echo "ERROR: macdeployqt not found at $MACDEPLOYQT"
    exit 1
fi

# Run macdeployqt with -dmg option
"$MACDEPLOYQT" "$APP_PATH" -dmg

# Find the generated DMG (macdeployqt creates it alongside the .app)
DMG_FILE=$(find . -name "*.dmg" -type f | head -1)
if [ -z "$DMG_FILE" ]; then
    echo "ERROR: Failed to create DMG"
    exit 1
fi

# Rename DMG to desired naming convention
TARGET_NAME="Neko云音乐-${FULL_VERSION}-macos-intel-x86_64.dmg"
mv "$DMG_FILE" "$TARGET_NAME"

# Copy to dist directory
OUTPUT_DIR="$SCRIPT_DIR/dist"
mkdir -p "$OUTPUT_DIR"
cp "$TARGET_NAME" "$OUTPUT_DIR/"

echo ""
echo "========================================="
echo "  Build Successful!"
echo "========================================="
echo "Output: dist/$TARGET_NAME"
echo ""
echo "To mount DMG: open dist/$TARGET_NAME"