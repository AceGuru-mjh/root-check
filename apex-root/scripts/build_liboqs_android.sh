#!/usr/bin/env bash
# APEX-Root liboqs Android Build Script (Linux/macOS)
set -euo pipefail

NDK_PATH="${ANDROID_NDK_HOME:-}"
LIBOQS_VER="0.15.0"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIBOQS_DIR="$PROJECT_ROOT/app/src/main/cpp/liboqs"
BUILD_DIR="$PROJECT_ROOT/build/liboqs_android"

echo "=== APEX-Root liboqs Android Build ==="
echo "liboqs version: $LIBOQS_VER"

# Detect NDK
if [ -z "$NDK_PATH" ]; then
    for d in "$ANDROID_HOME/ndk-bundle" "$ANDROID_HOME/ndk/26.3.11579264" "$HOME/Android/Sdk/ndk/26.3.11579264"; do
        if [ -f "$d/build/cmake/android.toolchain.cmake" ]; then
            NDK_PATH="$d"
            break
        fi
    done
fi

if [ -z "$NDK_PATH" ] || [ ! -d "$NDK_PATH" ]; then
    echo "ERROR: Android NDK not found. Set ANDROID_NDK_HOME or ANDROID_HOME." >&2
    exit 1
fi
echo "Using NDK: $NDK_PATH"

# Download liboqs if needed
if [ ! -f "$LIBOQS_DIR/CMakeLists.txt" ]; then
    echo "Downloading liboqs v$LIBOQS_VER..."
    curl -sL "https://github.com/open-quantum-safe/liboqs/archive/refs/tags/$LIBOQS_VER.tar.gz" \
        -o /tmp/liboqs-$LIBOQS_VER.tar.gz
    rm -rf /tmp/liboqs-$LIBOQS_VER 2>/dev/null
    tar -xzf /tmp/liboqs-$LIBOQS_VER.tar.gz -C /tmp
    rm -rf "$LIBOQS_DIR"
    mv /tmp/liboqs-$LIBOQS_VER "$LIBOQS_DIR"
    echo "liboqs extracted to $LIBOQS_DIR"
fi

# Build
mkdir -p "$BUILD_DIR"
cmake -S "$LIBOQS_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=29 \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DBUILD_SHARED_LIBS=OFF \
    -DOQS_BUILD_ONLY_LIB=ON \
    -DOQS_USE_OPENSSL=OFF \
    -DOQS_ENABLE_SIG_DILITHIUM=ON \
    -DOQS_ENABLE_SIG_ALL=OFF \
    -DOQS_ENABLE_KEM_ALL=OFF \
    -DOQS_OPT_FLAGS="-O2 -ffunction-sections -fdata-sections" \
    -DOQS_DIST_ARM=ON \
    -DOQS_DIST_ARM64=ON

cmake --build "$BUILD_DIR" --target oqs -- -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"

# Copy artifacts
mkdir -p "$LIBOQS_DIR/lib" "$LIBOQS_DIR/include/oqs"
cp "$BUILD_DIR/lib/liboqs.a" "$LIBOQS_DIR/lib/"
cp "$BUILD_DIR/include/oqs/"*.h "$LIBOQS_DIR/include/oqs/"
cp "$LIBOQS_DIR/src/common/common.h" "$LIBOQS_DIR/include/oqs/" 2>/dev/null || true

echo ""
echo "=== Build Complete ==="
echo "Library: $LIBOQS_DIR/lib/liboqs.a"
echo "Headers: $LIBOQS_DIR/include/oqs/"
echo ""
echo "Now run: cd $PROJECT_ROOT && ./gradlew assembleRelease"
