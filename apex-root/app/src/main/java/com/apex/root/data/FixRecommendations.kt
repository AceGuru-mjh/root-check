package com.apex.root.data

data class FixRecommendation(
    val id: String,
    val titleZh: String,
    val titleEn: String,
    val descriptionZh: String,
    val descriptionEn: String,
    val stepsZh: List<String>,
    val stepsEn: List<String>,
    val priority: Int,
    val oneClickFixAvailable: Boolean = false
)

object FixRecommendations {
    private val recommendations: Map<String, FixRecommendation> = mapOf(
        "属性" to FixRecommendation(
            id = "properties",
            titleZh = "系统属性异常",
            titleEn = "System Properties Abnormal",
            descriptionZh = "检测到可疑系统属性，可能存在 Root 工具修改了系统属性以隐藏 Root 状态。",
            descriptionEn = "Detected suspicious system properties. Root tools may have modified system properties to hide Root status.",
            stepsZh = listOf(
                "1. 打开终端模拟器或 ADB Shell",
                "2. 执行 getprop 查看所有系统属性",
                "3. 检查 ro.debuggable、ro.secure、ro.build.tags 等关键属性",
                "4. 如发现异常值（如 ro.debuggable=1），说明系统被修改",
                "5. 建议：恢复官方固件或刷入官方 Boot 镜像",
                "6. 使用 fastboot oem unlock 解锁 Bootloader 后重新刷入官方 ROM"
            ),
            stepsEn = listOf(
                "1. Open terminal emulator or ADB Shell",
                "2. Run getprop to view all system properties",
                "3. Check key properties: ro.debuggable, ro.secure, ro.build.tags",
                "4. If abnormal values found (e.g., ro.debuggable=1), system has been modified",
                "5. Recommendation: Restore official firmware or flash official boot image",
                "6. Use fastboot oem unlock to unlock bootloader, then reflash official ROM"
            ),
            priority = 8
        ),
        "文件" to FixRecommendation(
            id = "files",
            titleZh = "Root 相关文件",
            titleEn = "Root-Related Files Detected",
            descriptionZh = "检测到系统中存在 Root 工具相关文件（如 su 二进制文件、Magisk 文件等）。",
            descriptionEn = "Detected Root tool related files in the system (e.g., su binary, Magisk files).",
            stepsZh = listOf(
                "1. 打开文件管理器（需具有 Root 权限查看能力）",
                "2. 检查 /system/bin/su、/system/xbin/su 是否存在",
                "3. 检查 /data/adb/magisk、/cache/magisk.log 等 Magisk 相关文件",
                "4. 检查 /system/app/Superuser.apk 等超级用户管理应用",
                "5. 建议：卸载所有 Root 工具（Magisk、SuperSU 等）",
                "6. 通过 Magisk 的\"完全卸载\"功能，或手动删除相关文件",
                "7. 恢复官方 Boot 镜像并重启设备"
            ),
            stepsEn = listOf(
                "1. Open file manager (with Root permission viewing capability)",
                "2. Check if /system/bin/su, /system/xbin/su exist",
                "3. Check Magisk related files: /data/adb/magisk, /cache/magisk.log",
                "4. Check superuser management apps: /system/app/Superuser.apk",
                "5. Recommendation: Uninstall all Root tools (Magisk, SuperSU, etc.)",
                "6. Use Magisk's 'Uninstall' feature, or manually delete related files",
                "7. Restore official boot image and reboot device"
            ),
            priority = 9
        ),
        "内存" to FixRecommendation(
            id = "memory",
            titleZh = "内存特征异常",
            titleEn = "Memory Fingerprint Abnormal",
            descriptionZh = "在内存中检测到 Root 工具的特征码或运行痕迹，可能存在正在运行的 Root 进程。",
            descriptionEn = "Detected Root tool signatures or running traces in memory. Root processes may be running.",
            stepsZh = listOf(
                "1. 重启设备，这是最简单的清除内存痕迹的方法",
                "2. 重启后不要打开任何 Root 管理工具",
                "3. 安装一个可靠的设备安全扫描应用进行检查",
                "4. 如问题持续存在，检查是否有开机自启动的 Root 工具",
                "5. 进入安全模式（长按电源键 → 长按\"关机\"）",
                "6. 在安全模式下卸载所有可疑应用",
                "7. 如仍无法解决，建议恢复出厂设置"
            ),
            stepsEn = listOf(
                "1. Reboot device - simplest way to clear memory traces",
                "2. After reboot, do not open any Root management tools",
                "3. Install a reliable device security scanner to check",
                "4. If issue persists, check for auto-start Root tools on boot",
                "5. Enter Safe Mode (long press power → long press 'Power off')",
                "6. Uninstall all suspicious apps in Safe Mode",
                "7. If still unresolved, recommend factory reset"
            ),
            priority = 7
        ),
        "挂载" to FixRecommendation(
            id = "mount",
            titleZh = "挂载命名空间异常",
            titleEn = "Mount Namespace Abnormal",
            descriptionZh = "检测到挂载命名空间异常，可能存在进程级 Root 隐藏机制（如 Magisk 的 Mount Namespace 隔离）。",
            descriptionEn = "Detected mount namespace abnormality. Process-level Root hiding mechanism may exist (e.g., Magisk Mount Namespace isolation).",
            stepsZh = listOf(
                "1. 通过 ADB 连接设备",
                "2. 执行 adb shell cat /proc/self/mountinfo 查看挂载信息",
                "3. 对比不同进程的挂载信息是否一致",
                "4. 如发现某些进程的挂载视图不同，说明存在 Namespace 隔离",
                "5. 建议：关闭 Magisk 的\"挂载命名空间\"功能",
                "6. 打开 Magisk 设置 → 关闭\"Mount Namespace Mode\"",
                "7. 如不使用 Magisk，建议恢复官方固件"
            ),
            stepsEn = listOf(
                "1. Connect device via ADB",
                "2. Run adb shell cat /proc/self/mountinfo to view mount info",
                "3. Compare mount info across different processes",
                "4. If mount views differ, Namespace isolation exists",
                "5. Recommendation: Disable Magisk's 'Mount Namespace' feature",
                "6. Open Magisk Settings → Disable 'Mount Namespace Mode'",
                "7. If not using Magisk, recommend restoring official firmware"
            ),
            priority = 8
        ),
        "APatch" to FixRecommendation(
            id = "apatch",
            titleZh = "APatch 特征",
            titleEn = "APatch Signature Detected",
            descriptionZh = "检测到 APatch 特征。APatch 是一种基于内核的 Root 工具，通过修补内核镜像实现 Root，隐蔽性极强。",
            descriptionEn = "Detected APatch signature. APatch is a kernel-based Root tool that achieves Root by patching the kernel image, with extremely high stealth.",
            stepsZh = listOf(
                "1. APatch 通过修改内核实现 Root，普通方法难以检测和清除",
                "2. 检查 /data/adb/ap/ 目录是否存在 APatch 相关文件",
                "3. 检查 boot 镜像是否被修改：adb shell dumpsys bootcontrol",
                "4. 建议：刷入官方内核镜像（boot.img）",
                "5. 从官方 ROM 提取 boot.img 并通过 fastboot 刷入",
                "6. 执行: fastboot flash boot boot.img",
                "7. 如不确定官方镜像来源，建议完整恢复官方固件",
                "8. 清除所有数据并恢复出厂设置"
            ),
            stepsEn = listOf(
                "1. APatch achieves Root via kernel patching, hard to detect/remove by normal means",
                "2. Check if /data/adb/ap/ directory contains APatch related files",
                "3. Check if boot image is modified: adb shell dumpsys bootcontrol",
                "4. Recommendation: Flash official kernel image (boot.img)",
                "5. Extract boot.img from official ROM and flash via fastboot",
                "6. Run: fastboot flash boot boot.img",
                "7. If unsure about official image source, fully restore official firmware",
                "8. Wipe all data and perform factory reset"
            ),
            priority = 10
        ),
        "SELinux" to FixRecommendation(
            id = "selinux",
            titleZh = "SELinux 策略异常",
            titleEn = "SELinux Policy Abnormal",
            descriptionZh = "检测到 SELinux 上下文或策略异常，Root 工具可能修改了 SELinux 策略以允许特权操作。",
            descriptionEn = "Detected SELinux context or policy abnormality. Root tools may have modified SELinux policies to allow privileged operations.",
            stepsZh = listOf(
                "1. 通过 ADB 检查 SELinux 状态: adb shell getenforce",
                "2. 正常应为 Enforcing，如为 Permissive 则被修改",
                "3. 检查 SELinux 策略: adb shell sesearch --allow | grep su",
                "4. 如发现异常策略规则，说明策略被篡改",
                "5. 建议：恢复 SELinux 为 Enforcing 模式",
                "6. 执行: adb shell setenforce 1（临时恢复）",
                "7. 永久修复需刷入官方 boot.img 或恢复出厂设置",
                "8. 检查 /sys/fs/selinux/policy 文件哈希是否与官方一致"
            ),
            stepsEn = listOf(
                "1. Check SELinux status via ADB: adb shell getenforce",
                "2. Should be Enforcing; if Permissive, it has been modified",
                "3. Check SELinux policy: adb shell sesearch --allow | grep su",
                "4. If abnormal policy rules found, policy has been tampered",
                "5. Recommendation: Restore SELinux to Enforcing mode",
                "6. Run: adb shell setenforce 1 (temporary restore)",
                "7. Permanent fix requires flashing official boot.img or factory reset",
                "8. Check /sys/fs/selinux/policy file hash against official version"
            ),
            priority = 8
        ),
        "内核" to FixRecommendation(
            id = "kernel",
            titleZh = "内核异常",
            titleEn = "Kernel Abnormal",
            descriptionZh = "检测到内核存在异常修改痕迹，可能被植入了 Root 相关的内核模块或补丁。",
            descriptionEn = "Detected abnormal kernel modifications. Root-related kernel modules or patches may have been injected.",
            stepsZh = listOf(
                "1. 检查当前内核版本: adb shell uname -a",
                "2. 对比官方内核版本信息，确认是否被修改",
                "3. 检查内核命令行参数: adb shell cat /proc/cmdline",
                "4. 查看已加载的内核模块: adb shell lsmod",
                "5. 建议：刷入官方内核镜像",
                "6. 从设备官方 ROM 中提取 boot.img",
                "7. 通过 fastboot 模式刷入: fastboot flash boot boot.img",
                "8. 重启设备并验证内核版本是否恢复正常",
                "9. 如问题持续，建议完整恢复官方固件"
            ),
            stepsEn = listOf(
                "1. Check current kernel version: adb shell uname -a",
                "2. Compare with official kernel version to confirm modifications",
                "3. Check kernel command line: adb shell cat /proc/cmdline",
                "4. View loaded kernel modules: adb shell lsmod",
                "5. Recommendation: Flash official kernel image",
                "6. Extract boot.img from device's official ROM",
                "7. Flash via fastboot: fastboot flash boot boot.img",
                "8. Reboot and verify kernel version is restored",
                "9. If issue persists, fully restore official firmware"
            ),
            priority = 10
        ),
        "内核模块" to FixRecommendation(
            id = "kernel_modules",
            titleZh = "可疑内核模块",
            titleEn = "Suspicious Kernel Modules",
            descriptionZh = "检测到可疑的内核模块被加载，这些模块可能用于隐藏 Root 状态或提供内核级特权功能。",
            descriptionEn = "Detected suspicious kernel modules loaded. These modules may hide Root status or provide kernel-level privileged functions.",
            stepsZh = listOf(
                "1. 列出所有已加载内核模块: adb shell lsmod",
                "2. 对比官方模块列表，识别可疑模块",
                "3. 检查模块路径: adb shell cat /proc/modules",
                "4. 查看模块签名状态（如支持）",
                "5. 建议：卸载可疑内核模块",
                "6. 执行: adb shell rmmod <模块名>（需要 Root 权限）",
                "7. 检查 /system/lib/modules/ 或 /vendor/lib/modules/ 中的模块文件",
                "8. 删除非官方模块文件",
                "9. 重启设备验证模块是否不再自动加载",
                "10. 如无法手动清除，建议刷入官方固件"
            ),
            stepsEn = listOf(
                "1. List all loaded kernel modules: adb shell lsmod",
                "2. Compare with official module list to identify suspicious ones",
                "3. Check module paths: adb shell cat /proc/modules",
                "4. Check module signature status (if supported)",
                "5. Recommendation: Unload suspicious kernel modules",
                "6. Run: adb shell rmmod <module_name> (requires Root)",
                "7. Check module files in /system/lib/modules/ or /vendor/lib/modules/",
                "8. Delete unofficial module files",
                "9. Reboot to verify modules don't auto-load",
                "10. If cannot manually remove, recommend flashing official firmware"
            ),
            priority = 9
        ),
        "系统调用时序" to FixRecommendation(
            id = "syscall_timing",
            titleZh = "系统调用时序异常",
            titleEn = "Syscall Timing Abnormal",
            descriptionZh = "检测到系统调用执行时序异常，可能存在系统调用 Hook（如 syscall table 被修改），用于拦截和隐藏 Root 相关操作。",
            descriptionEn = "Detected abnormal syscall execution timing. Syscall Hook may exist (e.g., syscall table modified) to intercept and hide Root-related operations.",
            stepsZh = listOf(
                "1. 系统调用时序检测通过测量特定系统调用的执行时间来判断是否存在 Hook",
                "2. 被 Hook 的系统调用执行时间通常明显长于正常值",
                "3. 本检测为 Ring3 侧信道分析，不依赖内核符号表",
                "4. 对比同一系统调用在干净设备与当前设备上的耗时差异",
                "5. 建议：此类型修改通常由内核级 Root 工具造成",
                "6. 刷入官方内核镜像是唯一的可靠修复方法",
                "7. 从官方 ROM 提取 boot.img 并通过 fastboot 刷入",
                "8. 如使用自定义内核，切换到官方内核",
                "9. 恢复后重新运行检测以验证修复效果"
            ),
            stepsEn = listOf(
                "1. Syscall timing detection measures execution time to detect Hooks",
                "2. Hooked syscalls typically take significantly longer than normal",
                "3. This detection is a Ring3 side-channel analysis, no kernel symbols required",
                "4. Compare the same syscall's latency on a clean device vs. the current one",
                "5. Recommendation: This is usually caused by kernel-level Root tools",
                "6. Flashing official kernel image is the only reliable fix",
                "7. Extract boot.img from official ROM and flash via fastboot",
                "8. If using custom kernel, switch to official kernel",
                "9. Re-run detection after restore to verify fix"
            ),
            priority = 10
        ),
        "进程" to FixRecommendation(
            id = "processes",
            titleZh = "可疑进程",
            titleEn = "Suspicious Processes",
            descriptionZh = "检测到与 Root 工具相关的可疑进程正在运行，如 daemon 进程、su 进程等。",
            descriptionEn = "Detected suspicious processes related to Root tools running, such as daemon processes, su processes, etc.",
            stepsZh = listOf(
                "1. 查看所有运行进程: adb shell ps -A",
                "2. 搜索可疑进程名: 关注 su、magisk、daemon、supersu 等关键词",
                "3. 查看进程详情: adb shell cat /proc/<pid>/cmdline",
                "4. 建议：终止可疑 Root 进程",
                "5. 执行: adb shell kill -9 <pid>",
                "6. 检查进程的自启动配置: /data/adb/service.d/",
                "7. 删除可疑的自启动脚本",
                "8. 重启设备后检查进程是否再次出现",
                "9. 如反复出现，建议卸载相关 Root 工具或恢复出厂设置"
            ),
            stepsEn = listOf(
                "1. View all running processes: adb shell ps -A",
                "2. Search for suspicious process names: look for su, magisk, daemon, supersu",
                "3. View process details: adb shell cat /proc/<pid>/cmdline",
                "4. Recommendation: Terminate suspicious Root processes",
                "5. Run: adb shell kill -9 <pid>",
                "6. Check auto-start configs: /data/adb/service.d/",
                "7. Delete suspicious auto-start scripts",
                "8. Reboot and check if processes reappear",
                "9. If recurring, uninstall related Root tools or factory reset"
            ),
            priority = 7
        ),
        "自保护" to FixRecommendation(
            id = "self_protection",
            titleZh = "自保护机制异常",
            titleEn = "Self-Protection Mechanism Abnormal",
            descriptionZh = "检测到应用自保护机制异常，可能存在调试器附加、代码注入或 ptrace 攻击。",
            descriptionEn = "Detected app self-protection mechanism abnormality. Debugger attachment, code injection, or ptrace attacks may exist.",
            stepsZh = listOf(
                "1. 检查是否有调试器附加: adb shell cat /proc/self/status | grep TracerPid",
                "2. TracerPid 不为 0 表示有进程正在调试本应用",
                "3. 检查 /proc/self/maps 中是否有异常的共享库注入",
                "4. 查看是否有 Frida、Xposed 等注入框架运行",
                "5. 建议：关闭所有调试和注入工具",
                "6. 卸载 Frida Server: 检查 /data/local/tmp/ 目录",
                "7. 检查并卸载 Xposed/LSPosed 框架",
                "8. 重启设备以清除所有注入的库",
                "9. 如问题持续，建议恢复出厂设置"
            ),
            stepsEn = listOf(
                "1. Check for debugger attachment: adb shell cat /proc/self/status | grep TracerPid",
                "2. TracerPid non-zero means a process is debugging this app",
                "3. Check /proc/self/maps for abnormal shared library injection",
                "4. Check for injection frameworks like Frida, Xposed",
                "5. Recommendation: Disable all debugging and injection tools",
                "6. Uninstall Frida Server: check /data/local/tmp/ directory",
                "7. Check and uninstall Xposed/LSPosed framework",
                "8. Reboot to clear all injected libraries",
                "9. If issue persists, recommend factory reset"
            ),
            priority = 9
        ),
        "固件" to FixRecommendation(
            id = "firmware_partition",
            titleZh = "固件/分区完整性校验异常",
            titleEn = "Firmware/Partition Integrity Abnormal",
            descriptionZh = "检测到固件分区（Boot/Vendor/Recovery）或安全存储区域被篡改，可能存在 Magisk 修补、APatch 内核修改或第三方 Recovery 植入。",
            descriptionEn = "Detected firmware partition (Boot/Vendor/Recovery) or secure storage tampering. Possible Magisk patching, APatch kernel modification, or third-party Recovery injection.",
            stepsZh = listOf(
                "1. Boot 分区被修改（Magisk 间隙/APatch 内核）→ 刷入官方 boot.img",
                "2. 从官方固件包中提取 boot.img: fastboot flash boot boot.img",
                "3. Vendor 分区异常 → 刷入官方 vendor.img",
                "4. Third-party Recovery 检测 → 刷入官方 recovery.img",
                "5. TEE 安全存储异常 → 检查设备是否已解锁 Bootloader",
                "6. 不确定官方镜像时，建议完整线刷官方 ROM 包",
                "7. 重新锁回 Bootloader（谨慎操作）: fastboot oem lock",
                "8. 恢复后重新运行深度检测验证修复"
            ),
            stepsEn = listOf(
                "1. Boot partition modified (Magisk gap/APatch kernel) → flash official boot.img",
                "2. Extract boot.img from official firmware: fastboot flash boot boot.img",
                "3. Vendor partition abnormal → flash official vendor.img",
                "4. Third-party Recovery detected → flash official recovery.img",
                "5. TEE secure storage abnormal → check if Bootloader is unlocked",
                "6. When unsure, fully flash official ROM package",
                "7. Re-lock Bootloader (use with caution): fastboot oem lock",
                "8. Re-run deep detection after restore to verify fix"
            ),
            priority = 10
        ),
        "固件完整性" to FixRecommendation(
            id = "firmware_generic",
            titleZh = "固件完整性异常",
            titleEn = "Firmware Integrity Abnormal",
            descriptionZh = "检测到固件分区哈希与官方基线不一致，系统固件可能被篡改或替换。",
            descriptionEn = "Detected firmware partition hash mismatch with official baseline. System firmware may have been tampered or replaced.",
            stepsZh = listOf(
                "1. 记录异常分区的哈希值并与官方固件对比",
                "2. 从设备制造商官网获取官方固件包",
                "3. 通过 fastboot 模式刷入对应分区镜像",
                "4. 执行完整线刷以确保所有分区一致性",
                "5. 避免使用非官方的第三方 ROM 或修补版固件",
                "6. 恢复后重新运行深度检测以确认修复"
            ),
            stepsEn = listOf(
                "1. Record abnormal partition hash and compare with official firmware",
                "2. Obtain official firmware package from device manufacturer",
                "3. Flash corresponding partition image via fastboot mode",
                "4. Perform full firmware flash to ensure partition consistency",
                "5. Avoid using unofficial third-party ROMs or patched firmware",
                "6. Re-run deep detection after restore to confirm fix"
            ),
            priority = 9
        ),
        "Shamiko" to FixRecommendation(
            id = "shamiko",
            titleZh = "Shamiko 隐藏工具检测",
            titleEn = "Shamiko Hiding Tool Detected",
            descriptionZh = "检测到 Shamiko 行为特征。Shamiko 是 Zygisk 模块，用于隐藏 Root 状态。",
            descriptionEn = "Detected Shamiko behavior characteristics. Shamiko is a Zygisk module that hides Root status.",
            stepsZh = listOf(
                "1. 检查 /data/adb/modules 目录下是否存在 shamiko 模块",
                "2. 禁用或卸载 Shamiko 模块: Magisk → 模块 → 关闭 Shamiko",
                "3. 检查 /data/adb/shamiko/whitelist 白名单配置",
                "4. Shamiko 通过 sepolicy 覆写隐藏痕迹，需恢复官方 sepolicy",
                "5. 建议: 卸载 Shamiko 后重启设备",
                "6. 重启后重新运行检测以验证是否清除",
                "7. 如问题持续，可能是 Zygisk 残留，需刷入官方 boot.img"
            ),
            stepsEn = listOf(
                "1. Check /data/adb/modules for shamiko module",
                "2. Disable or uninstall Shamiko: Magisk → Modules → Disable Shamiko",
                "3. Check /data/adb/shamiko/whitelist for whitelist config",
                "4. Shamiko uses sepolicy override to hide traces, restore official sepolicy",
                "5. Recommendation: Uninstall Shamiko and reboot device",
                "6. Re-run detection after reboot to verify removal",
                "7. If issue persists, Zygisk residue may remain, flash official boot.img"
            ),
            priority = 10
        ),
        "ZygiskNext" to FixRecommendation(
            id = "zygisknext",
            titleZh = "ZygiskNext 隔离层检测",
            titleEn = "ZygiskNext Isolation Layer Detected",
            descriptionZh = "检测到 ZygiskNext 特征。ZygiskNext 是 Zygisk 的替代实现，通过内存隔离标记提供更强的隐蔽性。",
            descriptionEn = "Detected ZygiskNext characteristics. ZygiskNext is an alternative Zygisk implementation using memory isolation marks.",
            stepsZh = listOf(
                "1. 检查 /data/adb/modules 目录下是否存在 zygisknext 模块",
                "2. 检查 /data/adb/zygisknext 目录及其配置文件",
                "3. ZygiskNext 使用 memfd 私有通道加载模块，需重新打包 boot.img",
                "4. 建议: 在 Magisk 管理中关闭 ZygiskNext 模块",
                "5. 卸载后需重启设备以清除内存中的隔离标记",
                "6. 检查 /proc/self/maps 有无 zygisknext 残留映射",
                "7. 如手动清除困难，建议刷入官方 boot.img"
            ),
            stepsEn = listOf(
                "1. Check /data/adb/modules for zygisknext module",
                "2. Check /data/adb/zygisknext directory and config files",
                "3. ZygiskNext uses memfd private channels, may need boot.img repack",
                "4. Recommendation: Disable ZygiskNext module in Magisk Manager",
                "5. Reboot device after removal to clear memory isolation marks",
                "6. Check /proc/self/maps for zygisknext residual mappings",
                "7. If difficult to remove manually, flash official boot.img"
            ),
            priority = 10
        ),
        "APatch KPM" to FixRecommendation(
            id = "apatch_kpm",
            titleZh = "APatch KPM 内核补丁检测",
            titleEn = "APatch KPM Kernel Patch Detected",
            descriptionZh = "检测到 APatch KPM（Kernel Patch Module）内核补丁。",
            descriptionEn = "Detected APatch KPM (Kernel Patch Module). APatch patches the kernel via KPM modules.",
            stepsZh = listOf(
                "1. 检查 /sys/kpm/ 目录是否存在 KPM 模块节点",
                "2. APatch 的 truncate 后门通过特定参数值触发内核代码执行",
                "3. 建议: 使用 APatch 官方卸载工具移除",
                "4. APatch 可能会清除内核符号表以隐藏痕迹，但本检测不依赖该符号表",
                "5. 恢复方法: 刷入官方 boot.img（需确保不含 APatch 补丁）",
                "6. 执行 fastboot flash boot boot.img",
                "7. 重启后验证 /sys/kpm/ 节点是否消失",
                "8. 如无法清除，建议完整线刷官方 ROM"
            ),
            stepsEn = listOf(
                "1. Check /sys/kpm/ for KPM module nodes",
                "2. APatch truncate backdoor triggers kernel code via specific parameter values",
                "3. Recommendation: Use APatch official uninstaller",
                "4. APatch may erase kernel symbols to hide traces, but this detection does not rely on them",
                "5. Restore by flashing official boot.img (ensure APatch-free)",
                "6. Run fastboot flash boot boot.img",
                "7. Reboot and verify /sys/kpm/ node is gone",
                "8. If cannot remove, full flash official ROM"
            ),
            priority = 10
        ),
        "KernelSU" to FixRecommendation(
            id = "kernelsu_overlay",
            titleZh = "KernelSU overlayfs 伪装检测",
            titleEn = "KernelSU overlayfs Masquerading Detected",
            descriptionZh = "检测到 KernelSU 通过 overlayfs 伪装系统分区。",
            descriptionEn = "Detected KernelSU overlayfs masquerading on system partitions.",
            stepsZh = listOf(
                "1. 检查 /data/adb/ksu 目录是否存在 KernelSU 相关文件",
                "2. 检查 overlay 挂载中是否包含 ksu 关键词",
                "3. KernelSU 使用 overlay 镜像文件（/data/adb/modules/*/img）",
                "4. 建议: 使用 KernelSU 管理器卸载 Root",
                "5. 在 KernelSU 管理中关闭所有模块",
                "6. 执行 KernelSU 完全卸载（官方卸载脚本）",
                "7. 检查 /proc/self/mountinfo 中的 overlay 条目",
                "8. 恢复官方 boot.img 以确保内核 overlay 未被劫持"
            ),
            stepsEn = listOf(
                "1. Check /data/adb/ksu for KernelSU related files",
                "2. Check overlay mounts for ksu keywords",
                "3. KernelSU uses overlay image files (/data/adb/modules/*/img)",
                "4. Recommendation: Uninstall Root via KernelSU Manager",
                "5. Disable all modules in KernelSU Manager",
                "6. Run KernelSU complete uninstall (official script)",
                "7. Check overlay entries in /proc/self/mountinfo",
                "8. Restore official boot.img to prevent kernel overlay hijacking"
            ),
            priority = 10
        ),
        "LSPosed" to FixRecommendation(
            id = "lsposed",
            titleZh = "LSPosed/Riru 框架检测",
            titleEn = "LSPosed/Riru Framework Detected",
            descriptionZh = "检测到 LSPosed（旧版 Riru 或新版 Zygisk 模式）框架。",
            descriptionEn = "Detected LSPosed framework (legacy Riru or new Zygisk mode).",
            stepsZh = listOf(
                "1. 检查 /data/adb/lspd 目录是否存在 LSPosed 配置文件",
                "2. 检查 /data/adb/modules 中 lsposed 模块",
                "3. 区分 Riru 模式（旧版）与 Zygisk 模式（新版）",
                "4. 旧版 Riru: 检查 libriru_* 库是否在内存映射中",
                "5. 新版: 在 Magisk 管理中关闭 LSPosed 模块",
                "6. 检查 /proc/self/maps 中 liblspd.so 和 binder 通道",
                "7. 建议: 在 LSPosed 管理器中卸载所有模块",
                "8. 在 Magisk 中禁用/卸载 LSPosed 模块后重启"
            ),
            stepsEn = listOf(
                "1. Check /data/adb/lspd for LSPosed config files",
                "2. Check /data/adb/modules for lsposed module",
                "3. Distinguish Riru mode (legacy) from Zygisk mode (new)",
                "4. Legacy Riru: check libriru_* libraries in memory maps",
                "5. New Zygisk mode: disable LSPosed module in Magisk Manager",
                "6. Check /proc/self/maps for liblspd.so and binder channels",
                "7. Recommendation: Uninstall all modules in LSPosed Manager",
                "8. Disable/uninstall LSPosed module in Magisk and reboot"
            ),
            priority = 9
        )
    )

    fun getRecommendation(layer: String): FixRecommendation? {
        // Sort keys by length descending to match most specific first
        // This prevents "固件" matching before "固件完整性"
        val sortedKeys = recommendations.keys.sortedByDescending { it.length }
        val matchedKey = sortedKeys.firstOrNull { key ->
            when (key) {
                "系统调用时序" -> layer == "系统调用时序" || layer == "系统调用"
                "APatch KPM" -> layer.contains("APatch") && layer.contains("KPM")
                "固件完整性" -> layer.contains("固件") && (layer.contains("完整") || layer.contains("分区"))
                else -> layer == key || (layer.contains(key) && key.length >= 2)
            }
        }
        return matchedKey?.let { recommendations[it] }
    }

    fun getAllRecommendations(): Collection<FixRecommendation> {
        return recommendations.values
    }

    fun getRecommendationsForLayers(layers: List<String>): List<FixRecommendation> {
        return layers.mapNotNull { getRecommendation(it) }
            .sortedByDescending { it.priority }
            .distinctBy { it.id }
    }
}