#!/bin/bash
# 编译 apex_firewall.bpf.c 为 BPF 字节码 (.o)
# 用法: ./scripts/build_bpf.sh [output_dir]
#
# 依赖:
#   - clang (>= 12, 支持 -target bpf)
#   - llvm (可选，用于 bpftool gen skeleton)
#   - linux 内核头 (vmlinux.h 可选，BPF CO-RE)
#
# 输出:
#   $OUTPUT_DIR/apex_firewall.bpf.o  — BPF 字节码，供 service.sh bpftool 加载

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BPF_SRC="$REPO_ROOT/apex-root/bpf/apex_firewall.bpf.c"
BPF_HEADER_DIR="$REPO_ROOT/apex-root/bpf"

OUTPUT_DIR="${1:-$REPO_ROOT/build/bpf}"
mkdir -p "$OUTPUT_DIR"

OUTPUT_OBJ="$OUTPUT_DIR/apex_firewall.bpf.o"

echo "[build_bpf] Compiling $BPF_SRC"
echo "[build_bpf] Output: $OUTPUT_OBJ"

# 检查 clang
if ! command -v clang >/dev/null 2>&1; then
    echo "[build_bpf] ERROR: clang not found. Install clang >= 12." >&2
    exit 1
fi

CLANG_VERSION=$(clang --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
echo "[build_bpf] clang version: $CLANG_VERSION"

# 检查源文件
if [ ! -f "$BPF_SRC" ]; then
    echo "[build_bpf] ERROR: BPF source not found at $BPF_SRC" >&2
    exit 1
fi

# 编译 BPF 程序
# -target bpf: BPF 目标架构
# -O2: 优化级别
# -g: 生成调试信息（BTF）
# -c: 仅编译，不链接
# -D__TARGET_ARCH_arm64: ARM64 目标
# -I: 包含 bpf/ 目录
clang \
    -target bpf \
    -O2 -g \
    -D__TARGET_ARCH_arm64 \
    -I"$BPF_HEADER_DIR" \
    -I/usr/include \
    -I/usr/include/$(uname -m)-linux-gnu \
    -c "$BPF_SRC" \
    -o "$OUTPUT_OBJ"

if [ ! -s "$OUTPUT_OBJ" ]; then
    echo "[build_bpf] ERROR: Output object is empty" >&2
    exit 1
fi

OBJ_SIZE=$(stat -c%s "$OUTPUT_OBJ" 2>/dev/null || stat -f%z "$OUTPUT_OBJ")
echo "[build_bpf] ✓ Compiled successfully ($OBJ_SIZE bytes)"

# 可选：使用 bpftool 生成 skeleton
if command -v bpftool >/dev/null 2>&1; then
    echo "[build_bpf] Generating skeleton with bpftool..."
    SKELETON="$OUTPUT_DIR/apex_firewall.skel.h"
    if bpftool gen skeleton "$OUTPUT_OBJ" > "$SKELETON" 2>/dev/null; then
        echo "[build_bpf] ✓ Skeleton: $SKELETON"
    else
        echo "[build_bpf] ⚠ Skeleton generation failed (non-fatal)"
    fi
else
    echo "[build_bpf] ⚠ bpftool not found, skipping skeleton generation"
fi

# 复制到 Magisk 模块目录（如果 build_module.ps1 调用）
MODULE_BPF_DIR="$REPO_ROOT/apex-root/module/bpf"
if [ -d "$MODULE_BPF_DIR" ] || [ -n "$APEX_INSTALL_MODULE" ]; then
    mkdir -p "$MODULE_BPF_DIR"
    cp -f "$OUTPUT_OBJ" "$MODULE_BPF_DIR/"
    echo "[build_bpf] ✓ Copied to $MODULE_BPF_DIR/"
fi

echo "[build_bpf] Done."
