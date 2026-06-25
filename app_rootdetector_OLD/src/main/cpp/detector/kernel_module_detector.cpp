#include "kernel_module_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "KernelModuleDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// ==================== 官方内核模块白名单 ====================
std::vector<std::string> KernelModuleDetector::getOfficialModuleWhitelist() {
    return {
        // 通用内核模块
        "kernel", "vmlinux",
        // 虚拟化相关（模拟器/虚拟机环境）
        "vboxsf", "vboxguest", "vboxvideo", "vmw_vsock_vmci_transport",
        "vmw_vmci", "vmwgfx", "vmw_balloon", "vmw_vmmemctl",
        // 常见硬件驱动（Android 设备常见）
        "gpu", "mali", "adreno", "kgsl", "ion", "dma_heap",
        "wlan", "wifi", "bluetooth", "btusb", "hci_uart",
        "usbcore", "xhci_hcd", "ehci_hcd", "ohci_hcd",
        "mmc_core", "mmc_block", "sdhci",
        "ext4", "f2fs", "fuse", "exfat", "vfat", "fat32",
        "ntfs", "ntfs3",
        // 网络相关
        "tcp_cubic", "tcp_bbr", "iptable_filter", "ip_tables",
        "nf_conntrack", "nf_nat", "xfrm_user", "xfrm_algo",
        // 内核基础模块
        "cfg80211", "mac80211", "rfkill",
        "input_core", "evdev", "uinput",
        "hid", "usbhid",
        "scsi_mod", "sd_mod", "sr_mod",
        "loop", "dm_mod", "dm_crypt",
        "zram", "zsmalloc",
        // Android 特有模块
        "binder", "binder_linux",
        "ashmem", "ashmem_linux",
        "dm_verity",
        // GPU 驱动
        "pvr", "powervr", "nvgpu", "tegra_gpu",
        // 常见 SoC 模块
        "exynos", "mtk", "mediatek", "qcom", "msm", "msm_thermal",
        // 加密模块
        "aes", "aes_generic", "aes_arm", "sha1", "sha256", "sha512",
        "crc32", "crc32c", "michael_mic", "ccm", "ecb", "cbc",
        // 其他常见模块
        "tun", "ppp_generic", "slhc",
        "cpufreq", "cpufreq_interactive", "cpufreq_powersave",
        "thermal", "thermal_sys",
        "regulator", "clk", "pinctrl",
        "leds", "led_class",
        "rtc", "rtc_class",
        "i2c_core", "spi",
        "gpio", "gpiolib",
        "watchdog",
        "nvmem",
        "phy",
        "hwmon",
        // Trusty TEE
        "trusty", "trusty_ipc", "trusty_log", "trusty_test",
        // 常见驱动
        "goodix", "synaptics", "focaltech",
        "sensor", "sensors",
        "camera", "cam_sensor",
        "soundcore", "snd",
        "v4l2", "videobuf",
    };
}

// ==================== 可疑模块关键词 ====================
std::vector<std::string> KernelModuleDetector::getSuspiciousKeywords() {
    return {
        // Root 工具
        "magisk", "magiskhide", "magiskboot",
        "kernelsu", "ksu", "ksud",
        "apatch", "apatchd",
        "supersu", "superuser",
        // Hook 框架
        "xposed", "lsposed", "edxposed",
        "frida", "substrate",
        "riru", "zygisk",
        // 隐藏工具
        "shamiko", "hide_my_applist",
        "lspatch",
        // 内核 Hook 工具
        "khook", "hook", "inline_hook",
        "syscall_hook", "syscall_table",
        // 调试/注入工具
        "inject", "ptrace", "injector",
        // 已知 Root 内核模块
        "su", "superuser", "daemonsu",
        // 其他可疑关键词
        "rootkit", "backdoor", "exploit",
        "kernel_patch", "kpatch",
        "kprobe_hook", "ftrace_hook",
    };
}

// ==================== 可疑内核符号模式 ====================
std::vector<std::string> KernelModuleDetector::getSuspiciousSymbolPatterns() {
    return {
        // Root 工具相关符号
        "magisk", "ksu_", "kernelsu", "apatch",
        "su_daemon", "superuser",
        // Hook 相关符号
        "hook_syscall", "hook_memory", "hook_function",
        "inline_hook", "khook",
        "syscall_table_modify", "modify_syscall",
        // 内核 Patch 相关
        "kernel_patch", "kpatch", "patch_kernel",
        "bypass", "hide_process", "hide_file",
        // 内存注入相关
        "inject_code", "inject_module",
        "write_kernel_memory", "read_kernel_memory",
        // 异常导出
        "kallsyms_lookup_name",  // 正常内核不应导出此符号
    };
}

