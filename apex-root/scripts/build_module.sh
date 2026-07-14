#!/usr/bin/env bash
# APEX-Root Module Build Script (Linux/Mac)
# 构建 Magisk/KSU/APatch 可刷入的 ZIP 模块
#
# 用法:
#   ./build_module.sh                  # 构建主模块 (apex-root)
#   ./build_module.sh --daemon         # 构建 apex-hide-daemon 模块
#   ./build_module.sh --all            # 构建所有模块
#   ./build_module.sh --version 1.0.4  # 指定版本号
#
# 依赖:
#   - zip (命令行工具)
#   - 可选: Android NDK (用于编译 apex-hide-daemon 二进制)
#
# 借鉴:
#   - topjohnwu/Magisk 的 build.gradle 中 module zip 打包逻辑
#   - MagiskModulesAlt-Repo/MagiskModuleProps-Updater 的 module.prop 格式

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT="$ROOT/build/output"

# 解析参数
BUILD_MAIN=0
BUILD_DAEMON=0
VERSION=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --daemon) BUILD_DAEMON=1; shift ;;
        --all) BUILD_MAIN=1; BUILD_DAEMON=1; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        --help|-h)
            echo "用法: $0 [--daemon|--all] [--version X.Y.Z]"
            exit 0
            ;;
        *) BUILD_MAIN=1; shift ;;  # 默认构建主模块
    esac
done

# 默认：构建主模块
if [[ $BUILD_MAIN -eq 0 && $BUILD_DAEMON -eq 0 ]]; then
    BUILD_MAIN=1
fi

echo "=== APEX-Root Module Builder ==="
echo "  ROOT: $ROOT"
echo "  OUT:  $OUT"
echo "  Main:   $BUILD_MAIN"
echo "  Daemon: $BUILD_DAEMON"
[[ -n "$VERSION" ]] && echo "  Version: $VERSION"
echo ""

mkdir -p "$OUT"

# 检查 zip 命令
if ! command -v zip >/dev/null 2>&1; then
    echo "[ERROR] zip 命令未找到，请安装 zip 包" >&2
    exit 1
fi

# ─── 构建主模块 (apex-root) ──────────────────────
build_main_module() {
    local MODULE_DIR="$ROOT/module"
    local MODULE_OUT="$OUT/apex-root"
    local ZIP_NAME="apex-root-module-v${VERSION:-1.0}.zip"
    local ZIP_PATH="$OUT/$ZIP_NAME"

    echo "[1/6] Building main module (apex-root)..."

    # 清理
    rm -rf "$MODULE_OUT"
    mkdir -p "$MODULE_OUT"

    # 复制模块文件
    echo "[2/6] Copying module files..."
    cp -r "$MODULE_DIR"/* "$MODULE_OUT/"

    # 复制 SELinux 策略
    echo "[3/6] Copying SELinux policy..."
    cp -f "$ROOT/sepolicy/sepolicy.rule" "$MODULE_OUT/" 2>/dev/null || \
        echo "[WARN] sepolicy.rule not found"

    # 复制 BPF 程序（预编译）
    echo "[4/6] Copying BPF programs..."
    mkdir -p "$MODULE_OUT/bpf"
    if ls "$ROOT/build/bpf/"*.o >/dev/null 2>&1; then
        cp "$ROOT/build/bpf/"*.o "$MODULE_OUT/bpf/"
        echo "  Copied $(ls "$MODULE_OUT/bpf/"*.o | wc -l) BPF object(s)"
    else
        echo "  [WARN] BPF .o files not found, creating stub"
        : > "$MODULE_OUT/bpf/apex_firewall.bpf.o"
    fi

    # 更新 module.prop 版本号（如果指定）
    if [[ -n "$VERSION" ]]; then
        sed -i.bak "s/^version=.*/version=v$VERSION/" "$MODULE_OUT/module.prop"
        sed -i.bak "s/^versionCode=.*/versionCode=$(echo $VERSION | tr -d '.' | sed 's/^0*//')/" "$MODULE_OUT/module.prop"
        rm -f "$MODULE_OUT/module.prop.bak"
    fi

    # 更新 update.json
    if [[ -n "$VERSION" ]]; then
        cat > "$MODULE_OUT/update.json" << EOF
{
  "version": "v$VERSION",
  "versionCode": $(echo $VERSION | tr -d '.' | sed 's/^0*//'),
  "zipUrl": "https://github.com/mengjinghao/root-check/releases/download/v$VERSION/apex-root-module-v$VERSION.zip",
  "changelog": "https://github.com/mengjinghao/root-check/releases/tag/v$VERSION"
}
EOF
    fi

    # 创建 ZIP
    echo "[5/6] Creating flashable ZIP..."
    rm -f "$ZIP_PATH"
    (cd "$MODULE_OUT" && zip -r "$ZIP_PATH" . -x "*.DS_Store" -x "*__MACOSX*")

    # 验证
    echo "[6/6] Verifying..."
    local SIZE=$(stat -c%s "$ZIP_PATH" 2>/dev/null || stat -f%z "$ZIP_PATH")
    echo ""
    echo "=== Main module built ==="
    echo "  ZIP:      $ZIP_PATH"
    echo "  Size:     $((SIZE / 1024)) KB"
    echo "  Contents: $(unzip -l "$ZIP_PATH" | tail -1 | awk '{print $2}') files"
    echo ""
}

