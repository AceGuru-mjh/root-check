#!/system/bin/sh
# apex-hide-daemon — Magisk/KSU/APatch module installer
# 这个模块安装一个独立的 root 守护进程，在 boot 时启动，
# 通过 mount namespace 隔离 + LD_PRELOAD 拦截 libc，
# 对除白名单外的所有应用隐藏 root 痕迹。

MODDIR=${0%/*}
APEX_HIDE_DIR=/data/adb/apex-hide

echo "[APEX-Hide] Installing standalone hide daemon..."

# 创建工作目录
mkdir -p "$APEX_HIDE_DIR"
mkdir -p "$APEX_HIDE_DIR/bin"
mkdir -p "$APEX_HIDE_DIR/conf"
mkdir -p "$APEX_HIDE_DIR/log"
mkdir -p "$APEX_HIDE_DIR/empty"

# 复制守护进程二进制（占位，实际二进制需在 build 时填入）
if [ -f "$MODDIR/bin/apex_hide_daemon" ]; then
    cp -f "$MODDIR/bin/apex_hide_daemon" "$APEX_HIDE_DIR/bin/"
    chmod 0755 "$APEX_HIDE_DIR/bin/apex_hide_daemon"
    echo "[APEX-Hide] Daemon binary installed"
else
    echo "[APEX-Hide] WARNING: Daemon binary not found, using shell fallback"
fi

# 复制 LD_PRELOAD hook 库（占位）
if [ -f "$MODDIR/bin/libapex_hook.so" ]; then
    cp -f "$MODDIR/bin/libapex_hook.so" "$APEX_HIDE_DIR/bin/"
    chmod 0755 "$APEX_HIDE_DIR/bin/libapex_hook.so"
    echo "[APEX-Hide] Hook library installed"
fi

# 默认配置：白名单只含 root (UID 0) 和 system (UID 1000)
cat > "$APEX_HIDE_DIR/conf/whitelist.conf" << 'EOF'
# APEX Hide Daemon whitelist
# 每行一个 UID，这些 UID 的进程可以看到真实 root 状态
# 其他 UID 的进程会被隐藏
0
1000
EOF

# 默认隐藏路径配置
cat > "$APEX_HIDE_DIR/conf/hide_paths.conf" << 'EOF'
# APEX Hide Daemon hide paths
# 每行一个路径，这些路径对非白名单进程返回 ENOENT
/sbin/.magisk
/data/adb/magisk
/data/adb/ksu
/data/adb/ap
/data/adb/lspd
/data/adb/modules
/data/adb/shamiko
/data/adb/zygisknext
/data/adb/rezygisk
/proc/kernelsu
/sys/kpm
EOF

# 默认隐藏进程名配置
cat > "$APEX_HIDE_DIR/conf/hide_procs.conf" << 'EOF'
# APEX Hide Daemon hide process names
# 每行一个进程名，这些进程在 /proc 中对非白名单进程不可见
magiskd
magisk
ksud
apd
zygisknext
rezygiskd
shamiko
lspd
EOF

set_perm_recursive "$APEX_HIDE_DIR" 0 0 0755 0644
set_perm "$APEX_HIDE_DIR/bin/apex_hide_daemon" 0 0 0755 2>/dev/null || true
set_perm "$APEX_HIDE_DIR/bin/libapex_hook.so" 0 0 0755 2>/dev/null || true

echo "[APEX-Hide] Installation complete"
echo "[APEX-Hide] Daemon will start on next boot via service.sh"