// ==================== 检查模块是否在白名单中 ====================
bool KernelModuleDetector::isModuleWhitelisted(const std::string& moduleName) {
    auto whitelist = getOfficialModuleWhitelist();
    std::string lowerName = moduleName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    for (const auto& official : whitelist) {
        std::string lowerOfficial = official;
        std::transform(lowerOfficial.begin(), lowerOfficial.end(), lowerOfficial.begin(), ::tolower);
        if (lowerName.find(lowerOfficial) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ==================== 检查模块名是否包含可疑关键词 ====================
bool KernelModuleDetector::containsSuspiciousKeyword(const std::string& moduleName, std::string& matchedKeyword) {
    auto keywords = getSuspiciousKeywords();
    std::string lowerName = moduleName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    for (const auto& keyword : keywords) {
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
        if (lowerName.find(lowerKeyword) != std::string::npos) {
            matchedKeyword = keyword;
            return true;
        }
    }
    return false;
}

// ==================== 扫描 /proc/modules ====================
std::vector<KernelModuleInfo> KernelModuleDetector::scanProcModules() {
    std::vector<KernelModuleInfo> modules;

    // 优先使用 SyscallUtils 直接读取，避免 Hook
    std::string content = SyscallUtils::readFileDirect("/proc/modules");

    if (content.empty()) {
        // 回退到标准文件读取
        std::ifstream modulesFile("/proc/modules");
        if (!modulesFile.is_open()) {
            LOGW("无法打开 /proc/modules");
            return modules;
        }

        std::stringstream buffer;
        buffer << modulesFile.rdbuf();
        content = buffer.str();
    }

    std::istringstream stream(content);
    std::string line;

    // /proc/modules 格式: name size refcount dependencies state address
    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        KernelModuleInfo info;
        info.source = "proc";

        std::istringstream lineStream(line);
        std::string deps;

        if (lineStream >> info.name >> info.size >> info.refCount >> deps >> info.state >> info.address) {
            // 移除末尾逗号（依赖项）
            if (!deps.empty() && deps.back() == ',') {
                deps.pop_back();
            }
            modules.push_back(info);
        }
    }

    LOGI("从 /proc/modules 扫描到 %zu 个内核模块", modules.size());
    return modules;
}

// ==================== 扫描 /sys/module ====================
std::vector<std::string> KernelModuleDetector::scanSysModules() {
    std::vector<std::string> modules;

    // 使用 SyscallUtils 直接列出目录
    modules = SyscallUtils::listFilesDirect("/sys/module");

    if (modules.empty()) {
        LOGW("/sys/module 目录为空或无法访问");
    } else {
        LOGI("从 /sys/module 扫描到 %zu 个模块目录", modules.size());
    }

    return modules;
}

// ==================== 检测非官方内核模块 ====================
std::vector<KernelModuleInfo> KernelModuleDetector::detectUnofficialModules(
    const std::vector<KernelModuleInfo>& modules) {

    std::vector<KernelModuleInfo> suspicious;

    for (const auto& mod : modules) {
        std::string matchedKeyword;
        bool hasKeyword = containsSuspiciousKeyword(mod.name, matchedKeyword);
        bool whitelisted = isModuleWhitelisted(mod.name);

        if (hasKeyword) {
            // 包含可疑关键词，直接标记
            LOGW("检测到可疑内核模块: %s (匹配关键词: %s)", mod.name.c_str(), matchedKeyword.c_str());
            suspicious.push_back(mod);
        } else if (!whitelisted) {
            // 不在白名单中，标记为可疑
            LOGW("检测到非白名单内核模块: %s (大小: %s, 状态: %s)",
                 mod.name.c_str(), mod.size.c_str(), mod.state.c_str());
            suspicious.push_back(mod);
        }
    }

    return suspicious;
}

// ==================== 检查 kallsyms_lookup_name 是否被导出 ====================
bool KernelModuleDetector::checkKallsymsLookupNameExported() {
    // 优先使用 SyscallUtils 读取
    std::string content = SyscallUtils::readFileDirect("/proc/kallsyms");

    if (content.empty()) {
        // 回退到标准文件读取
        std::ifstream kallsyms("/proc/kallsyms");
        if (!kallsyms.is_open()) {
            LOGW("无法打开 /proc/kallsyms");
            return false;
        }

        std::stringstream buffer;
        buffer << kallsyms.rdbuf();
        content = buffer.str();
    }

    std::istringstream stream(content);
    std::string line;

    // /proc/kallsyms 格式: address type name [module]
    // 正常内核中 kallsyms_lookup_name 应该是小写 't'（局部符号）
    // 如果被导出为大写 'T'（全局符号），说明内核被修改
    while (std::getline(stream, line)) {
        if (line.find("kallsyms_lookup_name") != std::string::npos) {
            // 检查符号类型
            // 格式: ffffffff81234567 T kallsyms_lookup_name
            std::istringstream lineStream(line);
            std::string addr, type, name;
            if (lineStream >> addr >> type >> name) {
                if (type == "T") {
                    LOGW("检测到 kallsyms_lookup_name 被导出为全局符号 (T): %s", line.c_str());
                    return true;
                } else if (type == "t") {
                    LOGI("kallsyms_lookup_name 存在但为局部符号 (t)，正常");
                    return false;
                }
            }
        }
    }

    // 如果完全找不到该符号，可能是被隐藏了
    LOGI("未在 /proc/kallsyms 中找到 kallsyms_lookup_name");
    return false;
}

// ==================== 检查系统调用表是否被修改 ====================
bool KernelModuleDetector::checkSyscallTableModified() {
    // 检查 /proc/kallsyms 中 sys_call_table 的地址
    // 如果被 Hook，地址可能指向非内核代码段
    std::string content = SyscallUtils::readFileDirect("/proc/kallsyms");

    if (content.empty()) {
        std::ifstream kallsyms("/proc/kallsyms");
        if (!kallsyms.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << kallsyms.rdbuf();
        content = buffer.str();
    }

    std::istringstream stream(content);
    std::string line;

    unsigned long long sysCallTableAddr = 0;
    unsigned long long kernelTextStart = 0;
    unsigned long long kernelTextEnd = 0;

    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string addr, type, name;
        if (lineStream >> addr >> type >> name) {
            unsigned long long address = 0;
            std::stringstream ss;
            ss << std::hex << addr;
            ss >> address;

            if (name == "_stext" && (type == "T" || type == "t")) {
                kernelTextStart = address;
            } else if (name == "_etext" && (type == "T" || type == "t")) {
                kernelTextEnd = address;
            } else if (name == "sys_call_table" || name == "__sys_call_table") {
                sysCallTableAddr = address;
            }
        }
    }

    // 检查 sys_call_table 是否在内核代码段范围内
    if (sysCallTableAddr != 0 && kernelTextStart != 0 && kernelTextEnd != 0) {
        // sys_call_table 通常在内核数据段，但不应偏离太远
        // 如果地址为 0 或者明显不在合理范围，可能被 Hook
        if (sysCallTableAddr < kernelTextStart || sysCallTableAddr > kernelTextEnd * 2) {
            LOGW("检测到 sys_call_table 地址异常: 0x%llx (内核段: 0x%llx - 0x%llx)",
                 sysCallTableAddr, kernelTextStart, kernelTextEnd);
            return true;
        }
    }

    // 检查是否有多个 sys_call_table 符号（可能是 Hook 后伪造的）
    int syscallTableCount = 0;
    std::istringstream stream2(content);
    while (std::getline(stream2, line)) {
        if (line.find("sys_call_table") != std::string::npos) {
            syscallTableCount++;
        }
    }

    if (syscallTableCount > 2) {
        LOGW("检测到多个 sys_call_table 符号: %d 个（可能被 Hook）", syscallTableCount);
        return true;
    }

    return false;
}

// ==================== 扫描 /proc/kallsyms 查找异常符号 ====================
void KernelModuleDetector::scanKallsymsForSuspiciousSymbols(
    std::vector<KernelSymbolInfo>& suspiciousSymbols) {

    std::string content = SyscallUtils::readFileDirect("/proc/kallsyms");

    if (content.empty()) {
        std::ifstream kallsyms("/proc/kallsyms");
        if (!kallsyms.is_open()) {
            return;
        }

        std::stringstream buffer;
        buffer << kallsyms.rdbuf();
        content = buffer.str();
    }

    auto patterns = getSuspiciousSymbolPatterns();
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        for (const auto& pattern : patterns) {
            std::string lowerPattern = pattern;
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);

            if (lowerLine.find(lowerPattern) != std::string::npos) {
                KernelSymbolInfo sym;
                std::istringstream lineStream(line);
                if (lineStream >> sym.address >> sym.type >> sym.name) {
                    sym.reason = "匹配可疑模式: " + pattern;

                    // 对于 kallsyms_lookup_name，只有全局导出才算异常
                    if (pattern == "kallsyms_lookup_name" && sym.type != "T") {
                        continue;
                    }

                    suspiciousSymbols.push_back(sym);
                    LOGW("发现可疑内核符号: %s %s %s (%s)",
                         sym.address.c_str(), sym.type.c_str(), sym.name.c_str(), sym.reason.c_str());
                }
                break; // 一行只匹配一次
            }
        }
    }
}

