#include "shamiko_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "ShamikoDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

std::string ShamikoDetector::readFileContent(const std::string& path) {
    std::string content = SyscallUtils::readFileDirect(path);
    if (!content.empty()) return content;
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<std::string> ShamikoDetector::listDirectory(const std::string& path) {
    return SyscallUtils::listFilesDirect(path);
}

SecontextSnapshot ShamikoDetector::captureSecontext(const std::string& tag) {
    SecontextSnapshot snap;
    snap.tag = tag;
    snap.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    snap.context = readFileContent("/proc/self/attr/current");
    if (!snap.context.empty()) {
        snap.context.erase(snap.context.find_last_not_of(" \n\r\t") + 1);
    }
    return snap;
}

bool ShamikoDetector::detectZygiskContextTiming(std::vector<SecontextSnapshot>& history) {
    history.push_back(captureSecontext("phase1_enter"));
    history.push_back(captureSecontext("phase1_exit"));

    for (int i = 0; i < 5; i++) {
        std::string phase = "phase2_iter_" + std::to_string(i);
        history.push_back(captureSecontext(phase));
        usleep(500);
    }
    history.push_back(captureSecontext("phase2_final"));

    for (size_t i = 1; i < history.size(); i++) {
        const auto& prev = history[i - 1];
        const auto& curr = history[i];
        if (prev.context != curr.context) {
            long long delta = curr.timestamp_ms - prev.timestamp_ms;
            if (delta < 100) {
                LOGW("Zygisk 上下文快速跳变: %s -> %s (%lldms)",
                     prev.context.c_str(), curr.context.c_str(), delta);
                return true;
            }
        }
        if (curr.context.find("zygisk") != std::string::npos ||
            curr.context.find("magisk") != std::string::npos ||
            curr.context.find("shamiko") != std::string::npos) {
            LOGW("检测到 Shamiko/Zygisk 上下文: %s (tag: %s)",
                 curr.context.c_str(), curr.tag.c_str());
            return true;
        }
    }

    return false;
}

bool ShamikoDetector::detectSepolicyOverlay() {
    if (SyscallUtils::fileExistsDirect("/data/system/sepolicy")) {
        LOGW("检测到 sepolicy 临时加载文件: /data/system/sepolicy");
        LOGI("Shamiko 常通过临时加载 sepolicy 覆写 SELinux 策略");
        return true;
    }

    std::string sepolicy = readFileContent("/sys/fs/selinux/policy");
    std::string original = readFileContent("/sepolicy");

    if (!sepolicy.empty() && !original.empty()) {
        if (sepolicy.size() != original.size() || sepolicy != original) {
            LOGW("检测到 sepolicy 策略差异（运行时 vs 原始），可能被 shamiko 篡改");
            return true;
        }
    }

    std::string loadFile = readFileContent("/sys/fs/selinux/load");
    if (!loadFile.empty() && loadFile.find("shamiko") != std::string::npos) {
        return true;
    }

    return false;
}

bool ShamikoDetector::detectSelinuxDynamicOverride() {
    std::string current = readFileContent("/proc/self/attr/current");
    if (current.empty()) return false;

    std::string expected = "u:r:untrusted_app:s0";
    std::string lower = current;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("zygote") != std::string::npos ||
        lower.find("init") != std::string::npos ||
        lower.find("system") != std::string::npos) {
        if (getuid() >= 10000) {
            LOGW("非特权进程(%d)但上下文为特权域: %s，可能存在 SELinux 动态覆写",
                 getuid(), current.c_str());
            return true;
        }
    }

    std::ifstream contextList("/proc/self/attr/prev");
    if (contextList.is_open()) {
        std::string prev;
        while (std::getline(contextList, prev)) {
            std::string lowerPrev = prev;
            std::transform(lowerPrev.begin(), lowerPrev.end(), lowerPrev.begin(), ::tolower);
            if (lowerPrev.find("shamiko") != std::string::npos ||
                lowerPrev.find("zygisk") != std::string::npos) {
                LOGW("发现 SELinux 上下文历史中存在隐藏工具痕迹: %s", prev.c_str());
                return true;
            }
        }
    }

    std::string enforce = readFileContent("/sys/fs/selinux/enforce");
    if (!enforce.empty()) {
        int val = 0;
        try { val = std::stoi(enforce); } catch (...) {}
        if (val == 0) {
            std::string checkContext = readFileContent("/proc/self/attr/current");
            if (checkContext.find("untrusted_app") != std::string::npos) {
                LOGW("SELinux permissive 模式下 untrusted_app 仍可执行，可能存在覆写");
                return true;
            }
        }
    }

    return false;
}

