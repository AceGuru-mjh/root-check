package com.rootdetector.config

enum class Category(val displayNameZh: String, val displayNameEn: String) {
    ROOT_BINARY("Root 二进制", "Root Binary"),
    ROOT_MANAGER("Root 管理器", "Root Manager"),
    HOOK_FRAMEWORK("Hook 框架", "Hook Framework"),
    SYSTEM_PROPERTY("系统属性", "System Property"),
    BOOTLOADER("Bootloader", "Bootloader"),
    PROCESS("进程检测", "Process"),
    FILE_SYSTEM("文件系统", "File System"),
    MOUNT("挂载点", "Mount"),
    SELINUX("SELinux", "SELinux"),
    KERNEL("内核", "Kernel"),
    MEMORY("内存", "Memory"),
    TIMING_SIDE_CHANNEL("时序侧信道", "Timing Side-Channel"),
    TEE("TEE 安全", "TEE Security"),
    FIRMWARE("固件", "Firmware"),
    VIRTUALIZATION("虚拟化", "Virtualization"),
    NETWORK("网络", "Network"),
    DEBUG("调试检测", "Debug Detection"),
    DUAL_APP("多开/工作资料", "Dual App/Work Profile"),
    CUSTOM_ROM("自定义 ROM", "Custom ROM"),
    ADVANCED("高级检测", "Advanced Detection")
}

