#include "memory_fingerprint_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <android/log.h>
#include <cstring>

#define LOG_TAG "MemoryFingerprintDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 获取 Root 工具的内存特征库
std::vector<MemorySignature> MemoryFingerprintDetector::getRootToolSignatures() {
    return {
        // Magisk 特征
        {
            "Magisk",
            {
                "libmagisk.so",
                "magiskd",
                "/data/adb/magisk",
                "Magisk",
                "magisk",
                "MAGISK",
                "magiskd",
                "magiskhide",
                "magiskinit",
                "magiskboot"
            },
            {
                // Magisk 字节模式特征 (示例)
                {
                    "magisk_header",
                    {0x4D, 0x61, 0x67, 0x69, 0x73, 0x6B, 0x00},  // "Magisk\0"
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
                },
                {
                    "magisk_socket",
                    {0x2F, 0x64, 0x65, 0x76, 0x2F, 0x73, 0x6F, 0x63, 0x6B, 0x65, 0x74, 0x2F, 0x6D, 0x61, 0x67, 0x69, 0x73, 0x6B},  // "/dev/socket/magisk"
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
                }
            }
        },
        // Zygisk 特征
        {
            "Zygisk",
            {
                "libzygisk.so",
                "zygisk",
                "Zygisk",
                "ZYGISK",
                "/dev/socket/zygisk",
                "zygiskd",
                "libzygisk"
            },
            {
                {
                    "zygisk_socket",
                    {0x2F, 0x64, 0x65, 0x76, 0x2F, 0x73, 0x6F, 0x63, 0x6B, 0x65, 0x74, 0x2F, 0x7A, 0x79, 0x67, 0x69, 0x73, 0x6B},  // "/dev/socket/zygisk"
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
                }
            }
        },
        // LSPosed 特征
        {
            "LSPosed",
            {
                "liblspd.so",
                "lsposed",
                "LSPosed",
                "LPOSED",
                "lspd",
                "LSPD",
                "/data/adb/lspd",
                "xposed",
                "Xposed",
                "XPOSED",
                "libxposed"
            },
            {
                {
                    "xposed_marker",
                    {0x58, 0x70, 0x6F, 0x73, 0x65, 0x64, 0x00},  // "Xposed\0"
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
                }
            }
        },
        // Shamiko 特征
        {
            "Shamiko",
            {
                "shamiko",
                "Shamiko",
                "SHAMIKO",
                "libshamiko.so",
                "/data/adb/modules/shamiko",
                "hide",
                "Hide",
                "HIDE"
            },
            {
                // Shamiko 特征字节模式
                {
                    "shamiko_hide",
                    {0x68, 0x69, 0x64, 0x65, 0x00},  // "hide\0"
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
                }
            }
        },
        // KernelSU 特征
        {
            "KernelSU",
            {
                "kernelsu",
                "KernelSU",
                "KERNELSU",
                "ksu",
                "KSU",
                "/data/adb/ksu",
                "libkernelsu.so"
            },
            {}
        },
        // APatch 特征
        {
            "APatch",
            {
                "apatch",
                "APatch",
                "APATCH",
                "/data/adb/apatch",
                "libapatch.so"
            },
            {}
        }
    };
}

// 解析 /proc/self/maps 获取可执行内存区域
std::vector<MemoryRegion> MemoryFingerprintDetector::getExecutableMemoryRegions() {
    std::vector<MemoryRegion> regions;
    std::ifstream maps("/proc/self/maps");
    
    if (!maps.is_open()) {
        LOGE("无法打开 /proc/self/maps");
        return regions;
    }
    
    std::string line;
    while (std::getline(maps, line)) {
        // 解析 maps 格式: address perms offset dev inode pathname
        // 例如: 7f8a1b2c4000-7f8a1b2c5000 r-xp 00000000 08:01 123456 /system/lib64/libc.so
        
        std::istringstream iss(line);
        std::string addr_range, perms, offset, dev, inode;
        std::string path;
        
        iss >> addr_range >> perms >> offset >> dev >> inode;
        
        // 读取剩余部分作为路径（可能包含空格）
        std::getline(iss, path);
        // 去除前导空格
        size_t start = path.find_first_not_of(" \t");
        if (start != std::string::npos) {
            path = path.substr(start);
        } else {
            path = "";
        }
        
        // 检查是否有执行权限
        if (perms.length() >= 4 && perms[2] == 'x') {
            MemoryRegion region;
            
            // 解析地址范围
            size_t dash_pos = addr_range.find('-');
            if (dash_pos != std::string::npos) {
                std::string start_str = addr_range.substr(0, dash_pos);
                std::string end_str = addr_range.substr(dash_pos + 1);
                
                region.start_addr = std::stoull(start_str, nullptr, 16);
                region.end_addr = std::stoull(end_str, nullptr, 16);
            } else {
                continue;
            }
            
            region.readable = (perms[0] == 'r');
            region.writable = (perms[1] == 'w');
            region.executable = (perms[2] == 'x');
            region.path = path;
            
            regions.push_back(region);
        }
    }
    
    LOGI("找到 %zu 个可执行内存区域", regions.size());
    return regions;
}

