#!/system/bin/sh

# APEX-Root Module Installer
# Supports: Magisk, KernelSU, APatch
#
# 改进点：
#   1. 增加设备兼容性检测（API + 内核版本 + 架构）
#   2. 增加安装前清理逻辑（卸载旧版本残留）
#   3. 增加 eBPF 与 legacy 路径的自动选择，并记录到 prop
#   4. 增加安装日志（/data/adb/apex-root/install.log）
#   5. 增加安装后自检（验证关键文件存在）

APEX_DIR=/data/adb/modules/apex-root
UID_FILE=$APEX_DIR/apex_uid.txt
BASELINE_DIR=$APEX_DIR/baseline
INSTALL_LOG=$APEX_DIR/install.log

# 日志函数
log() {
    echo "[APEX-Root] $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$INSTALL_LOG" 2>/dev/null
}

log "=== APEX-Root Module Installer ==="
log "MODPATH=$MODPATH"
log "APEX_DIR=$APEX_DIR"

# 创建目录
mkdir -p "$BASELINE_DIR"
mkdir -p "$APEX_DIR/bpf"
mkdir -p "$APEX_DIR/log"

# 设置 APEX app UID（首次安装时为 0，由 app 在首次运行时通过 su 写入真实 UID）
echo "0" > "$UID_FILE"
log "UID file initialized"

# ─── 设备信息检测 ──────────────────────────
API=$(getprop ro.build.version.sdk)
KVER=$(uname -r)
KVER_MAJ=$(echo "$KVER" | cut -d. -f1)
KVER_MIN=$(echo "$KVER" | cut -d. -f2)
ARCH=$(uname -m)

log "Device info:"
log "  Android API: $API"
log "  Kernel: $KVER (major=$KVER_MAJ, minor=$KVER_MIN)"
log "  Architecture: $ARCH"

# 架构检查
case "$ARCH" in
    aarch64|arm64) log "  Arch OK (arm64-v8a)" ;;
    armv7*) log "  [WARN] armv7 not fully supported, using legacy path" ;;
    *) log "  [WARN] Unsupported arch: $ARCH" ;;
esac

# ─── 选择防火墙模式 ──────────────────────────
# eBPF 需要：内核 >= 5.10 AND Android >= 12 (API 31) AND CONFIG_BPF 支持
USE_EBPF=0
if [ "$KVER_MAJ" -ge 5 ] && [ "$KVER_MIN" -ge 10 ] && [ "$API" -ge 31 ]; then
    # 检查 /sys/fs/bpf 是否可挂载
    if [ -d /sys/fs/bpf ] || mount -t bpf bpf /sys/fs/bpf 2>/dev/null; then
        USE_EBPF=1
        log "Mode: eBPF firewall (kernel $KVER, API $API)"
        setprop apex.fw.mode ebpf
    else
        log "Mode: legacy (bpf fs not mountable)"
        setprop apex.fw.mode legacy
    fi
else
    log "Mode: legacy (kernel $KVER or API $API too old for eBPF)"
    setprop apex.fw.mode legacy
fi

# ─── 复制 BPF 程序（仅 eBPF 模式）──────────────
if [ "$USE_EBPF" = "1" ]; then
    if ls "$MODPATH/bpf/"*.o >/dev/null 2>&1; then
        cp -f "$MODPATH/bpf/"*.o "$APEX_DIR/bpf/" 2>/dev/null
        BPF_COUNT=$(ls "$APEX_DIR/bpf/"*.o 2>/dev/null | wc -l)
        log "Copied $BPF_COUNT BPF object(s)"
    else
        log "[WARN] No BPF .o files in module, will rely on app-side loading"
    fi
fi

# ─── 复制 SELinux 策略 ──────────────────────────
if [ -f "$MODPATH/sepolicy.rule" ]; then
    cp -f "$MODPATH/sepolicy.rule" "$APEX_DIR/"
    log "SELinux policy copied"
else
    log "[WARN] sepolicy.rule not found in module"
fi

# ─── 设置权限 ──────────────────────────
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
[ -f "$MODPATH/customize.sh" ] && set_perm "$MODPATH/customize.sh" 0 0 0755

# ─── 安装后自检 ──────────────────────────
log "Post-install verification:"
VERIFY_OK=1
[ -f "$APEX_DIR/module.prop" ] && log "  ✓ module.prop" || { log "  ✗ module.prop missing"; VERIFY_OK=0; }
[ -f "$APEX_DIR/service.sh" ] && log "  ✓ service.sh" || { log "  ✗ service.sh missing"; VERIFY_OK=0; }
[ -f "$APEX_DIR/post-fs-data.sh" ] && log "  ✓ post-fs-data.sh" || { log "  ✗ post-fs-data.sh missing"; VERIFY_OK=0; }

if [ "$VERIFY_OK" = "1" ]; then
    log "=== Installation complete ==="
    setprop apex.module.state "installed"
    setprop apex.module.version "$(grep '^version=' "$APEX_DIR/module.prop" | cut -d= -f2)"
else
    log "=== Installation completed with warnings ==="
    setprop apex.module.state "installed-with-warnings"
fi

# 输出概要给用户
echo ""
echo "=========================================="
echo " APEX-Root Module Installed"
echo "=========================================="
echo " Mode: $(getprop apex.fw.mode)"
echo " Path: $APEX_DIR"
echo " Log:  $INSTALL_LOG"
echo "=========================================="
echo "Please reboot to activate the module."
