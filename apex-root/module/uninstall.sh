#!/system/bin/sh

# APEX-Root Uninstaller

APEX_DIR=/data/adb/modules/apex-root

log -t APEX-Root "Uninstalling..."

# Unload eBPF program
if [ -f /sys/fs/bpf/apex_firewall ]; then
    rm -f /sys/fs/bpf/apex_firewall 2>/dev/null
    log -t APEX-Root "eBPF program removed"
fi

# Clean up
rm -rf $APEX_DIR 2>/dev/null

log -t APEX-Root "Uninstall complete"
