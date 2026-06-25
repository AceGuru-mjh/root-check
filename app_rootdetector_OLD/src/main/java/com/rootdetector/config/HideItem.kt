package com.rootdetector.config

enum class HideCategory(val displayNameZh: String, val displayNameEn: String) {
    FILE_HIDING("文件隐藏", "File Hiding"),
    PROCESS_HIDING("进程隐藏", "Process Hiding"),
    PROPERTY_SPOOF("属性伪造", "Property Spoof"),
    MEMORY_CLEAN("内存清理", "Memory Clean"),
    MOUNT_HIDING("挂载隐藏", "Mount Hiding"),
    SELINUX_SPOOF("SELinux 伪装", "SELinux Spoof"),
    KERNEL_LEVEL("内核级隐藏", "Kernel-Level Hiding"),
    NETWORK_HIDING("网络隐藏", "Network Hiding"),
    BEHAVIOR_SPOOF("行为伪装", "Behavior Spoof"),
    SELF_PROTECT("自保护", "Self-Protection")
}

enum class HideItem(
    val id: String,
    val category: HideCategory,
    val level: Int,
    val defaultEnabled: Boolean,
    val displayNameZh: String,
    val displayNameEn: String,
    val descriptionZh: String,
    val descriptionEn: String,
    val techniqueZh: String,
    val techniqueEn: String
) {
    // ==================== H0 - 基础隐藏 (Level 1) ====================
    HIDE_SU_FILES(
        "hide_su_files", HideCategory.FILE_HIDING, 1, true,
        "隐藏 SU 二进制", "Hide SU Binary",
        "隐藏 /system/bin/su、/system/xbin/su 等路径", "Hide /system/bin/su, /system/xbin/su paths",
        "挂载命名空间隔离 / bind mount 覆盖", "Mount namespace isolation / bind mount override"
    ),
    CLEAN_ABNORMAL_DIRS(
        "clean_abnormal_dirs", HideCategory.FILE_HIDING, 1, true,
        "清理异常目录", "Clean Abnormal Dirs",
        "清理 TWRP、HMA、XPrivacyLua 等特征目录", "Clean TWRP, HMA, XPrivacyLua feature dirs",
        "目录重命名 / 删除 / 挂载覆盖", "Rename / delete / mount override"
    ),
    SPOOF_BASIC_PROPERTIES(
        "spoof_basic_properties", HideCategory.PROPERTY_SPOOF, 1, true,
        "伪装基础系统属性", "Spoof Basic Properties",
        "伪装 ro.debuggable=0、ro.secure=1 等", "Spoof ro.debuggable=0, ro.secure=1",
        "Magisk system.prop / resetprop / Property Patching", "Magisk system.prop / resetprop"
    ),
    HIDE_MODULES_DIR(
        "hide_modules_dir", HideCategory.FILE_HIDING, 1, true,
        "隐藏模块目录", "Hide Modules Directory",
        "隐藏 /data/adb/modules 目录", "Hide /data/adb/modules directory",
        "KernelSU overlayfs / Magisk 命名空间隔离", "KernelSU overlayfs / namespace isolation"
    ),
    SPOOF_BUILD_TAGS(
        "spoof_build_tags", HideCategory.PROPERTY_SPOOF, 1, true,
        "伪装编译标签", "Spoof Build Tags",
        "伪装 ro.build.tags 为 release-keys", "Spoof ro.build.tags to release-keys",
        "Magisk resetprop / prop 覆写", "Magisk resetprop / prop override"
    ),
    CLEAN_DATA_LOCAL_TMP(
        "clean_data_local_tmp", HideCategory.FILE_HIDING, 1, true,
        "清理临时目录特征", "Clean Temp Dir Traces",
        "清理 /data/local/tmp 中的 Frida 等特征文件", "Clean Frida traces from /data/local/tmp",
        "文件删除 / 目录权限修改", "File delete / dir permission change"
    ),

    // ==================== H1 - 标准隐藏 (Level 2) ====================
    HIDE_MAGISK_DAEMON(
        "hide_magisk_daemon", HideCategory.PROCESS_HIDING, 2, true,
        "隐藏 Magisk 守护进程", "Hide Magisk Daemon",
        "隐藏 magiskd 进程及其 Socket", "Hide magiskd process and sockets",
        "/proc 文件系统过滤 / SELinux 上下文伪造", "/proc filesystem filter / SELinux context spoof"
    ),
    FILTER_PROC_MAPS(
        "filter_proc_maps", HideCategory.MEMORY_CLEAN, 2, true,
        "过滤内存映射表", "Filter Memory Maps",
        "过滤 /proc/self/maps 中的敏感库映射", "Filter sensitive library maps from /proc/self/maps",
        "内核级 maps 过滤 (KernelSU/Spectre) / memfd", "Kernel-level maps filter / memfd"
    ),
    FILTER_MOUNT_INFO(
        "filter_mount_info", HideCategory.MOUNT_HIDING, 2, true,
        "过滤挂载信息", "Filter Mount Info",
        "过滤 /proc/mounts 和 mountinfo 中的敏感条目", "Filter sensitive mount entries",
        "内核级 mountinfo 过滤 (susfs)", "Kernel-level mountinfo filtering (susfs)"
    ),
    SPOOF_SELINUX_CONTEXT(
        "spoof_selinux_context", HideCategory.SELINUX_SPOOF, 2, true,
        "伪装 SELinux 上下文", "Spoof SELinux Context",
        "将 magisk/ksu/apatch 上下文伪装为系统进程", "Spoof magisk/ksu/apatch context to system",
        "SELinux 策略注入 / sepolicy 覆写", "SELinux policy injection / sepolicy override"
    ),
    HIDE_ZYGISK_LIBS(
        "hide_zygisk_libs", HideCategory.MEMORY_CLEAN, 2, true,
        "隐藏 Zygisk 注入库", "Hide Zygisk Injected Libs",
        "隐藏 zygisk/lsposed/riru 的 so 库注入痕迹", "Hide zygisk/lsposed/riru .so injection traces",
        "memfd 私有加载 / 匿名映射 / 库重命名", "memfd private loading / anonymous mapping"
    ),
    SPOOF_BOOTLOADER(
        "spoof_bootloader", HideCategory.PROPERTY_SPOOF, 2, true,
        "伪装 Bootloader 状态", "Spoof Bootloader State",
        "伪装 ro.boot.verifiedbootstate 为 green", "Spoof ro.boot.verifiedbootstate to green",
        "resetprop / KernelSU 内核伪装", "resetprop / KernelSU kernel spoof"
    ),
    SPOOF_KERNEL_VERSION(
        "spoof_kernel_version", HideCategory.PROPERTY_SPOOF, 2, false,
        "伪装内核版本", "Spoof Kernel Version",
        "伪装 uname -r 和 /proc/version", "Spoof uname -r and /proc/version",
        "内核级 newuname Hook / 字符串替换", "Kernel-level newuname hook / string replace"
    ),

    // ==================== H2 - 深度隐藏 (Level 3) ====================
    KERNEL_SYSCALL_HIDE(
        "kernel_syscall_hide", HideCategory.KERNEL_LEVEL, 3, false,
        "内核系统调用拦截", "Kernel Syscall Interception",
        "在内核层拦截文件/进程检测系统调用", "Intercept file/process detection syscalls at kernel level",
        "Spectre/KernelPatch KPM / 内核 inline hook", "Spectre/KernelPatch KPM / kernel inline hook"
    ),
    KERNEL_SYMBOL_HIDE(
        "kernel_symbol_hide", HideCategory.KERNEL_LEVEL, 3, false,
        "内核符号表清除", "Kernel Symbol Table Cleaning",
        "从 kallsyms 中清除 Root 特征符号", "Remove Root feature symbols from kallsyms",
        "内核符号表覆盖 / KPM 模块隐藏", "Kernel symbol table overwrite / KPM hide"
    ),
    AUDIT_LOG_SUPPRESS(
        "audit_log_suppress", HideCategory.KERNEL_LEVEL, 3, false,
        "审计日志抑制", "Audit Log Suppression",
        "抑制 SELinux AVC 审计日志和内核日志", "Suppress SELinux AVC audit and kernel logs",
        "内核 audit_log_start / avc_denied Hook", "Kernel audit_log_start / avc_denied hook"
    ),
    DMESG_CLEAN(
        "dmesg_clean", HideCategory.KERNEL_LEVEL, 3, false,
        "内核日志清理", "Kernel Log Cleaning",
        "从 dmesg/kmsg 中过滤敏感信息", "Filter sensitive info from dmesg/kmsg",
        "内核 devkmsg_read 过滤 / 日志替换", "Kernel devkmsg_read filter / log replacement"
    ),
    TIMING_NORMALIZE(
        "timing_normalize", HideCategory.BEHAVIOR_SPOOF, 3, false,
        "时序归一化", "Timing Normalization",
        "归一化系统调用执行时间避免时序检测", "Normalize syscall execution time to avoid timing detection",
        "延迟注入 / 时钟抖动 / 执行时间填充", "Latency injection / clock jitter / padding"
    ),
    NAMESPACE_DEEP_ISOLATE(
        "namespace_deep_isolate", HideCategory.MOUNT_HIDING, 3, false,
        "命名空间深度隔离", "Deep Namespace Isolation",
        "为每个应用创建独立的挂载/PID/网络命名空间", "Create per-app mount/PID/net namespace isolation",
        "Linux namespace 完整隔离 + 子命名空间", "Full Linux namespace + sub-namespace"
    ),
    HIDE_FROM_SCHED(
        "hide_from_sched", HideCategory.PROCESS_HIDING, 3, false,
        "调度器隐藏", "Scheduler Hiding",
        "从进程调度器视图和 /proc/sched_debug 隐藏", "Hide from scheduler and /proc/sched_debug",
        "内核调度器结构体覆盖", "Kernel scheduler struct overwrite"
    ),

    // ==================== H3 - 取证级隐藏 (Level 4) ====================
    EBPF_HIDE(
        "ebpf_hide", HideCategory.KERNEL_LEVEL, 4, false,
        "eBPF 隐藏", "eBPF Hiding",
        "从 eBPF 程序枚举中隐藏 Root 特征", "Hide Root features from eBPF program enumeration",
        "eBPF 程序列表过滤 / BPF 系统调用拦截", "eBPF program list filter / BPF syscall intercept"
    ),
    HARDWARE_SPOOF(
        "hardware_spoof", HideCategory.BEHAVIOR_SPOOF, 4, false,
        "硬件信息伪装", "Hardware Info Spoof",
        "伪装 CPU/内存/存储/网络硬件特征", "Spoof CPU/ram/storage/network hardware characteristics",
        "内核硬件信息注入 / SMBIOS 伪造", "Kernel hw info injection / SMBIOS forgery"
    ),
    CANVAS_FINGERPRINT_SPOOF(
        "canvas_fingerprint_spoof", HideCategory.BEHAVIOR_SPOOF, 4, false,
        "Canvas 指纹伪装", "Canvas Fingerprint Spoof",
        "注入噪声干扰 Canvas/WebGL 指纹采集", "Inject noise to interfere Canvas/WebGL fingerprinting",
        "WebGL 渲染器劫持 / Canvas 像素噪声", "WebGL renderer hijack / Canvas pixel noise"
    ),
    P2P_HIDE_SYNC(
        "p2p_hide_sync", HideCategory.SELF_PROTECT, 4, false,
        "P2P 隐藏状态同步", "P2P Hide State Sync",
        "通过点对点协议同步多实例隐藏状态", "Sync hide state across instances via P2P",
        "P2P 共识协议 / 状态复制", "P2P consensus protocol / state replication"
    ),
    AI_EVASION(
        "ai_evasion", HideCategory.BEHAVIOR_SPOOF, 4, false,
        "AI 检测规避", "AI Detection Evasion",
        "生成接近真实用户的指纹和行为模式", "Generate realistic user fingerprints and behavior patterns",
        "GAN 指纹生成 / 行为模拟 / 数据增强", "GAN fingerprint generation / behavior simulation"
    ),
    FULL_SELF_PROTECT(
        "full_self_protect", HideCategory.SELF_PROTECT, 4, false,
        "完整自保护", "Full Self-Protection",
        "APK 完整性校验 / 反注入 / 反调试 / 反Hook", "APK integrity / anti-injection / anti-debug / anti-hook",
        "签名校验 / 运行时完整性监控 / 多路径检测", "Signature check / runtime integrity / multi-path"
    );

    companion object {
        fun byLevel(level: Int): List<HideItem> = entries.filter { it.level == level }
        fun byCategory(category: HideCategory): List<HideItem> = entries.filter { it.category == category }
        fun byId(id: String): HideItem? = entries.firstOrNull { it.id == id }
        fun defaultsForLevel(level: Int): Set<String> =
            entries.filter { it.level <= level && it.defaultEnabled }.map { it.id }.toSet()
    }
}
