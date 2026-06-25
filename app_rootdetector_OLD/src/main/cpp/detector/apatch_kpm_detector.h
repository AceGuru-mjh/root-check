#ifndef ROOTDETECTOR_APATCH_KPM_DETECTOR_H
#define ROOTDETECTOR_APATCH_KPM_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>

struct TruncateTimingSample {
    long long t_backdoor_ns;
    long long t_normal_ns;
    long long diff_ns;
    int threshold_used;
    bool is_suspicious;
};

struct KallsymsFeature {
    std::string symbol_name;
    std::string original_state;
    std::string current_state;
    bool is_erased;
};

struct KpmModuleInfo {
    std::string name;
    std::string path;
    bool is_loaded;
    std::string version;
};

struct ApatchKpmDetectionDetail {
    bool has_kpm_node;
    bool has_truncate_backdoor_low;
    bool has_truncate_backdoor_high;
    bool has_truncate_dual_threshold;
    bool has_kallsyms_erasure;
    bool has_kernel_symbol_hiding;
    std::vector<TruncateTimingSample> truncate_samples;
    std::vector<KallsymsFeature> kallsyms_features;
    std::vector<KpmModuleInfo> kpm_modules;
    std::vector<std::string> findings;
};

class ApatchKpmDetector {
public:
    static DetectionResult detect();
    static ApatchKpmDetectionDetail detectDetailed();

private:
    static bool detectKpmNode(std::vector<KpmModuleInfo>& modules);
    static bool detectTruncateBackdoor(std::vector<TruncateTimingSample>& samples);
    static bool detectDualThresholdValidation(const std::vector<TruncateTimingSample>& samples);
    static bool detectKallsymsErasure(std::vector<KallsymsFeature>& features);
    static bool detectKernelSymbolHiding(std::vector<KallsymsFeature>& features);
    static long long measureTimens(int fd, int value, int iterations);
    static std::string readContent(const std::string& path);
};

#endif