bool ShamikoDetector::detectMountFilterRules(std::vector<MountFilterRule>& rules) {
    std::string mounts = readFileContent("/proc/self/mountinfo");
    if (mounts.empty()) return false;

    std::istringstream stream(mounts);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        MountFilterRule rule;
        std::vector<std::string> parts;
        std::istringstream ls(line);
        std::string token;
        while (ls >> token) parts.push_back(token);

        if (parts.size() >= 10) {
            rule.source = parts[3];
            rule.target = parts[4];
            rule.fstype = parts[7];
            rule.options = parts[9];

            if (rule.target.find("/data/adb") != std::string::npos &&
                rule.options.find("ro") != std::string::npos) {
                rule.is_shamiko_style = true;
                rules.push_back(rule);
                LOGW("Shamiko 风格挂载过滤规则: %s -> %s (ro)",
                     rule.source.c_str(), rule.target.c_str());
                found = true;
            }

            if (rule.source.find("shamiko") != std::string::npos ||
                rule.source.find(".shamiko") != std::string::npos) {
                rule.is_shamiko_style = true;
                rules.push_back(rule);
                found = true;
            }
        }
    }

    if (!found) {
        std::ifstream selfMounts("/proc/self/mounts");
        if (selfMounts.is_open()) {
            std::string ml;
            while (std::getline(selfMounts, ml)) {
                if (ml.find("/data/adb/modules") != std::string::npos &&
                    ml.find("tmpfs") != std::string::npos) {
                    MountFilterRule r;
                    r.target = ml;
                    r.is_shamiko_style = true;
                    rules.push_back(r);
                    found = true;
                }
            }
        }
    }

    return found;
}

bool ShamikoDetector::detectProcFilterWhitelist(std::vector<ProcFilterEntry>& entries) {
    auto procDirs = listDirectory("/proc");
    bool found = false;

    for (const auto& dir : procDirs) {
        int pid = 0;
        try { pid = std::stoi(dir); } catch (...) { continue; }
        if (pid <= 0 || pid == getpid()) continue;

        std::string cmdline = readFileContent("/proc/" + dir + "/cmdline");
        if (cmdline.empty()) continue;
        if (!cmdline.empty() && cmdline.back() != '\0')
            cmdline += '\0';
        std::string procName = cmdline.substr(0, cmdline.find('\0'));

        std::string secontext = readFileContent("/proc/" + dir + "/attr/current");
        if (!secontext.empty()) {
            secontext.erase(secontext.find_last_not_of(" \n\r\t") + 1);
        }

        std::string status = readFileContent("/proc/" + dir + "/status");
        bool tracerVisible = status.find("TracerPid:\t0") == std::string::npos;

        ProcFilterEntry entry;
        entry.pid = pid;
        entry.name = procName;
        entry.secontext = secontext;
        entry.is_whitelisted = false;

        if (secontext.find("shamiko") != std::string::npos) {
            entry.is_whitelisted = true;
            entries.push_back(entry);
            LOGW("Shamiko 进程白名单: pid=%d name=%s ctx=%s",
                 pid, procName.c_str(), secontext.c_str());
            found = true;
        }

        if (!tracerVisible && pid < 1000 && procName.find("shamiko") != std::string::npos) {
            entry.is_whitelisted = true;
            entries.push_back(entry);
            found = true;
        }
    }

    std::string maps = readFileContent("/proc/self/maps");
    if (maps.find("shamiko_whitelist") != std::string::npos ||
        maps.find("shamiko_whitelist") != std::string::npos) {
        LOGW("内存映射中存在 shamiko 白名单特征");
        found = true;
    }

    return found;
}

