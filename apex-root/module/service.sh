#!/system/bin/sh

# APEX-Root service
# 在 boot 完成后启动，管理防火墙状态
#
# 改进点：
#   1. 增加更健壮的 boot 等待（带超时，避免永久阻塞）
#   2. 增加 eBPF 加载失败的回退到 legacy 模式
#   3. 增加周期性健康检查（每 60s 验证隐藏状态）
#   4. 增加 PID 文件管理，避免重复启动
#   5. 增加结构化日志

APEX_DIR=/data/adb/modules/apex-root
BPF_DIR=$APEX_DIR/bpf
MODE=$(getprop apex.fw.mode)
UID_FILE=$APEX_DIR/apex_uid.txt
PID_FILE=$APEX_DIR/service.pid
LOG_FILE=$APEX_DIR/log/service.log
HEALTH_FILE=$APEX_DIR/health.status

# 日志函数（带时间戳和级别）
log() {
    local level="$1"
    local msg="$2"
    local ts=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$ts] [$level] $msg" >> "$LOG_FILE" 2>/dev/null
    log -t APEX-Root "$level: $msg"
}

# 防止重复启动
if [ -f "$PID_FILE" ]; then
    OLD_PID=$(cat "$PID_FILE" 2>/dev/null)
    if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
        log "INFO" "Service already running PID=$OLD_PID, exit"
        exit 0
    fi
fi
echo $$ > "$PID_FILE"

log "INFO" "=== APEX-Root service starting ==="
log "INFO" "Mode: $MODE, PID: $$"

# ─── 等待 boot 完成（带 120s 超时）──────────
BOOT_WAIT=0
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
    BOOT_WAIT=$((BOOT_WAIT + 2))
    if [ $BOOT_WAIT -ge 120 ]; then
        log "ERROR" "Boot wait timeout (120s), continuing anyway"
        break
    fi
done
log "INFO" "Boot completed after ${BOOT_WAIT}s"

# 额外等待 5s 让系统稳定
sleep 5

# ─── 读取 APEX app UID ──────────────────
APEX_UID=$(cat "$UID_FILE" 2>/dev/null || echo "0")
log "INFO" "APEX app UID: $APEX_UID"

# ─── 根据模式启动防火墙 ──────────────────
if [ "$MODE" = "ebpf" ]; then
    log "INFO" "Starting eBPF firewall"

    if [ -f "$BPF_DIR/apex_firewall.bpf.o" ]; then
        # 优先使用 bpftool 加载
        if command -v bpftool >/dev/null 2>&1; then
            log "INFO" "Loading eBPF via bpftool"
            if bpftool prog load "$BPF_DIR/apex_firewall.bpf.o" /sys/fs/bpf/apex_firewall 2>>"$LOG_FILE"; then
                log "INFO" "eBPF loaded successfully via bpftool"
                echo "ebpf_active" > "$HEALTH_FILE"
            else
                log "ERROR" "bpftool load failed, falling back to legacy"
                setprop apex.fw.mode legacy
                MODE=legacy
            fi
        else
            log "WARN" "bpftool not available, relying on app-side loading"
            echo "ebpf_pending_app_load" > "$HEALTH_FILE"
        fi
    else
        log "ERROR" "BPF object not found at $BPF_DIR, falling back to legacy"
        setprop apex.fw.mode legacy
        MODE=legacy
    fi
fi

if [ "$MODE" = "legacy" ]; then
    log "INFO" "Legacy mode: mount namespace hide active"
    echo "legacy_active" > "$HEALTH_FILE"
fi

# ─── 健康检查循环 ──────────────────────────
log "INFO" "Starting health check loop (60s interval)"

HEALTH_CYCLES=0
while true; do
    sleep 60
    HEALTH_CYCLES=$((HEALTH_CYCLES + 1))

    # 检查 1: 验证当前模式是否仍然有效
    CURRENT_MODE=$(getprop apex.fw.mode)
    if [ "$CURRENT_MODE" != "$MODE" ]; then
        log "WARN" "Mode changed: $MODE → $CURRENT_MODE"
        MODE=$CURRENT_MODE
    fi

    # 检查 2: eBPF 模式下验证 BPF 程序是否仍加载
    if [ "$MODE" = "ebpf" ]; then
        if [ ! -e /sys/fs/bpf/apex_firewall ]; then
            log "WARN" "eBPF program disappeared, attempting reload"
            if command -v bpftool >/dev/null 2>&1 && [ -f "$BPF_DIR/apex_firewall.bpf.o" ]; then
                bpftool prog load "$BPF_DIR/apex_firewall.bpf.o" /sys/fs/bpf/apex_firewall 2>>"$LOG_FILE" \
                    && log "INFO" "eBPF reloaded" \
                    || log "ERROR" "eBPF reload failed"
            fi
        fi
    fi

    # 检查 3: 验证 APEX app UID 是否已更新（从 0 变为真实 UID）
    NEW_UID=$(cat "$UID_FILE" 2>/dev/null || echo "0")
    if [ "$NEW_UID" != "$APEX_UID" ]; then
        log "INFO" "APEX UID updated: $APEX_UID → $NEW_UID"
        APEX_UID=$NEW_UID
    fi

    # 写入健康状态
    echo "cycle=$HEALTH_CYCLES mode=$MODE uid=$APEX_UID ts=$(date '+%s')" > "$HEALTH_FILE"

    # 每 10 个周期（10 分钟）输出一次心跳
    if [ $((HEALTH_CYCLES % 10)) -eq 0 ]; then
        log "INFO" "Heartbeat: cycle=$HEALTH_CYCLES mode=$MODE"
    fi
done
