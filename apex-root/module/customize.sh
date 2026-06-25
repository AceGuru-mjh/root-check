#!/system/bin/sh

# APEX-Root Module Installer
# Supports: Magisk, KernelSU, APatch

APEX_DIR=/data/adb/modules/apex-root
UID_FILE=$APEX_DIR/apex_uid.txt
BASELINE_DIR=$APEX_DIR/baseline

echo "[APEX-Root] Installing..."

# Create directories
mkdir -p $BASELINE_DIR

# Set APEX app UID (populated by app on first run)
echo "0" > $UID_FILE

# Determine API level and kernel support for optimal path
API=$(getprop ro.build.version.sdk)
KVER=$(uname -r | cut -d. -f1,2)

echo "[APEX-Root] Android API: $API, Kernel: $KVER"

# Install eBPF program if kernel >= 5.10 (Android 12+)
BPF_VER_MAJ=$(echo $KVER | cut -d. -f1)
BPF_VER_MIN=$(echo $KVER | cut -d. -f2)
if [ $BPF_VER_MAJ -ge 5 ] && [ $BPF_VER_MIN -ge 10 ]; then
    echo "[APEX-Root] Kernel supports eBPF, installing BPF firewall"
    mkdir -p $APEX_DIR/bpf
    cp -f $MODPATH/bpf/*.o $APEX_DIR/bpf/ 2>/dev/null || true
    setprop apex.fw.mode ebpf
else
    echo "[APEX-Root] Kernel too old for eBPF, using legacy path"
    setprop apex.fw.mode legacy
fi

# SELinux policy
cp -f $MODPATH/sepolicy.rule $APEX_DIR/ 2>/dev/null || true

set_perm_recursive $MODPATH 0 0 0755 0644
set_perm $MODPATH/service.sh 0 0 0755

echo "[APEX-Root] Installation complete"