// 从 /proc/self/mem 读取指定内存区域
std::vector<uint8_t> MemoryFingerprintDetector::readMemoryRegion(uintptr_t start_addr, uintptr_t end_addr) {
    std::vector<uint8_t> data;
    
    int mem_fd = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
    if (mem_fd < 0) {
        LOGE("无法打开 /proc/self/mem");
        return data;
    }
    
    size_t size = end_addr - start_addr;
    data.resize(size);
    
    // 使用 pread 读取指定地址的内存
    ssize_t bytes_read = pread(mem_fd, data.data(), size, start_addr);
    
    if (bytes_read < 0) {
        LOGE("读取内存区域 0x%lx - 0x%lx 失败", start_addr, end_addr);
        data.clear();
    } else if (static_cast<size_t>(bytes_read) < size) {
        LOGD("部分读取内存区域 0x%lx - 0x%lx: %zd / %zu 字节", start_addr, end_addr, bytes_read, size);
        data.resize(bytes_read);
    }
    
    close(mem_fd);
    return data;
}

// 在内存数据中搜索字符串特征
bool MemoryFingerprintDetector::searchStringPattern(const std::vector<uint8_t>& data,
                                                     const std::string& pattern,
                                                     uintptr_t base_addr,
                                                     uintptr_t& match_addr) {
    if (data.empty() || pattern.empty()) {
        return false;
    }
    
    // 使用 Boyer-Moore-Horspool 或简单的 memmem 搜索
    const char* pattern_cstr = pattern.c_str();
    size_t pattern_len = pattern.length();
    
    // 在数据中搜索字符串
    for (size_t i = 0; i + pattern_len <= data.size(); ++i) {
        if (memcmp(data.data() + i, pattern_cstr, pattern_len) == 0) {
            match_addr = base_addr + i;
            return true;
        }
    }
    
    return false;
}

// 在内存数据中搜索字节模式特征
bool MemoryFingerprintDetector::searchBytePattern(const std::vector<uint8_t>& data,
                                                   const BytePattern& pattern,
                                                   uintptr_t base_addr,
                                                   uintptr_t& match_addr) {
    if (data.empty() || pattern.pattern.empty() || 
        pattern.pattern.size() != pattern.mask.size()) {
        return false;
    }
    
    size_t pattern_len = pattern.pattern.size();
    
    // 搜索带掩码的字节模式
    for (size_t i = 0; i + pattern_len <= data.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < pattern_len; ++j) {
            if ((data[i + j] & pattern.mask[j]) != (pattern.pattern[j] & pattern.mask[j])) {
                match = false;
                break;
            }
        }
        
        if (match) {
            match_addr = base_addr + i;
            return true;
        }
    }
    
    return false;
}