# ─── 构建 apex-hide-daemon 模块 ──────────────────
build_daemon_module() {
    local MODULE_DIR="$ROOT/modules/apex-hide-daemon"
    local MODULE_OUT="$OUT/apex-hide-daemon"
    local ZIP_NAME="apex-hide-daemon-module-v${VERSION:-1.0}.zip"
    local ZIP_PATH="$OUT/$ZIP_NAME"

    echo "[1/6] Building daemon module (apex-hide-daemon)..."

    # 先尝试编译 daemon 二进制
    echo "[2/6] Building daemon binary..."
    if [[ -x "$MODULE_DIR/build_daemon.sh" ]]; then
        (
            cd "$MODULE_DIR"
            ./build_daemon.sh 2>&1 | sed 's/^/  /'
        ) || echo "  [WARN] Daemon binary build failed (will use shell fallback)"
    fi

    # 清理
    rm -rf "$MODULE_OUT"
    mkdir -p "$MODULE_OUT"

    # 复制模块文件
    echo "[3/6] Copying module files..."
    cp -r "$MODULE_DIR"/* "$MODULE_OUT/"
    rm -rf "$MODULE_OUT/src"  # 不打包源码

    # 更新 module.prop 版本号
    if [[ -n "$VERSION" ]]; then
        sed -i.bak "s/^version=.*/version=v$VERSION/" "$MODULE_OUT/module.prop"
        sed -i.bak "s/^versionCode=.*/versionCode=$(echo $VERSION | tr -d '.' | sed 's/^0*//')/" "$MODULE_OUT/module.prop"
        rm -f "$MODULE_OUT/module.prop.bak"
    fi

    # 创建 ZIP
    echo "[4/6] Creating flashable ZIP..."
    rm -f "$ZIP_PATH"
    (cd "$MODULE_OUT" && zip -r "$ZIP_PATH" . -x "*.DS_Store" -x "*__MACOSX*" -x "*.bak")

    # 验证
    echo "[5/6] Verifying..."
    local SIZE=$(stat -c%s "$ZIP_PATH" 2>/dev/null || stat -f%z "$ZIP_PATH")
    echo ""
    echo "=== Daemon module built ==="
    echo "  ZIP:      $ZIP_PATH"
    echo "  Size:     $((SIZE / 1024)) KB"
    echo "  Contents: $(unzip -l "$ZIP_PATH" | tail -1 | awk '{print $2}') files"
    echo ""
}

# 执行构建
if [[ $BUILD_MAIN -eq 1 ]]; then
    build_main_module
fi

if [[ $BUILD_DAEMON -eq 1 ]]; then
    build_daemon_module
fi

echo "=== All done ==="
echo "Output directory: $OUT"
ls -la "$OUT"/*.zip 2>/dev/null || echo "(no zip files)"
