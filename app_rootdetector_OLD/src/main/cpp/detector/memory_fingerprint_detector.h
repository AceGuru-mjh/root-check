#ifndef ROOTDETECTOR_MEMORY_FINGERPRINT_DETECTOR_H
#define ROOTDETECTOR_MEMORY_FINGERPRINT_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>
#include <cstdint>

// 内存区域信息
struct MemoryRegion {
    uintptr_t start_addr;
    uintptr_t end_addr;
    bool readable;
    bool writable;
    bool executable;
    std::string path;
};

// 字节模式特征
struct BytePattern {
    std::string name;
    std::vector<uint8_t> pattern;
    std::vector<uint8_t> mask;  // 掩码，0xFF 表示精确匹配，0x00 表示通配
};

// 内存特征签名
struct MemorySignature {
    std::string tool_name;
    std::vector<std::string> string_patterns;
    std::vector<BytePattern> byte_patterns;
};

// 检测结果详情
struct FingerprintMatchResult {
    std::string tool_name;
    std::string match_type;  // "string" or "byte_pattern"
    std::string match_detail;
    uintptr_t address;
};

class MemoryFingerprintDetector {
public:
    static DetectionResult detect();
    
private:
    // 获取 Root 工具的内存特征库
    static std::vector<MemorySignature> getRootToolSignatures();
    
    // 解析 /proc/self/maps 获取可执行内存区域
    static std::vector<MemoryRegion> getExecutableMemoryRegions();
    
    // 从 /proc/self/mem 读取指定内存区域
    static std::vector<uint8_t> readMemoryRegion(uintptr_t start_addr, uintptr_t end_addr);
    
    // 在内存数据中搜索字符串特征
    static bool searchStringPattern(const std::vector<uint8_t>& data, 
                                    const std::string& pattern,
                                    uintptr_t base_addr,
                                    uintptr_t& match_addr);
    
    // 在内存数据中搜索字节模式特征
    static bool searchBytePattern(const std::vector<uint8_t>& data,
                                  const BytePattern& pattern,
                                  uintptr_t base_addr,
                                  uintptr_t& match_addr);
    
    // 扫描单个内存区域
    static std::vector<FingerprintMatchResult> scanMemoryRegion(
        const MemoryRegion& region,
        const std::vector<MemorySignature>& signatures);
    
    // 执行完整的内存指纹扫描
    static std::vector<FingerprintMatchResult> performDeepScan();
};

#endif // ROOTDETECTOR_MEMORY_FINGERPRINT_DETECTOR_H