// ==================== 检测内核符号表异常 ====================
bool KernelModuleDetector::detectSymbolTableAnomaly(std::vector<KernelSymbolInfo>& suspiciousSymbols) {
    bool anomaly = false;

    // 检查 kallsyms_lookup_name 是否被导出
    if (checkKallsymsLookupNameExported()) {
        anomaly = true;
        KernelSymbolInfo sym;
        sym.name = "kallsyms_lookup_name";
        sym.type = "T";
        sym.reason = "kallsyms_lookup_name 被导出为全局符号，正常内核不应导出";
        suspiciousSymbols.push_back(sym);
    }

    // 检查系统调用表是否被修改
    if (checkSyscallTableModified()) {
        anomaly = true;
        KernelSymbolInfo sym;
        sym.name = "sys_call_table";
        sym.type = "?";
        sym.reason = "系统调用表地址异常或存在多个符号";
        suspiciousSymbols.push_back(sym);
    }

    // 扫描可疑符号
    std::vector<KernelSymbolInfo> kallsymsSuspicious;
    scanKallsymsForSuspiciousSymbols(kallsymsSuspicious);
    if (!kallsymsSuspicious.empty()) {
        anomaly = true;
        suspiciousSymbols.insert(suspiciousSymbols.end(),
                                 kallsymsSuspicious.begin(),
                                 kallsymsSuspicious.end());
    }

    return anomaly;
}

