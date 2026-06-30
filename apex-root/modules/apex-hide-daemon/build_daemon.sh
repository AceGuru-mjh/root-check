# apex-hide-daemon 构建脚本
# 编译 src/apex_hide_daemon.cpp 为 ARM64 Android 二进制
#
# 用法:
#   ./build_daemon.sh              # 用 NDK 编译
#   ./build_daemon.sh --host       # 用本机 g++ 编译（仅用于语法检查）
#
# 依赖:
#   - Android NDK (推荐 r26+)
#   - 或 clang/gcc with arm64 cross-compile support

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_DIR="$SCRIPT_DIR/.."
SRC="$MODULE_DIR/src/apex_hide_daemon.cpp"
OUTPUT_DIR="$MODULE_DIR/bin"
OUTPUT="$OUTPUT_DIR/apex_hide_daemon"

mkdir -p "$OUTPUT_DIR"

# 检查源文件
if [ ! -f "$SRC" ]; then
    echo "[build_daemon] ERROR: source not found: $SRC" >&2
    exit 1
fi

# 寻找 NDK
NDK_PATH="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
HOST_MODE=0

if [ "$1" = "--host" ]; then
    HOST_MODE=1
    echo "[build_daemon] Host compile mode (syntax check)"
elif [ -z "$NDK_PATH" ] || [ ! -d "$NDK_PATH" ]; then
    echo "[build_daemon] WARNING: ANDROID_NDK_HOME not set, trying host compiler"
    HOST_MODE=1
else
    echo "[build_daemon] Using NDK: $NDK_PATH"
fi

if [ "$HOST_MODE" = "1" ]; then
    # 主机编译（用于语法检查）
    if ! command -v g++ >/dev/null 2>&1; then
        echo "[build_daemon] ERROR: g++ not found" >&2
        exit 1
    fi
    g++ -std=c++17 -O2 -Wall -Wextra \
        -DHOST_BUILD \
        -o "${OUTPUT}_host" "$SRC" \
        -lpthread 2>&1 | head -50 || {
        echo "[build_daemon] Host compile failed (expected on non-Linux), syntax check only"
        g++ -std=c++17 -fsyntax-only -DHOST_BUILD "$SRC" 2>&1 | head -50
        exit 0
    }
    echo "[build_daemon] ✓ Host binary: ${OUTPUT}_host"
    # 也复制到 output 作为占位
    cp -f "${OUTPUT}_host" "$OUTPUT" 2>/dev/null || true
    exit 0
fi

# NDK 交叉编译
NDK_BUILD="$NDK_PATH/ndk-build"
if [ ! -x "$NDK_BUILD" ]; then
    # 尝试独立工具链
    TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang++"
    if [ ! -x "$TOOLCHAIN" ]; then
        echo "[build_daemon] ERROR: NDK toolchain not found" >&2
        exit 1
    fi
    "$TOOLCHAIN" -std=c++17 -O2 -Wall \
        -static -fPIE -pie \
        -o "$OUTPUT" "$SRC" \
        -lpthread -llog
else
    # 用 NDK 直接编译
    API=24
    HOST_TAG=linux-x86_64
    TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/$HOST_TAG"
    TARGET=aarch64-linux-android$API
    CC="$TOOLCHAIN/bin/${TARGET}++"

    if [ ! -x "$CC" ]; then
        echo "[build_daemon] ERROR: $CC not found" >&2
        exit 1
    fi

    "$CC" -std=c++17 -O2 -Wall \
        -fPIE -pie \
        -o "$OUTPUT" "$SRC" \
        -lpthread -llog
fi

if [ -s "$OUTPUT" ]; then
    echo "[build_daemon] ✓ Built: $OUTPUT ($(stat -c%s "$OUTPUT") bytes)"
else
    echo "[build_daemon] ERROR: output empty" >&2
    exit 1
fi
