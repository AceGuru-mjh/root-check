#include "apatch_kpm_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <dirent.h>
#include <android/log.h>

#define LOG_TAG "ApatchKpmDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

static const int TRUNCATE_ITERATIONS = 10000;
static const long long THRESHOLD_LOW_NS = 300;
static const long long THRESHOLD_HIGH_NS = 800;

std::string ApatchKpmDetector::readContent(const std::string& path) {
    std::string c = SyscallUtils::readFileDirect(path);
    if (!c.empty()) return c;
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool ApatchKpmDetector::detectKpmNode(std::vector<KpmModuleInfo>& modules) {
    auto entries = SyscallUtils::listFilesDirect("/sys/kpm");
    if (!entries.empty()) {
        for (const auto& entry : entries) {
            KpmModuleInfo info;
            info.name = entry;
            info.path = "/sys/kpm/" + entry;
            info.is_loaded = true;
            info.version = readContent("/sys/kpm/" + entry + "/version");
            modules.push_back(info);
            LOGW("检测到 KPM 节点: /sys/kpm/%s", entry.c_str());
        }
        return true;
    }

    std::string kpmLoad = readContent("/sys/kpm/kpm_loaded");
    if (!kpmLoad.empty()) {
        KpmModuleInfo info;
        info.name = "kpm_loaded";
        info.path = "/sys/kpm/kpm_loaded";
        info.is_loaded = (kpmLoad.find("1") != std::string::npos);
        modules.push_back(info);
        LOGW("检测到 KPM 加载状态: %s", kpmLoad.c_str());
        return true;
    }

    std::string kpmList = readContent("/sys/kpm/list");
    if (!kpmList.empty()) {
        std::istringstream stream(kpmList);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            KpmModuleInfo info;
            info.name = line;
            info.path = "/sys/kpm/list";
            info.is_loaded = true;
            modules.push_back(info);
        }
        if (!modules.empty()) return true;
    }

    DIR* kpmDir = opendir("/sys/kpm");
    if (kpmDir) {
        closedir(kpmDir);
        if (modules.empty()) {
            KpmModuleInfo info;
            info.name = "kpm_dir_exists";
            info.path = "/sys/kpm";
            info.is_loaded = false;
            modules.push_back(info);
        }
        return true;
    }

    return false;
}

long long ApatchKpmDetector::measureTimens(int fd, int value, int iterations) {
    struct timespec ts;
    long long start, end;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    start = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    for (int i = 0; i < iterations; i++) {
        ftruncate(fd, value);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    end = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    return (end - start) / iterations;
}

bool ApatchKpmDetector::detectTruncateBackdoor(std::vector<TruncateTimingSample>& samples) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        fd = open("/proc/self/fd/0", O_RDWR);
        if (fd < 0) return false;
    }

    for (int warm = 0; warm < 200; warm++) {
        ftruncate(fd, 0xffff);
        ftruncate(fd, 0x10000);
    }

    std::vector<long long> backdoorValues = {0xffff, 0xfffe, 0xfffd, 0xfffc, 0x1ffff, 0x2ffff, 0x3ffff};
    std::vector<long long> normalValues = {0x10000, 0x10001, 0x20000, 0x30000, 0x40000, 0x50000, 0x7fff};

    bool found = false;

    for (size_t i = 0; i < backdoorValues.size() && i < normalValues.size(); i++) {
        TruncateTimingSample sample;
        sample.t_backdoor_ns = measureTimens(fd, (int)backdoorValues[i], TRUNCATE_ITERATIONS);
        sample.t_normal_ns = measureTimens(fd, (int)normalValues[i], TRUNCATE_ITERATIONS);
        sample.diff_ns = sample.t_backdoor_ns - sample.t_normal_ns;

        bool lowSuspicious = (sample.diff_ns > THRESHOLD_LOW_NS);
        bool highSuspicious = (sample.diff_ns > THRESHOLD_HIGH_NS);

        if (lowSuspicious || highSuspicious) {
            sample.threshold_used = highSuspicious ? THRESHOLD_HIGH_NS : THRESHOLD_LOW_NS;
            sample.is_suspicious = true;
            found = true;
            LOGW("APatch truncate 后门: 0x%llx -> %lldns, 正常: 0x%llx -> %lldns, 差: %lldns",
                 backdoorValues[i], sample.t_backdoor_ns,
                 normalValues[i], sample.t_normal_ns,
                 sample.diff_ns);
        } else {
            sample.is_suspicious = false;
            sample.threshold_used = 0;
        }

        samples.push_back(sample);
    }

    close(fd);
    return found;
}

