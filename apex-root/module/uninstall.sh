#!/system/bin/sh

# APEX-Root Uninstaller
#
# 改进点：
#   1. 增加 eBPF 程序的彻底卸载（umount + rm）
#   2. 增加挂载点的清理（umount 所有 bind-mount）
#   3. 增加临时目录清理（/data/local/tmp/.apex_empty）
#   4. 增加 prop 清理（apex.fw.* / apex.module.*）
#   5. 增加卸载日志

APEX_DIR=/data/adb/modules/apex-root
UNINSTALL_LOG=/data/local/tmp/apex_uninstall.log

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$UNINSTALL_LOG" 2>/dev/null
    log -t APEX-Root "$1"
}

log "=== APEX-Root Uninstaller starting ==="

# ─── 1. 停止 service（通过 PID 文件）──────────
PID_FILE="$APEX_DIR/service.pid"
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE" 2>/dev/null)
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        log "Stopping service PID=$PID"
        kill -TERM "$PID" 2>/dev/null
        sleep 1
        kill -KILL "$PID" 2>/dev/null
    fi
    rm -f "$PID_FILE"
fi

# ─── 2. 卸载 eBPF 程序 ──────────────────
if [ -e /sys/fs/bpf/apex_firewall ]; then
    log "Unloading eBPF program"
    # 先尝试 rm（对 pinned BPF 程序有效）
    rm -f /sys/fs/bpf/apex_firewall 2>/dev/null
    # 如果还在，尝试 umount（极端情况）
    umount /sys/fs/bpf/apex_firewall 2>/dev/null
    log "eBPF program removed"
fi

# ─── 3. 卸载所有 bind-mount ──────────────────
EMPTY_DIR=/data/local/tmp/.apex_empty

# 卸载 apex-root 模块目录的 bind-mount
APEX_MODULE_PATHS="/data/adb/modules/apex-root /data/adb/ksu/modules/apex-root /data/adb/ap/modules/apex-root"
for path in $APEX_MODULE_PATHS; do
    if mountpoint -q "$path" 2>/dev/null; then
        log "Unmounting: $path"
        umount "$path" 2>/dev/null
    fi
done

# 卸载 /sbin/.magisk
if mountpoint -q /sbin/.magisk 2>/dev/null; then
    log "Unmounting: /sbin/.magisk"
    umount /sbin/.magisk 2>/dev/null
fi

# ─── 4. 清理临时目录 ──────────────────
if [ -d "$EMPTY_DIR" ]; then
    log "Cleaning up: $EMPTY_DIR"
    rm -rf "$EMPTY_DIR"
fi

# ─── 5. 清理模块目录 ──────────────────
if [ -d "$APEX_DIR" ]; then
    log "Removing module directory: $APEX_DIR"
    rm -rf "$APEX_DIR"
fi

# ─── 6. 清理 props ──────────────────
setprop apex.fw.mode ""
setprop apex.fw.ns ""
setprop apex.module.state "uninstalled"
setprop apex.module.version ""
setprop apex.hide_daemon.pid ""
setprop apex.hide_daemon.state "stopped"

log "=== Uninstall complete ==="
echo ""
echo "=========================================="
echo " APEX-Root Module Uninstalled"
echo "=========================================="
echo " Uninstall log: $UNINSTALL_LOG"
echo " Please reboot to fully clean up."
echo "=========================================="
