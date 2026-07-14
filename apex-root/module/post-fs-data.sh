#!/system/bin/sh

# APEX-Root post-fs-data
# 早期挂载命名空间隔离
#
# 改进点：
#   1. 增加挂载失败的容错（不阻塞 boot）
#   2. 增加挂载状态记录到 prop（供 app 读取）
#   3. 增加对 KernelSU/APatch 模块路径的兼容
#   4. 修复：原脚本对目录 bind-mount /dev/null 会 EINVAL，使用空目录

log -t APEX-Root "post-fs-data: starting early hide"

# 创建空目录作为 bind-mount source
EMPTY_DIR=/data/local/tmp/.apex_empty
mkdir -p "$EMPTY_DIR" 2>/dev/null

# 状态记录
HIDE_SUCCESS=0
HIDE_FAIL=0

# 隐藏函数：尝试 bind-mount 空目录覆盖目标
hide_path() {
    local target="$1"
    local label="$2"

    if [ -e "$target" ]; then
        if mount -o bind "$EMPTY_DIR" "$target" 2>/dev/null; then
            log -t APEX-Root "Hidden: $label ($target)"
            HIDE_SUCCESS=$((HIDE_SUCCESS + 1))
        else
            log -t APEX-Root "Failed to hide: $label ($target)"
            HIDE_FAIL=$((HIDE_FAIL + 1))
        fi
    fi
}

# ─── 隐藏 apex-root 自己的模块条目 ──────────
# 兼容 Magisk / KernelSU / APatch 的模块路径
APEX_MODULE_PATHS="/data/adb/modules/apex-root"
# KernelSU 也使用 /data/adb/modules，但有时会镜像到 /data/adb/ksu/modules
[ -d /data/adb/ksu/modules/apex-root ] && APEX_MODULE_PATHS="$APEX_MODULE_PATHS /data/adb/ksu/modules/apex-root"
# APatch 路径
[ -d /data/adb/ap/modules/apex-root ] && APEX_MODULE_PATHS="$APEX_MODULE_PATHS /data/adb/ap/modules/apex-root"

for module_path in $APEX_MODULE_PATHS; do
    if [ -d "$module_path" ]; then
        hide_path "$module_path" "apex-root module"
    fi
done

# ─── 隐藏 /sbin/.magisk（如果存在）──────────
hide_path "/sbin/.magisk" "Magisk internal"

# ─── 记录挂载状态到 prop ──────────────────
if [ $HIDE_FAIL -eq 0 ]; then
    setprop apex.fw.ns "ok"
    log -t APEX-Root "post-fs-data complete: $HIDE_SUCCESS paths hidden"
elif [ $HIDE_SUCCESS -gt 0 ]; then
    setprop apex.fw.ns "partial"
    log -t APEX-Root "post-fs-data complete: $HIDE_SUCCESS ok, $HIDE_FAIL failed"
else
    setprop apex.fw.ns "fail"
    log -t APEX-Root "post-fs-data complete: all $HIDE_FAIL failed (non-fatal)"
fi

# 不阻塞 boot —— 即使隐藏失败也继续
exit 0