bool ApatchKpmDetector::detectDualThresholdValidation(const std::vector<TruncateTimingSample>& samples) {
    int lowHits = 0, highHits = 0;
    for (const auto& s : samples) {
        if (s.diff_ns > THRESHOLD_LOW_NS) lowHits++;
        if (s.diff_ns > THRESHOLD_HIGH_NS) highHits++;
    }

    bool lowPass = (lowHits >= 3);
    bool highPass = (highHits >= 1);

    if (lowPass && highPass) {
        LOGW("双阈值交叉验证通过: 低阈值命中 %d/7, 高阈值命中 %d/7 (确认 APatch)", lowHits, highHits);
        return true;
    }

    if (lowPass && !highPass) {
        LOGI("仅低阈值命中 %d/7，高阈值未命中，可能存在误报但需关注", lowHits);
    }

    return lowPass && highPass;
}

bool ApatchKpmDetector::detectKallsymsErasure(std::vector<KallsymsFeature>& features) {
    std::string kallsyms = readContent("/proc/kallsyms");
    if (kallsyms.empty()) return false;

    std::vector<std::string> targetSymbols = {
        "kallsyms_lookup_name", "printk", "sprintf",
        "sys_call_table", "proc_create", "kprobe",
        "ftrace_direct_func", "__arm64_sys_openat"
    };

    bool found = false;
    std::istringstream stream(kallsyms);
    std::string line;

    for (const auto& sym : targetSymbols) {
        KallsymsFeature feat;
        feat.symbol_name = sym;
        feat.is_erased = true;

        std::istringstream resetStream(kallsyms);
        bool seen = false;
        while (std::getline(resetStream, line)) {
            if (line.find(sym) != std::string::npos) {
                std::istringstream ls(line);
                std::string addr, type, name;
                if (ls >> addr >> type >> name) {
                    feat.original_state = addr + " " + type;
                    feat.current_state = feat.original_state;
                    feat.is_erased = false;
                    seen = true;
                }
                break;
            }
        }

        if (!seen) {
            feat.current_state = "ERASED";
            features.push_back(feat);
            LOGW("内核符号被擦除: %s", sym.c_str());
            found = true;
        } else {
            features.push_back(feat);
        }
    }

    return found;
}

bool ApatchKpmDetector::detectKernelSymbolHiding(std::vector<KallsymsFeature>& features) {
    std::string kallsyms = readContent("/proc/kallsyms");
    if (kallsyms.empty()) return false;

    std::string modules = readContent("/proc/modules");
    std::string sysModules;
    auto sysModDirs = SyscallUtils::listFilesDirect("/sys/module");

    bool found = false;

    for (const auto& modDir : sysModDirs) {
        if (modules.find(modDir) == std::string::npos) {
            std::string refcnt = readContent("/sys/module/" + modDir + "/refcnt");
            if (!refcnt.empty() && refcnt != "0") {
                std::string lower = modDir;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("apatch") != std::string::npos ||
                    lower.find("kpm") != std::string::npos ||
                    lower.find("kpatch") != std::string::npos) {
                    KallsymsFeature feat;
                    feat.symbol_name = "module:" + modDir;
                    feat.original_state = "exists:sys/module";
                    feat.current_state = "hidden:proc/modules";
                    feat.is_erased = true;
                    features.push_back(feat);
                    LOGW("内核模块被隐藏: %s (/sys/module 存在但 /proc/modules 不存在)", modDir.c_str());
                    found = true;
                }
            }
        }
    }

    std::set<std::string> kpmHooks = {
        "apatch_", "kpm_", "kpatch_", "kprobe_hook_",
        "ftrace_hook_", "inline_hook_"
    };

    std::istringstream ksStream(kallsyms);
    std::string line;
    bool hasAnyKpmSymbol = false;

    while (std::getline(ksStream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& hook : kpmHooks) {
            if (lower.find(hook) != std::string::npos) {
                hasAnyKpmSymbol = true;
                KallsymsFeature feat;
                std::istringstream ls(line);
                ls >> feat.symbol_name;
                std::string t, n;
                if (ls >> t >> n) feat.symbol_name = n;
                feat.original_state = line;
                feat.current_state = line;
                feat.is_erased = false;
                features.push_back(feat);
                LOGW("发现 KPM 相关符号: %s", line.c_str());
                found = true;
                break;
            }
        }
    }

    if (hasAnyKpmSymbol) {
        std::string kallsyms2 = readContent("/proc/kallsyms");
        if (kallsyms2 != kallsyms) {
            LOGW("kallsyms 内容在两次读取中不一致，可能存在动态隐藏");
            found = true;
        }
    }

    return found;
}

