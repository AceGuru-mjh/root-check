#!/system/bin/sh
# apex-hide-daemon — uninstall.sh
APEX_HIDE_DIR=/data/adb/apex-hide
PID_FILE="$APEX_HIDE_DIR/daemon.pid"

# 停止守护进程
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE" 2>/dev/null)
    if [ -n "$PID" ]; then
        kill -TERM "$PID" 2>/dev/null
        sleep 1
        kill -KILL "$PID" 2>/dev/null
    fi
    rm -f "$PID_FILE"
fi

# 卸载所有 bind-mount
EMPTY_DIR="$APEX_HIDE_DIR/empty"
HIDE_PATHS="$APEX_HIDE_DIR/conf/hide_paths.conf"
if [ -f "$HIDE_PATHS" ]; then
    while IFS= read -r path; do
        path=$(echo "$path" | sed 's/#.*//; s/[[:space:]]//g')
        [ -z "$path" ] && continue
        if mountpoint -q "$path" 2>/dev/null; then
            umount "$path" 2>/dev/null
        fi
    done < "$HIDE_PATHS"
fi

# 清理
rm -rf "$APEX_HIDE_DIR"
setprop apex.hide_daemon.state "stopped"
setprop apex.hide_daemon.pid ""

echo "[APEX-Hide] Uninstalled"
