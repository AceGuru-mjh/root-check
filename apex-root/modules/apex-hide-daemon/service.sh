#!/system/bin/sh
# apex-hide-daemon — service.sh
# 在 boot 完成后启动守护进程

MODDIR=${0%/*}
APEX_HIDE_DIR=/data/adb/apex-hide
DAEMON="$APEX_HIDE_DIR/bin/apex_hide_daemon"
PID_FILE="$APEX_HIDE_DIR/daemon.pid"
LOG_FILE="$APEX_HIDE_DIR/log/daemon.log"

# 等待 boot 完成
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# 额外等待 10 秒让系统稳定
sleep 10

echo "[APEX-Hide] Starting daemon..." > "$LOG_FILE"

# 如果二进制存在，启动它
if [ -x "$DAEMON" ]; then
    # 检查是否已在运行
    if [ -f "$PID_FILE" ]; then
        OLD_PID=$(cat "$PID_FILE" 2>/dev/null)
        if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
            echo "[APEX-Hide] Daemon already running PID=$OLD_PID" >> "$LOG_FILE"
            exit 0
        fi
    fi

    # 启动守护进程
    nohup "$DAEMON" >> "$LOG_FILE" 2>&1 &
    DAEMON_PID=$!
    echo "$DAEMON_PID" > "$PID_FILE"
    echo "[APEX-Hide] Daemon started PID=$DAEMON_PID" >> "$LOG_FILE"

    # 等待 daemon 初始化
    sleep 2
    if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
        echo "[APEX-Hide] ERROR: Daemon exited prematurely" >> "$LOG_FILE"
        rm -f "$PID_FILE"
        exit 1
    fi

    setprop apex.hide_daemon.pid "$DAEMON_PID"
    setprop apex.hide_daemon.state "running"
    echo "[APEX-Hide] Daemon ready" >> "$LOG_FILE"
else
    # 二进制不存在，使用 shell fallback
    echo "[APEX-Hide] Daemon binary not found, using shell fallback" >> "$LOG_FILE"
    "$MODDIR/fallback.sh" >> "$LOG_FILE" 2>&1 &
    FALLBACK_PID=$!
    echo "$FALLBACK_PID" > "$PID_FILE"
    setprop apex.hide_daemon.state "fallback"
    echo "[APEX-Hide] Shell fallback started PID=$FALLBACK_PID" >> "$LOG_FILE"
fi
