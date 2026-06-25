#include "memory_detector.h"
#include "property_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <android/log.h>

#define LOG_TAG "MemoryDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// 缓存过期时间（秒）
static const int CACHE_EXPIRY_SECONDS = 30;

// 全局缓存实例
static ScanCache g_scanCache = {{}, {}, 0, false};

// 内存特征结构体
struct MemorySignature {
    std::string name;
    std::vector<std::string> patterns;
};

// 获取 Root 工具的内存特征库
std::vector<MemorySignature> getRootToolSignatures() {
    return {
        {
            "Magisk",
            {"libmagisk.so", "magiskd", "/data/adb/magisk", "Magisk", "magisk"}
        },
        {
            "Zygisk",
            {"libzygisk.so", "zygisk", "/dev/socket/zygisk", "Zygisk"}
        },
        {
            "LSPosed",
            {"liblspd.so", "lsposed", "lspd", "/data/adb/lspd", "LSPosed", "xposed"}
        },
        {
            "Shamiko",
            {"shamiko", "Shamiko", "/data/adb/modules/shamiko", "libshamiko.so"}
        },
        {
            "KernelSU",
            {"kernelsu", "ksu", "/data/adb/ksu", "KernelSU", "libkernelsu.so"}
        },
        {
            "APatch",
            {"apatch", "APatch", "/data/adb/apatch", "libapatch.so"}
        },
        {
            "Riru",
            {"riru", "Riru", "libriru.so", "/data/adb/modules/riru-core"}
        }
    };
}

// 构建特征集合（使用 HashSet 优化查找）
std::unordered_set<std::string> buildSignatureSet(const std::vector<MemoryRegionInfo>& regions) {
    std::unordered_set<std::string> signatures;
    for (const auto& region : regions) {
        // 使用路径和权限作为签名
        signatures.insert(region.path + ":" + region.permissions);
    }
    return signatures;
}

// 检查区域是否发生变化
bool MemoryDetector::isRegionChanged(const MemoryRegionInfo& old_region, const MemoryRegionInfo& new_region) {
    return old_region.start_addr != new_region.start_addr ||
           old_region.end_addr != new_region.end_addr ||
           old_region.permissions != new_region.permissions ||
           old_region.path != new_region.path;
}

// 获取扫描缓存
ScanCache& MemoryDetector::getScanCache() {
    return g_scanCache;
}

// 判断是否需要重新扫描
bool MemoryDetector::shouldRescan() {
    if (!g_scanCache.is_valid) {
        return true;
    }
    
    time_t now = time(nullptr);
    return (now - g_scanCache.last_scan_time) > CACHE_EXPIRY_SECONDS;
}

// 清除缓存
void MemoryDetector::clearCache() {
    g_scanCache.last_regions.clear();
    g_scanCache.last_signatures.clear();
    g_scanCache.last_scan_time = 0;
    g_scanCache.is_valid = false;
    LOGI("内存扫描缓存已清除");
}

// 使用 mmap 方式获取内存区域（性能优化）
std::vector<MemoryRegionInfo> MemoryDetector::getMemoryRegionsWithMmap() {
    std::vector<MemoryRegionInfo> regions;
    
    // 打开 /proc/self/maps 文件
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) {
        LOGI("无法打开 /proc/self/maps");
        return regions;
    }
    
    // 获取文件大小
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return regions;
    }
    
    size_t file_size = st.st_size;
    
    // 使用 mmap 映射文件
    void* mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return regions;
    }
    
    // 建议内核预读
    madvise(mapped, file_size, MADV_SEQUENTIAL);
    
    // 解析映射的内容
    const char* data = static_cast<const char*>(mapped);
    const char* end = data + file_size;
    
    while (data < end) {
        // 找到行尾
        const char* line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = end;
        }
        
        std::string line(data, line_end - data);
        
        // 解析内存区域信息
        // 格式: address perms offset dev inode pathname
        MemoryRegionInfo region;
        size_t pos = 0;
        
        // 解析地址范围
        size_t dash_pos = line.find('-', pos);
        if (dash_pos != std::string::npos) {
            region.start_addr = std::stoull(line.substr(pos, dash_pos - pos), nullptr, 16);
            pos = dash_pos + 1;
            size_t space_pos = line.find(' ', pos);
            if (space_pos != std::string::npos) {
                region.end_addr = std::stoull(line.substr(pos, space_pos - pos), nullptr, 16);
                region.size = region.end_addr - region.start_addr;
                pos = space_pos + 1;
            }
        }
        
        // 解析权限
        size_t next_space = line.find(' ', pos);
        if (next_space != std::string::npos) {
            region.permissions = line.substr(pos, next_space - pos);
            pos = next_space + 1;
        }
        
        // 跳过 offset, dev, inode
        for (int i = 0; i < 3 && pos < line.length(); ++i) {
            size_t skip_space = line.find(' ', pos);
            if (skip_space != std::string::npos) {
                pos = skip_space + 1;
                // 跳过多余空格
                while (pos < line.length() && line[pos] == ' ') pos++;
            }
        }
        
        // 解析路径
        if (pos < line.length()) {
            region.path = line.substr(pos);
            // 去除前导空格
            size_t start = region.path.find_first_not_of(" \t");
            if (start != std::string::npos) {
                region.path = region.path.substr(start);
            }
        }
        
        if (!region.path.empty() || !region.permissions.empty()) {
            regions.push_back(region);
        }
        
        // 移动到下一行
        data = line_end + 1;
    }
    
    // 释放映射
    munmap(mapped, file_size);
    close(fd);
    
    LOGI("通过 mmap 解析到 %zu 个内存区域", regions.size());
    return regions;
}

