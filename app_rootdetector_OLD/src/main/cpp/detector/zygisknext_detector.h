#ifndef ROOTDETECTOR_ZYGISKNEXT_DETECTOR_H
#define ROOTDETECTOR_ZYGISKNEXT_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>

struct MemoryIsolationMark {
    unsigned long start_addr;
    unsigned long end_addr;
    std::string perms;
    std::string path;
    bool is_isolated;
    std::string reason;
};

struct AnonymousNamespaceInfo {
    int ns_fd;
    std::string ns_type;
    std::string target_proc;
    bool has_escape_intercept;
};

struct MemfdChannel {
    std::string name;
    unsigned long address;
    size_t size;
    bool is_private;
    std::string module_name;
};

struct ZygiskNextDetectionDetail {
    bool has_memory_isolation;
    bool has_ns_escape_intercept;
    bool has_private_memfd;
    bool has_zygisknext_module;
    std::vector<MemoryIsolationMark> isolated_regions;
    std::vector<AnonymousNamespaceInfo> ns_entries;
    std::vector<MemfdChannel> memfd_channels;
    std::vector<std::string> findings;
};

class ZygiskNextDetector {
public:
    static DetectionResult detect();
    static ZygiskNextDetectionDetail detectDetailed();

private:
    static bool scanMemoryIsolationMarks(std::vector<MemoryIsolationMark>& marks);
    static bool detectNamespaceAutoEscape(std::vector<AnonymousNamespaceInfo>& entries);
    static bool detectPrivateMemfdChannels(std::vector<MemfdChannel>& channels);
    static bool detectZygiskNextModule();
    static std::string readProcFile(const std::string& path);
    static std::vector<std::string> listDir(const std::string& path);
};

#endif