ApatchKpmDetectionDetail ApatchKpmDetector::detectDetailed() {
    ApatchKpmDetectionDetail detail;
    detail.has_kpm_node = false;
    detail.has_truncate_backdoor_low = false;
    detail.has_truncate_backdoor_high = false;
    detail.has_truncate_dual_threshold = false;
    detail.has_kallsyms_erasure = false;
    detail.has_kernel_symbol_hiding = false;

    LOGI("=== APatch KPM 内核补丁检测 ===");

    if (detectKpmNode(detail.kpm_modules)) {
        detail.has_kpm_node = true;
        detail.findings.push_back("检测到 KPM 模块加载节点 (" +
                                  std::to_string(detail.kpm_modules.size()) + " 个)");
    }

    if (detectTruncateBackdoor(detail.truncate_samples)) {
        int lowCount = 0, highCount = 0;
        for (const auto& s : detail.truncate_samples) {
            if (s.diff_ns > THRESHOLD_LOW_NS) lowCount++;
            if (s.diff_ns > THRESHOLD_HIGH_NS) highCount++;
        }
        if (lowCount > 0) {
            detail.has_truncate_backdoor_low = true;
            detail.findings.push_back("低频阈值检测到 truncate 后门特征 (" +
                                      std::to_string(lowCount) + "/" +
                                      std::to_string(detail.truncate_samples.size()) + ")");
        }
        if (highCount > 0) {
            detail.has_truncate_backdoor_high = true;
            detail.findings.push_back("高频阈值确认 truncate 后门存在 (" +
                                      std::to_string(highCount) + "/" +
                                      std::to_string(detail.truncate_samples.size()) + ")");
        }

        if (detectDualThresholdValidation(detail.truncate_samples)) {
            detail.has_truncate_dual_threshold = true;
            detail.findings.push_back("双阈值交叉验证通过，确认 APatch 后门");
        }
    }

    if (detectKallsymsErasure(detail.kallsyms_features)) {
        detail.has_kallsyms_erasure = true;
        detail.findings.push_back("检测到 kallsyms 符号擦除");
    }

    if (detectKernelSymbolHiding(detail.kallsyms_features)) {
        detail.has_kernel_symbol_hiding = true;
        detail.findings.push_back("检测到内核符号隐藏/模块隐藏");
    }

    return detail;
}

DetectionResult ApatchKpmDetector::detect() {
    DetectionResult result;
    result.layer = "对抗隐藏-APatch KPM内核补丁探针";
    result.detected = false;

    auto detail = detectDetailed();

    bool confirmed = detail.has_truncate_dual_threshold ||
                     (detail.has_truncate_backdoor_high && detail.has_kpm_node);

    bool suspicious = detail.has_truncate_backdoor_low ||
                      detail.has_kallsyms_erasure ||
                      detail.has_kernel_symbol_hiding;

    if (confirmed) {
        result.detected = true;
        std::string summary;
        for (size_t i = 0; i < detail.findings.size(); i++) {
            if (i > 0) summary += "; ";
            summary += detail.findings[i];
        }
        result.detail = "APatch KPM 已确认: " + summary;
    } else if (suspicious) {
        result.detected = true;
        result.detail = "APatch KPM 可疑: 部分特征匹配";
    } else {
        result.detail = "未发现 APatch KPM 特征";
    }

    return result;
}