std::vector<std::string> MemoryDetector::getRootKeywords() {
    std::vector<std::string> keywords;
    auto signatures = getRootToolSignatures();
    
    // 使用 HashSet 去重
    std::unordered_set<std::string> unique_keywords;
    for (const auto& sig : signatures) {
        for (const auto& pattern : sig.patterns) {
            unique_keywords.insert(pattern);
        }
    }
    
    keywords.reserve(unique_keywords.size());
    for (const auto& keyword : unique_keywords) {
        keywords.push_back(keyword);
    }
    
    return keywords;
}

bool MemoryDetector::checkAnonymousExecutableMemory() {
    // 使用缓存机制
    if (!shouldRescan()) {
        // 使用缓存数据进行检查
        for (const auto& region : g_scanCache.last_regions) {
            if ((region.permissions.find("r-xp") != std::string::npos || 
                 region.permissions.find("rwxp") != std::string::npos) &&
                (region.path.empty() || region.path[0] != '/')) {
                LOGD("使用缓存检测到匿名可执行内存映射: %s", region.path.c_str());
                return true;
            }
        }
        return false;
    }
    
    // 获取内存区域（使用 mmap 优化）
    auto regions = getMemoryRegionsWithMmap();
    
    for (const auto& region : regions) {
        // 查找可执行权限的匿名映射
        if (region.permissions.find("r-xp") != std::string::npos || 
            region.permissions.find("rwxp") != std::string::npos) {
            // 检查是否是匿名映射（没有文件路径）
            if (region.path.empty() || region.path[0] != '/') {
                LOGI("检测到匿名可执行内存映射: %s", region.path.c_str());
                return true;
            }
        }
    }
    
    return false;
}

bool MemoryDetector::checkMemfdJitCache() {
    // 使用缓存机制
    if (!shouldRescan()) {
        for (const auto& region : g_scanCache.last_regions) {
            if (region.path.find("/memfd:jit-cache") != std::string::npos ||
                region.path.find("jit-cache") != std::string::npos) {
                LOGD("使用缓存检测到 JIT cache 内存区域");
                return true;
            }
        }
        return false;
    }
    
    // 获取内存区域（使用 mmap 优化）
    auto regions = getMemoryRegionsWithMmap();
    
    for (const auto& region : regions) {
        if (region.path.find("/memfd:jit-cache") != std::string::npos ||
            region.path.find("jit-cache") != std::string::npos) {
            LOGI("检测到 JIT cache 内存区域: %s", region.path.c_str());
            return true;
        }
    }
    
    return false;
}

bool MemoryDetector::searchMemoryForStrings() {
    // 使用缓存机制
    if (!shouldRescan()) {
        auto signatures = getRootToolSignatures();
        for (const auto& sig : signatures) {
            for (const auto& pattern : sig.patterns) {
                std::string lowerPattern = pattern;
                std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
                
                if (g_scanCache.last_signatures.find(lowerPattern) != g_scanCache.last_signatures.end()) {
                    LOGD("使用缓存检测到 %s 特征", sig.name.c_str());
                    return true;
                }
            }
        }
        return false;
    }
    
    // 获取内存区域（使用 mmap 优化）
    auto regions = getMemoryRegionsWithMmap();
    
    auto signatures = getRootToolSignatures();
    std::unordered_set<std::string> found_signatures;
    
    for (const auto& region : regions) {
        // 检查映射路径中是否包含特征字符串
        std::string lowerPath = region.path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
        
        for (const auto& sig : signatures) {
            for (const auto& pattern : sig.patterns) {
                std::string lowerPattern = pattern;
                std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
                
                if (lowerPath.find(lowerPattern) != std::string::npos) {
                    LOGI("在内存映射中发现 %s 特征: %s in %s", sig.name.c_str(), pattern.c_str(), region.path.c_str());
                    found_signatures.insert(lowerPattern);
                    return true;
                }
            }
        }
    }
    
    // 更新缓存
    g_scanCache.last_signatures = found_signatures;
    
    return false;
}

DetectionResult MemoryDetector::detect() {
    DetectionResult result;
    result.layer = "第 3 层：内存扫描检测";
    result.detected = false;
    
    std::vector<std::string> findings;
    
    // 检查是否需要重新扫描
    bool need_rescan = shouldRescan();
    
    if (need_rescan) {
        // 获取新的内存区域数据
        auto regions = getMemoryRegionsWithMmap();
        
        // 执行各项检测
        if (checkAnonymousExecutableMemory()) {
            findings.push_back("匿名可执行内存映射");
        }
        
        if (checkMemfdJitCache()) {
            findings.push_back("JIT cache 内存区域");
        }
        
        if (searchMemoryForStrings()) {
            findings.push_back("Root 工具内存特征");
        }
        
        // 更新缓存
        g_scanCache.last_regions = regions;
        g_scanCache.last_scan_time = time(nullptr);
        g_scanCache.is_valid = true;
    } else {
        LOGI("使用缓存数据进行检测（缓存年龄：%ld 秒）", 
             time(nullptr) - g_scanCache.last_scan_time);
        
        // 使用缓存数据执行检测
        if (checkAnonymousExecutableMemory()) {
            findings.push_back("匿名可执行内存映射");
        }
        
        if (checkMemfdJitCache()) {
            findings.push_back("JIT cache 内存区域");
        }
        
        if (searchMemoryForStrings()) {
            findings.push_back("Root 工具内存特征");
        }
    }
    
    if (!findings.empty()) {
        result.detected = true;
        result.detail = "发现 " + std::to_string(findings.size()) + " 个可疑内存特征：";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.detail += findings[i];
            if (i < findings.size() - 1) {
                result.detail += "; ";
            }
        }
    } else {
        result.detail = "未发现可疑内存特征";
    }
    
    return result;
}