enum class DetectionItem(
    val id: String,
    val category: Category,
    val level: Int,
    val defaultEnabled: Boolean,
    val displayNameZh: String,
    val displayNameEn: String,
    val descriptionZh: String,
    val descriptionEn: String,
    val referenceSource: String = ""
) {
    // ==================== L0 - 基础检测 (Level 1) ====================
    SU_BINARY(
        "su_binary", Category.ROOT_BINARY, 1, true,
        "SU 二进制检测", "SU Binary Detection",
        "扫描标准路径下的 su 二进制文件", "Scan standard paths for su binary",
        "Ruru/春秋/Momo 基础"
    ),
    SUSPICIOUS_APPS(
        "suspicious_apps", Category.ROOT_MANAGER, 1, true,
        "可疑应用检测", "Suspicious Apps Detection",
        "检测已安装的 Root 管理器应用包名", "Detect installed Root manager packages",
        "Ruru/春秋/Momo"
    ),
    ABNORMAL_DIRECTORIES(
        "abnormal_directories", Category.FILE_SYSTEM, 1, true,
        "异常目录检测", "Abnormal Directory Detection",
        "检测 TWRP/HMA/XPrivacyLua 等异常目录", "Detect TWRP/HMA/XPrivacyLua dirs",
        "Ruru"
    ),
    DUAL_APP_DETECT(
        "dual_app_detect", Category.DUAL_APP, 1, true,
        "多开/工作资料检测", "Dual App/Work Profile Detection",
        "检测应用是否运行在多开或工作资料环境中", "Detect dual app or work profile",
        "Ruru"
    ),
    XPOSED_HOOKS(
        "xposed_hooks", Category.HOOK_FRAMEWORK, 1, true,
        "Xposed Hook 检测(Java层)", "Xposed Hook Detection (Java)",
        "检测 Xposed/LSPosed 框架注入的 Java Hook", "Detect Xposed/LSPosed Java hooks",
        "Ruru/春秋"
    ),
    MAPS_SCAN_BASIC(
        "maps_scan_basic", Category.MEMORY, 1, true,
        "内存映射基础扫描", "Basic Memory Map Scan",
        "扫描 /proc/self/maps 中的 Hook 框架特征", "Scan /proc/self/maps for hook frameworks",
        "Momo/春秋"
    ),
    PROPERTY_CHECK_BASIC(
        "property_check_basic", Category.SYSTEM_PROPERTY, 1, true,
        "系统属性基础检测", "Basic Property Check",
        "检测 ro.debuggable、ro.secure 等关键属性", "Check ro.debuggable, ro.secure properties",
        "春秋/Momo"
    ),
    BOOTLOADER_CHECK(
        "bootloader_check", Category.BOOTLOADER, 1, true,
        "Bootloader 锁状态检测", "Bootloader Lock State",
        "检测设备 Bootloader 是否已解锁", "Detect if bootloader is unlocked",
        "春秋"
    ),

    // ==================== L1 - 标准检测 (Level 2) ====================
    MAGISK_DAEMON(
        "magisk_daemon", Category.ROOT_MANAGER, 2, true,
        "Magisk 守护进程检测", "Magisk Daemon Detection",
        "检测 magiskd 进程及 Magisk 特征", "Detect magiskd process and Magisk features",
        "春秋/Momo"
    ),
    ZYGOSK_DETECT(
        "zygisk_detect", Category.HOOK_FRAMEWORK, 2, true,
        "Zygisk 注入检测", "Zygisk Injection Detection",
        "检测 Zygisk 的 so 库注入痕迹", "Detect Zygisk library injection traces",
        "春秋/Momo"
    ),
    KERNELSU_DETECT(
        "kernelsu_detect", Category.ROOT_MANAGER, 2, true,
        "KernelSU 检测", "KernelSU Detection",
        "检测 KernelSU overlayfs 及内核特征", "Detect KernelSU overlayfs and kernel features",
        "春秋"
    ),
    APATCH_DETECT(
        "apatch_detect", Category.ROOT_MANAGER, 2, true,
        "APatch 检测", "APatch Detection",
        "检测 APatch 的 KPM 内核补丁特征", "Detect APatch KPM kernel patches",
        "春秋"
    ),
    LSPOSED_DETECT(
        "lsposed_detect", Category.HOOK_FRAMEWORK, 2, true,
        "LSPosed 框架检测", "LSPosed Framework Detection",
        "检测 LSPosed/Riru 框架的文件和进程特征", "Detect LSPosed/Riru file and process traces",
        "春秋"
    ),
    FRIDA_DETECT(
        "frida_detect", Category.HOOK_FRAMEWORK, 2, true,
        "Frida 检测", "Frida Detection",
        "端口扫描、D-Bus 协议、内存特征检测 Frida", "Port scan, D-Bus, memory detection for Frida",
        "春秋/Momo"
    ),
    SHAMIKO_DETECT(
        "shamiko_detect", Category.HOOK_FRAMEWORK, 2, true,
        "Shamiko 隐藏工具检测", "Shamiko Hiding Tool Detection",
        "检测 Shamiko 的行为特征和文件残留", "Detect Shamiko behavior and file residues",
        "春秋"
    ),
    PROCESS_SCAN(
        "process_scan", Category.PROCESS, 2, true,
        "全局进程扫描", "Full Process Scan",
        "扫描所有运行进程的名称和 SELinux 上下文", "Scan all running process names and SELinux contexts",
        "春秋/Momo"
    ),
    FILE_SYSTEM_SCAN(
        "file_system_scan", Category.FILE_SYSTEM, 2, true,
        "文件系统深度扫描", "Deep File System Scan",
        "扫描 100+ 个 Root 工具相关路径", "Scan 100+ Root tool related paths",
        "春秋"
    ),
    MOUNT_CHECK(
        "mount_check", Category.MOUNT, 2, true,
        "挂载点检测", "Mount Point Detection",
        "检测 overlayfs、loop 设备等异常挂载", "Detect overlayfs, loop devices, abnormal mounts",
        "春秋/Momo"
    ),
    SELINUX_CHECK(
        "selinux_check", Category.SELINUX, 2, true,
        "SELinux 策略检测", "SELinux Policy Check",
        "检测 SELinux 强制状态和上下文异常", "Check SELinux enforcing mode and context",
        "春秋/Momo"
    ),
    KERNEL_MODULE_CHECK(
        "kernel_module_check", Category.KERNEL, 2, true,
        "内核模块检测", "Kernel Module Detection",
        "检测已加载的可疑内核模块", "Detect suspicious loaded kernel modules",
        "春秋"
    ),
    RECOVERY_DETECT(
        "recovery_detect", Category.FILE_SYSTEM, 2, true,
        "第三方 Recovery 检测", "Third-party Recovery Detection",
        "检测 TWRP/OrangeFox 等第三方 Recovery", "Detect TWRP/OrangeFox custom recoveries",
        "Ruru/春秋"
    ),
    CUSTOM_ROM_DETECT(
        "custom_rom_detect", Category.CUSTOM_ROM, 2, false,
        "自定义 ROM 检测", "Custom ROM Detection",
        "检测系统是否为第三方自定义 ROM", "Detect if running a custom ROM",
        "Momo"
    ),

    // ==================== L2 - 深度检测 (Level 3) ====================
    SYSCALL_TIMING(
        "syscall_timing", Category.TIMING_SIDE_CHANNEL, 3, false,
        "系统调用时序分析", "Syscall Timing Analysis",
        "测量系统调用执行时间检测 Hook", "Measure syscall execution time to detect hooks",
        "春秋高级"
    ),
    CACHE_TIMING(
        "cache_timing", Category.TIMING_SIDE_CHANNEL, 3, false,
        "缓存时序分析", "Cache Timing Analysis",
        "通过缓存时序检测虚拟化环境", "Detect virtualization via cache timing",
        "高级"
    ),
    INTERRUPT_TIMING(
        "interrupt_timing", Category.TIMING_SIDE_CHANNEL, 3, false,
        "中断时序分析", "Interrupt Timing Analysis",
        "测量中断响应时间检测内核修改", "Measure interrupt latency for kernel mods",
        "高级"
    ),
    TEE_CHECK(
        "tee_check", Category.TEE, 3, false,
        "TEE 完整性检测", "TEE Integrity Check",
        "检测 TEE 状态、证书链和密钥证明", "Check TEE state, cert chain, key attestation",
        "春秋"
    ),
    FIRMWARE_HASH(
        "firmware_hash", Category.FIRMWARE, 3, false,
        "固件分区哈希校验", "Firmware Partition Hash Check",
        "校验 boot/vendor/recovery 分区哈希", "Verify boot/vendor/recovery partition hashes",
        "春秋"
    ),
    MEMORY_FINGERPRINT(
        "memory_fingerprint", Category.MEMORY, 3, false,
        "内存指纹深度检测", "Deep Memory Fingerprint",
        "扫描匿名可执行段和代码洞", "Scan anonymous executable segments and code caves",
        "春秋"
    ),
    BINDER_LATENCY(
        "binder_latency", Category.TIMING_SIDE_CHANNEL, 3, false,
        "Binder 延迟分析", "Binder Latency Analysis",
        "通过 Binder 调用延迟检测 Hook", "Detect hooks via Binder call latency",
        "高级"
    ),
    PAGEFAULT_MONITOR(
        "pagefault_monitor", Category.MEMORY, 3, false,
        "缺页异常监控", "Pagefault Monitoring",
        "监控缺页异常模式检测异常", "Monitor pagefault patterns for anomalies",
        "高级"
    ),
    CGROUP_NS_DETECT(
        "cgroup_ns_detect", Category.VIRTUALIZATION, 3, false,
        "Cgroup/命名空间隔离检测", "Cgroup/Namespace Isolation Check",
        "检测容器化或沙盒环境", "Detect containerized or sandbox environments",
        "高级"
    ),
    DEBUG_DETECT_DEEP(
        "debug_detect_deep", Category.DEBUG, 3, true,
        "深度调试检测", "Deep Debug Detection",
        "检测 ptrace、调试器、JDWP 等多种调试方式", "Detect ptrace, debugger, JDWP",
        "通用"
    ),
    EBPF_DETECT(
        "ebpf_detect", Category.KERNEL, 3, false,
        "eBPF 程序检测", "eBPF Program Detection",
        "检测内核加载的 eBPF 程序", "Detect kernel eBPF programs",
        "高级"
    ),

    // ==================== L3 - 取证检测 (Level 4) ====================
    DAEMON_SELF_PROTECT(
        "daemon_self_protect", Category.ADVANCED, 4, false,
        "守护进程自保护", "Daemon Self-Protection",
        "运行独立守护进程进行交叉验证", "Run independent daemon for cross-validation",
        "高级"
    ),
    REPLICATION_CHECK(
        "replication_check", Category.ADVANCED, 4, false,
        "跨进程复制校验", "Cross-Process Replication Check",
        "多进程隔离验证检测结果一致性", "Multi-process isolated verification",
        "高级"
    ),
    AI_ANOMALY(
        "ai_anomaly", Category.ADVANCED, 4, false,
        "AI 异常检测", "AI Anomaly Detection",
        "基于机器学习的异常行为模式分析", "ML-based abnormal behavior analysis",
        "高级"
    ),
    P2P_CONSENSUS(
        "p2p_consensus", Category.ADVANCED, 4, false,
        "P2P 共识校验", "P2P Consensus Verification",
        "通过点对点协议多设备交叉验证", "Cross-device verification via P2P",
        "高级"
    ),
    HARDWARE_PROBE(
        "hardware_probe", Category.ADVANCED, 4, false,
        "硬件探针检测", "Hardware Probe Detection",
        "直接读取硬件寄存器验证设备真实性", "Read hardware registers for authenticity",
        "高级"
    ),
    NETWORK_FINGERPRINT(
        "network_fingerprint", Category.NETWORK, 4, false,
        "网络指纹检测", "Network Fingerprint Detection",
        "TCP/IP 栈指纹、JA3/JA4 TLS 指纹检测", "TCP/IP stack, JA3/JA4 TLS fingerprinting",
        "高级"
    );

    companion object {
        fun byLevel(level: Int): List<DetectionItem> = entries.filter { it.level == level }
        fun byCategory(category: Category): List<DetectionItem> = entries.filter { it.category == category }
        fun byId(id: String): DetectionItem? = entries.firstOrNull { it.id == id }
        fun defaultsForLevel(level: Int): Set<String> =
            entries.filter { it.level <= level && it.defaultEnabled }.map { it.id }.toSet()
    }
}