// ==================== 检测内核内存中的异常代码段 ====================
bool KernelModuleDetector::detectMemoryAnomaly(std::vector<std::string>& findings) {
    bool anomaly = false;

    std::string content = SyscallUtils::readFileDirect("/proc/kallsyms");

    if (content.empty()) {
        std::ifstream kallsyms("/proc/kallsyms");
        if (!kallsyms.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << kallsyms.rdbuf();
        content = buffer.str();
    }

    // 检查是否存在可疑的内核代码段特征
    std::istringstream stream(content);
    std::string line;

    int suspiciousTextCount = 0;
    bool foundHiddenModule = false;

    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string addr, type, name;
        if (!(lineStream >> addr >> type >> name)) continue;

        // 检查 1: 查找属于未知模块的可执行代码段
        // 模块代码段通常标记为 [module_name]
        if (type == "T" || type == "t") {
            // 检查符号名中是否包含可疑的模块标记
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            // 检查是否有隐藏模块的特征
            // 正常模块的符号会带有 [module_name] 标记
            // 隐藏模块可能没有或者使用伪造的标记
            if (line.find("[") != std::string::npos) {
                // 提取模块名
                size_t start = line.find('[');
                size_t end = line.find(']');
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    std::string modName = line.substr(start + 1, end - start - 1);
                    std::string matchedKeyword;
                    if (containsSuspiciousKeyword(modName, matchedKeyword)) {
                        findings.push_back("发现可疑模块代码段: " + modName + " (符号: " + name + ")");
                        foundHiddenModule = true;
                    }
                }
            }
        }

        // 检查 2: 大量连续的 T 类型符号可能表示注入的代码段
        if (type == "T") {
            suspiciousTextCount++;
        }
    }

    if (foundHiddenModule) {
        anomaly = true;
    }

    // 检查 /sys/module 中的模块是否与 /proc/modules 一致
    auto procModules = scanProcModules();
    auto sysModules = scanSysModules();

    if (!procModules.empty() && !sysModules.empty()) {
        // 找出在 /sys/module 中存在但 /proc/modules 中不存在的模块
        // 这可能是被隐藏的模块
        for (const auto& sysMod : sysModules) {
            bool foundInProc = false;
            for (const auto& procMod : procModules) {
                if (sysMod == procMod.name) {
                    foundInProc = true;
                    break;
                }
            }

            // 某些模块只在 /sys/module 中存在是正常的（内置模块）
            // 但如果模块有 refcnt 属性，说明它是动态加载的
            std::string refcntPath = "/sys/module/" + sysMod + "/refcnt";
            std::string refcnt = SyscallUtils::readFileDirect(refcntPath);
            if (!refcnt.empty() && !foundInProc) {
                // 动态加载模块但不在 /proc/modules 中，可能被隐藏
                std::string matchedKeyword;
                if (containsSuspiciousKeyword(sysMod, matchedKeyword)) {
                    findings.push_back("发现隐藏模块: " + sysMod + " (在 /sys/module 中存在但 /proc/modules 中隐藏)");
                    anomaly = true;
                }
            }
        }
    }

    return anomaly;
}

