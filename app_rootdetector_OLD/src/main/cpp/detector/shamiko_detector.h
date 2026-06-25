#ifndef ROOTDETECTOR_SHAMIKO_DETECTOR_H
#define ROOTDETECTOR_SHAMIKO_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>
#include <chrono>

struct SecontextSnapshot {
    std::string context;
    long long timestamp_ms;
    std::string tag;
};

struct MountFilterRule {
    std::string source;
    std::string target;
    std::string fstype;
    std::string options;
    bool is_shamiko_style;
};

struct ProcFilterEntry {
    int pid;
    std::string name;
    std::string secontext;
    bool is_whitelisted;
};

struct ShamikoDetectionDetail {
    bool has_zygisk_context_timing;
    bool has_sepolicy_overlay;
    bool has_selinux_dynamic_override;
    bool has_mount_filter_rules;
    bool has_proc_filter_whitelist;
    bool has_shamiko_module;
    std::vector<std::string> findings;
    std::vector<SecontextSnapshot> context_history;
    std::vector<MountFilterRule> suspicious_mounts;
    std::vector<ProcFilterEntry> hidden_processes;
};

class ShamikoDetector {
public:
    static DetectionResult detect();
    static ShamikoDetectionDetail detectDetailed();

private:
    static SecontextSnapshot captureSecontext(const std::string& tag);
    static bool detectZygiskContextTiming(std::vector<SecontextSnapshot>& history);
    static bool detectSepolicyOverlay();
    static bool detectSelinuxDynamicOverride();
    static bool detectMountFilterRules(std::vector<MountFilterRule>& rules);
    static bool detectProcFilterWhitelist(std::vector<ProcFilterEntry>& entries);
    static bool detectShamikoModule();
    static std::string readFileContent(const std::string& path);
    static std::vector<std::string> listDirectory(const std::string& path);
};

#endif
