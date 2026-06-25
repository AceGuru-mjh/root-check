#!/system/bin/sh

# APEX-Root service
# Runs in background, manages firewall state

APEX_DIR=/data/adb/modules/apex-root
BPF_DIR=$APEX_DIR/bpf
MODE=$(getprop apex.fw.mode)
UID_FILE=$APEX_DIR/apex_uid.txt

log() {
    echo "[APEX-Root] $1"
    log -t APEX-Root "$1"
}

# Wait for boot completion
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

log "Service starting, mode=$MODE"

# Read APEX app UID
APEX_UID=$(cat $UID_FILE 2>/dev/null || echo "0")

if [ "$MODE" = "ebpf" ]; then
    log "Starting eBPF firewall (UID=$APEX_UID)"
    if [ -f $BPF_DIR/apex_firewall.o ]; then
        # Load eBPF program via bpftool or direct syscall
        if command -v bpftool >/dev/null 2>&1; then
            bpftool prog load $BPF_DIR/apex_firewall.o /sys/fs/bpf/apex_firewall
            log "eBPF loaded via bpftool"
        else
            log "bpftool not available, relying on app-side loading"
        fi
    else
        log "BPF object not found at $BPF_DIR"
    fi
else
    log "Legacy mode: mount namespace hide active"
    # Mount namespace isolation handled by /data/adb/service.d
fi

log "APEX-Root ready"
