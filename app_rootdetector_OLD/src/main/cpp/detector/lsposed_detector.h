#ifndef ROOTDETECTOR_LSPOSED_DETECTOR_H
#define ROOTDETECTOR_LSPOSED_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>

struct RiruInjectionTrace {
    std::string lib_path;
    unsigned long base_addr;
    std::string riru_version;
    bool is_old_riru;
};

struct LSPosedZygiskMode {
    std::string lib_path;
    unsigned long base_addr;
    bool is_zygisk_mode;
    std::string lsposed_version;
};

struct LiblspdBinderChannel {
    std::string channel_type;
    std::string endpoint;
    bool is_active;
    std::string binder_context;
};

struct LSPosedDetectionDetail {
    bool has_old_riru;
    bool has_new_zygisk_mode;
    bool has_liblspd_binder;
    bool has_lsposed_module;
    std::vector<RiruInjectionTrace> riru_traces;
    std::vector<LSPosedZygiskMode> zygisk_modes;
    std::vector<LiblspdBinderChannel> binder_channels;
    std::vector<std::string> findings;
};

class LSPosedDetector {
public:
    static DetectionResult detect();
    static LSPosedDetectionDetail detectDetailed();

private:
    static bool detectOldRiruInjection(std::vector<RiruInjectionTrace>& traces);
    static bool detectLSPosedZygiskMode(std::vector<LSPosedZygiskMode>& modes);
    static bool detectLiblspdBinderChannel(std::vector<LiblspdBinderChannel>& channels);
    static bool detectLSPosedModule();
    static std::string getLibContent(const std::string& path);
    static std::vector<std::string> listDir(const std::string& path);
};

#endif
