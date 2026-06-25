#ifndef ROOTDETECTOR_MEMORY_DETECTOR_H
#define ROOTDETECTOR_MEMORY_DETECTOR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sys/types.h>

struct DetectionResult;

// 内存区域信息
struct MemoryRegionInfo {
    uintptr_t start_addr;
    uintptr_t end_addr;
    std::string permissions;
    std::string path;
    size_t size;
};

// 扫描缓存
struct ScanCache {
    std::vector<MemoryRegionInfo> last_regions;
    std::unordered_set<std::string> last_signatures;
    time_t last_scan_time;
    bool is_valid;
};

class MemoryDetector {
public:
    static DetectionResult detect();
    
    // 清除缓存
    static void clearCache();
    
private:
    static std::vector<std::string> getRootKeywords();
    static bool checkAnonymousExecutableMemory();
    static bool checkMemfdJitCache();
    static bool searchMemoryForStrings();
    
    // 性能优化方法
    static std::vector<MemoryRegionInfo> getMemoryRegionsWithMmap();
    static bool isRegionChanged(const MemoryRegionInfo& old_region, const MemoryRegionInfo& new_region);
    static std::unordered_set<std::string> buildSignatureSet(const std::vector<MemoryRegionInfo>& regions);
    
    // 缓存管理
    static ScanCache& getScanCache();
    static bool shouldRescan();
};

#endif // ROOTDETECTOR_MEMORY_DETECTOR_H