bool ShamikoDetector::detectShamikoModule() {
    auto modules = listDirectory("/data/adb/modules");
    for (const auto& mod : modules) {
        std::string lower = mod;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("shamiko") != std::string::npos) {
            LOGW("检测到 Shamiko 模块: %s", mod.c_str());
            return true;
        }
        std::string disableFile = "/data/adb/modules/" + mod + "/disable";
        if (SyscallUtils::fileExistsDirect(disableFile)) continue;
        std::string updateJson = readFileContent("/data/adb/modules/" + mod + "/update.json");
        if (updateJson.find("shamiko") != std::string::npos ||
            updateJson.find("Shamiko") != std::string::npos) {
            LOGW("模块 %s 的 update.json 包含 shamiko 引用", mod.c_str());
            return true;
        }
    }

    if (SyscallUtils::fileExistsDirect("/data/adb/shamiko")) {
        LOGW("检测到 Shamiko 目录: /data/adb/shamiko");
        return true;
    }
    if (SyscallUtils::fileExistsDirect("/data/adb/modules/shamiko")) {
        LOGW("检测到 Shamiko 模块目录: /data/adb/modules/shamiko");
        return true;
    }

    std::string propContent = readFileContent("/data/adb/shamiko/whitelist");
    if (!propContent.empty()) {
        LOGW("检测到 Shamiko 白名单配置文件");
        return true;
    }

    return false;
}

ShamikoDetectionDetail ShamikoDetector::detectDetailed() {
    ShamikoDetectionDetail detail;
    detail.has_zygisk_context_timing = false;
    detail.has_sepolicy_overlay = false;
    detail.has_selinux_dynamic_override = false;
    detail.has_mount_filter_rules = false;
    detail.has_proc_filter_whitelist = false;
    detail.has_shamiko_module = false;

    LOGI("=== Shamiko 行为特征专项检测 ===");

    if (detectZygiskContextTiming(detail.context_history)) {
        detail.has_zygisk_context_timing = true;
        detail.findings.push_back("检测到 Zygisk 上下文时序异常（可能由 Shamiko 动态替换引发）");
    }

    if (detectSepolicyOverlay()) {
        detail.has_sepolicy_overlay = true;
        detail.findings.push_back("检测到 sepolicy 临时加载/覆写（Shamiko 特征）");
    }

    if (detectSelinuxDynamicOverride()) {
        detail.has_selinux_dynamic_override = true;
        detail.findings.push_back("检测到 SELinux 上下文动态覆写（Shamiko 运行时篡改）");
    }

    if (detectMountFilterRules(detail.suspicious_mounts)) {
        detail.has_mount_filter_rules = true;
        detail.findings.push_back("检测到 " + std::to_string(detail.suspicious_mounts.size()) +
                                  " 条 Shamiko 风格挂载过滤规则");
    }

    if (detectProcFilterWhitelist(detail.hidden_processes)) {
        detail.has_proc_filter_whitelist = true;
        detail.findings.push_back("检测到 " + std::to_string(detail.hidden_processes.size()) +
                                  " 个进程被加入 Shamiko 白名单");
    }

    if (detectShamikoModule()) {
        detail.has_shamiko_module = true;
        detail.findings.push_back("检测到 Shamiko 模块/目录残留");
    }

    return detail;
}

DetectionResult ShamikoDetector::detect() {
    DetectionResult result;
    result.layer = "对抗隐藏-SHamiko行为特征探针";
    result.detected = false;

    auto detail = detectDetailed();

    if (detail.has_zygisk_context_timing || detail.has_sepolicy_overlay ||
        detail.has_selinux_dynamic_override || detail.has_mount_filter_rules ||
        detail.has_proc_filter_whitelist || detail.has_shamiko_module) {
        result.detected = true;
        std::string summary;
        for (size_t i = 0; i < detail.findings.size(); i++) {
            if (i > 0) summary += "; ";
            summary += detail.findings[i];
        }
        result.detail = "Shamiko 异常: " + summary;
    } else {
        result.detail = "未发现 Shamiko 行为特征";
    }

    return result;
}
