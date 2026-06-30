#!/system/bin/sh

# APEX-Root post-fs-data
# Early mount namespace isolation
#
# 修复：原脚本 'mount -o bind /dev/null /data/adb/modules' 会覆盖整个
# modules 目录，导致 Magisk Manager 看不到任何模块（包括 apex-root 自己），
# 模块卸载/更新都会失败 —— 属于自毁行为。
#
# 现改为：只 bind-mount 隐藏 apex-root 自己的模块目录，
# 且使用空目录而非 /dev/null（避免对目录 bind-mount /dev/null 时 EINVAL）。

log -t APEX-Root "post-fs-data: starting early hide"

# 创建空目录作为 bind-mount source
EMPTY_DIR=/data/local/tmp/.apex_empty
mkdir -p "$EMPTY_DIR" 2>/dev/null

# 只隐藏 apex-root 自己的模块条目（保留 Magisk Manager 可见性）
APEX_MODULE=/data/adb/modules/apex-root
if [ -d "$APEX_MODULE" ]; then
    if mount -o bind "$EMPTY_DIR" "$APEX_MODULE" 2>/dev/null; then
        log -t APEX-Root "Hidden apex-root module entry: $APEX_MODULE"
        setprop apex.fw.ns ok
    else
        log -t APEX-Root "Failed to hide apex-root module: $APEX_MODULE"
        setprop apex.fw.ns fail
    fi
else
    # apex-root 模块未安装，跳过
    log -t APEX-Root "apex-root module not installed, skip hide"
    setprop apex.fw.ns skip
fi

# 隐藏 /sbin/.magisk（如果存在）
if [ -d /sbin/.magisk ]; then
    mount -o bind "$EMPTY_DIR" /sbin/.magisk 2>/dev/null && \
        log -t APEX-Root "Hidden /sbin/.magisk"
fi

# 不再隐藏整个 /data/adb/modules，保留 Magisk Manager 功能
log -t APEX-Root "post-fs-data complete"