// 扫描单个内存区域
std::vector<FingerprintMatchResult> MemoryFingerprintDetector::scanMemoryRegion(
    const MemoryRegion& region,
    const std::vector<MemorySignature>& signatures) {
    
    std::vector<FingerprintMatchResult> results;
    
    // 读取内存区域内容
    std::vector<uint8_t> data = readMemoryRegion(region.start_addr, region.end_addr);
    if (data.empty()) {
        return results;
    }
    
    // 对每个特征签名进行搜索
    for (const auto& sig : signatures) {
        // 搜索字符串特征
        for (const auto& pattern : sig.string_patterns) {
            uintptr_t match_addr = 0;
            if (searchStringPattern(data, pattern, region.start_addr, match_addr)) {
                FingerprintMatchResult result;
                result.tool_name = sig.tool_name;
                result.match_type = "string";
                result.match_detail = pattern;
                result.address = match_addr;
                results.push_back(result);
                
                LOGI("在内存区域 0x%lx 发现 %s 字符串特征: %s @ 0x%lx",
                     region.start_addr, sig.tool_name.c_str(), pattern.c_str(), match_addr);
            }
        }
        
        // 搜索字节模式特征
        for (const auto& pattern : sig.byte_patterns) {
            uintptr_t match_addr = 0;
            if (searchBytePattern(data, pattern, region.start_addr, match_addr)) {
                FingerprintMatchResult result;
                result.tool_name = sig.tool_name;
                result.match_type = "byte_pattern";
                result.match_detail = pattern.name;
                result.address = match_addr;
                results.push_back(result);
                
                LOGI("在内存区域 0x%lx 发现 %s 字节模式特征: %s @ 0x%lx",
                     region.start_addr, sig.tool_name.c_str(), pattern.name.c_str(), match_addr);
            }
        }
    }
    
    return results;
}

// 执行完整的内存指纹扫描
std::vector<FingerprintMatchResult> MemoryFingerprintDetector::performDeepScan() {
    std::vector<FingerprintMatchResult> all_results;
    
    // 获取所有可执行内存区域
    std::vector<MemoryRegion> regions = getExecutableMemoryRegions();
    if (regions.empty()) {
        LOGE("未找到可执行内存区域");
        return all_results;
    }
    
    // 获取特征库
    std::vector<MemorySignature> signatures = getRootToolSignatures();
    
    // 扫描每个内存区域
    size_t scanned_regions = 0;
    for (const auto& region : regions) {
        // 跳过特殊映射（如 [vvar], [vdso], [vsyscall] 等）
        if (!region.path.empty() && region.path[0] == '[') {
            continue;
        }
        
        auto results = scanMemoryRegion(region, signatures);
        all_results.insert(all_results.end(), results.begin(), results.end());
        scanned_regions++;
    }
    
    LOGI("扫描完成：共扫描 %zu 个内存区域，发现 %zu 个匹配", 
         scanned_regions, all_results.size());
    
    return all_results;
}

// 主检测函数
DetectionResult MemoryFingerprintDetector::detect() {
    DetectionResult result;
    result.layer = "第3层增强：深度内存指纹扫描";
    result.detected = false;
    
    LOGI("开始深度内存指纹扫描...");
    
    // 执行深度扫描
    std::vector<FingerprintMatchResult> matches = performDeepScan();
    
    if (matches.empty()) {
        result.detail = "未发现 Root 工具内存特征";
        LOGI("未发现 Root 工具内存特征");
        return result;
    }
    
    // 统计检测到的工具
    std::vector<std::string> detected_tools;
    std::vector<std::string> findings;
    
    for (const auto& match : matches) {
        // 检查是否已经记录过该工具
        bool already_recorded = false;
        for (const auto& tool : detected_tools) {
            if (tool == match.tool_name) {
                already_recorded = true;
                break;
            }
        }
        
        if (!already_recorded) {
            detected_tools.push_back(match.tool_name);
        }
        
        // 构建详细信息
        std::string finding = match.tool_name + " (" + match.match_type + ": " + 
                             match.match_detail + " @ 0x" + 
                             std::to_string(match.address) + ")";
        findings.push_back(finding);
    }
    
    result.detected = true;
    result.detail = "检测到 " + std::to_string(detected_tools.size()) + " 个 Root 工具，" +
                   std::to_string(matches.size()) + " 个内存特征匹配: ";
    
    for (size_t i = 0; i < findings.size(); ++i) {
        result.detail += findings[i];
        if (i < findings.size() - 1) {
            result.detail += "; ";
        }
    }
    
    LOGI("深度内存指纹扫描完成：检测到 %zu 个 Root 工具", detected_tools.size());
    
    return result;
}