// ==================== 详细检测接口 ====================
KernelModuleDetectionDetail KernelModuleDetector::detectDetailed() {
    KernelModuleDetectionDetail detail;
    detail.hasUnofficialModules = false;
    detail.hasSymbolAnomaly = false;
    detail.hasMemoryAnomaly = false;
    detail.hasKallsymsExported = false;
    detail.hasSyscallTableHook = false;

    // 阶段 1: 扫描 /proc/modules
    LOGI("=== 阶段 1: 扫描 /proc/modules ===");
    detail.allModules = scanProcModules();

    // 阶段 2: 扫描 /sys/module
    LOGI("=== 阶段 2: 扫描 /sys/module ===");
    auto sysModules = scanSysModules();

    // 阶段 3: 检测非官方模块
    LOGI("=== 阶段 3: 检测非官方内核模块 ===");
    detail.suspiciousModules = detectUnofficialModules(detail.allModules);
    if (!detail.suspiciousModules.empty()) {
        detail.hasUnofficialModules = true;
        detail.findings.push_back("发现 " + std::to_string(detail.suspiciousModules.size()) + " 个非官方内核模块");
        for (const auto& mod : detail.suspiciousModules) {
            detail.findings.push_back("  - " + mod.name + " (大小: " + mod.size + ", 状态: " + mod.state + ")");
        }
    }

    // 阶段 4: 检测内核符号表异常
    LOGI("=== 阶段 4: 检测内核符号表异常 ===");
    detail.hasSymbolAnomaly = detectSymbolTableAnomaly(detail.suspiciousSymbols);
    if (detail.hasSymbolAnomaly) {
        detail.findings.push_back("检测到 " + std::to_string(detail.suspiciousSymbols.size()) + " 个可疑内核符号");
        for (const auto& sym : detail.suspiciousSymbols) {
            detail.findings.push_back("  - " + sym.name + " (" + sym.reason + ")");
            if (sym.name == "kallsyms_lookup_name") {
                detail.hasKallsymsExported = true;
            }
            if (sym.name == "sys_call_table") {
                detail.hasSyscallTableHook = true;
            }
        }
    }

    // 阶段 5: 检测内核内存异常
    LOGI("=== 阶段 5: 检测内核内存异常代码段 ===");
    std::vector<std::string> memoryFindings;
    detail.hasMemoryAnomaly = detectMemoryAnomaly(memoryFindings);
    if (detail.hasMemoryAnomaly) {
        detail.findings.push_back("检测到内核内存异常");
        detail.findings.insert(detail.findings.end(), memoryFindings.begin(), memoryFindings.end());
    }

    return detail;
}

// ==================== 标准检测接口 ====================
DetectionResult KernelModuleDetector::detect() {
    DetectionResult result;
    result.layer = "第6层：内核模块检测";
    result.detected = false;

    auto detail = detectDetailed();

    if (detail.hasUnofficialModules || detail.hasSymbolAnomaly || detail.hasMemoryAnomaly) {
        result.detected = true;

        // 构建简洁的检测结果
        std::vector<std::string> summary;
        if (detail.hasUnofficialModules) {
            summary.push_back("非官方内核模块 (" + std::to_string(detail.suspiciousModules.size()) + " 个)");
        }
        if (detail.hasKallsymsExported) {
            summary.push_back("kallsyms_lookup_name 被导出");
        }
        if (detail.hasSyscallTableHook) {
            summary.push_back("系统调用表异常");
        }
        if (detail.hasMemoryAnomaly) {
            summary.push_back("内核内存异常");
        }

        result.detail = "内核模块异常: ";
        for (size_t i = 0; i < summary.size(); ++i) {
            result.detail += summary[i];
            if (i < summary.size() - 1) {
                result.detail += "; ";
            }
        }
    } else {
        result.detail = "未发现内核模块异常 (已扫描 " +
                        std::to_string(detail.allModules.size()) + " 个模块)";
    }

    return result;
}
