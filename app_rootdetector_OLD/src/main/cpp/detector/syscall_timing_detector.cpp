#include "syscall_timing_detector.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <dirent.h>
#include <android/log.h>
#include <cstring>
#include <algorithm>
#include <numeric>

#define LOG_TAG "SyscallTimingDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// 高精度时间获取（纳秒）
long long SyscallTimingDetector::getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// 系统调用类型转字符串
std::string SyscallTimingDetector::syscallTypeToString(SyscallType type) {
    switch (type) {
        case SyscallType::OPENAT: return "openat";
        case SyscallType::READLINKAT: return "readlinkat";
        case SyscallType::GETDENTS64: return "getdents64";
        case SyscallType::STAT: return "stat";
        default: return "unknown";
    }
}

// 测量 openat 系统调用时间
long long SyscallTimingDetector::measureOpenat(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);
    
    const char* test_path = "/dev/null";
    
    for (int i = 0; i < iterations; i++) {
        long long start = getTimeNs();
        int fd = syscall(__NR_openat, AT_FDCWD, test_path, O_RDONLY | O_CLOEXEC, 0);
        long long end = getTimeNs();
        
        if (fd >= 0) {
            close(fd);
            times.push_back(end - start);
        }
    }
    
    if (times.empty()) return 0;
    
    // 排序并去除极端值（取中间 80%）
    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;
    
    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    
    return sum / (end_idx - start_idx);
}

// 测量 readlinkat 系统调用时间
long long SyscallTimingDetector::measureReadlinkat(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);
    
    const char* test_path = "/proc/self/exe";
    char buffer[PATH_MAX];
    
    for (int i = 0; i < iterations; i++) {
        long long start = getTimeNs();
        long ret = syscall(__NR_readlinkat, AT_FDCWD, test_path, buffer, sizeof(buffer) - 1);
        long long end = getTimeNs();
        
        if (ret > 0) {
            times.push_back(end - start);
        }
    }
    
    if (times.empty()) return 0;
    
    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;
    
    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    
    return sum / (end_idx - start_idx);
}

// 测量 getdents64 系统调用时间
long long SyscallTimingDetector::measureGetdents64(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);
    
    const char* test_path = "/system/bin";
    
    for (int i = 0; i < iterations; i++) {
        int fd = syscall(__NR_openat, AT_FDCWD, test_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
        if (fd < 0) continue;
        
        char buf[4096];
        long long start = getTimeNs();
        long nread = syscall(__NR_getdents64, fd, buf, sizeof(buf));
        long long end = getTimeNs();
        
        close(fd);
        
        if (nread > 0) {
            times.push_back(end - start);
        }
    }
    
    if (times.empty()) return 0;
    
    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;
    
    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    
    return sum / (end_idx - start_idx);
}

// 测量 stat 系统调用时间
long long SyscallTimingDetector::measureStat(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);
    
    const char* test_path = "/system/bin/sh";
    struct stat statbuf;
    
    for (int i = 0; i < iterations; i++) {
        long long start = getTimeNs();
        int ret = syscall(__NR_stat, test_path, &statbuf);
        long long end = getTimeNs();
        
        if (ret == 0) {
            times.push_back(end - start);
        }
    }
    
    if (times.empty()) return 0;
    
    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;
    
    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    
    return sum / (end_idx - start_idx);
}

// 获取设备 CPU 架构
std::string SyscallTimingDetector::getDeviceArch() {
#if defined(__aarch64__)
    return "arm64-v8a";
#elif defined(__arm__)
    return "armeabi-v7a";
#elif defined(__i386__)
    return "x86";
#elif defined(__x86_64__)
    return "x86_64";
#else
    return "unknown";
#endif
}

// 获取 Android 版本
std::string SyscallTimingDetector::getAndroidVersion() {
    char value[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.build.version.release", value);
    return std::string(value);
}

// 获取基准数据库
std::map<std::string, BaselineEntry> SyscallTimingDetector::getBaselineDatabase() {
    std::map<std::string, BaselineEntry> database;
    
    // arm64-v8a 架构基准数据
    database["arm64-v8a_12"] = {
        "arm64-v8a", "12",
        1500,   // openat
        2000,   // readlinkat
        3500,   // getdents64
        1200    // stat
    };
    
    database["arm64-v8a_13"] = {
        "arm64-v8a", "13",
        1400,   // openat
        1800,   // readlinkat
        3200,   // getdents64
        1100    // stat
    };
    
    database["arm64-v8a_14"] = {
        "arm64-v8a", "14",
        1300,   // openat
        1700,   // readlinkat
        3000,   // getdents64
        1000    // stat
    };
    
    // armeabi-v7a 架构基准数据（通常较慢）
    database["armeabi-v7a_12"] = {
        "armeabi-v7a", "12",
        2500,   // openat
        3200,   // readlinkat
        5500,   // getdents64
        2000    // stat
    };
    
    database["armeabi-v7a_13"] = {
        "armeabi-v7a", "13",
        2300,   // openat
        3000,   // readlinkat
        5200,   // getdents64
        1800    // stat
    };
    
    database["armeabi-v7a_14"] = {
        "armeabi-v7a", "14",
        2200,   // openat
        2800,   // readlinkat
        5000,   // getdents64
        1700    // stat
    };
    
    return database;
}

// 获取当前设备的基准数据
BaselineEntry SyscallTimingDetector::getCurrentBaseline() {
    std::string arch = getDeviceArch();
    std::string version = getAndroidVersion();
    
    BaselineEntry baseline;
    if (findBaseline(arch, version, baseline)) {
        return baseline;
    }
    
    // 如果找不到精确匹配，返回默认值
    LOGW("未找到匹配的基准数据，使用默认值: arch=%s, version=%s", 
         arch.c_str(), version.c_str());
    
    baseline.arch = arch;
    baseline.android_version = version;
    baseline.openat_ns = 2000;
    baseline.readlinkat_ns = 2500;
    baseline.getdents64_ns = 4000;
    baseline.stat_ns = 1500;
    
    return baseline;
}

// 查找匹配的基准数据
bool SyscallTimingDetector::findBaseline(const std::string& arch, const std::string& version, 
                                         BaselineEntry& baseline) {
    auto database = getBaselineDatabase();
    std::string key = arch + "_" + version;
    
    auto it = database.find(key);
    if (it != database.end()) {
        baseline = it->second;
        return true;
    }
    
    return false;
}

// 检测时间异常
bool SyscallTimingDetector::detectAnomaly(SyscallType type, long long measured_time_ns, 
                                          long long baseline_time_ns, std::string& reason) {
    if (baseline_time_ns == 0) {
        reason = "基准时间为 0，无法比较";
        return false;
    }
    
    double ratio = static_cast<double>(measured_time_ns) / baseline_time_ns;
    
    if (ratio > ANOMALY_THRESHOLD) {
        reason = "执行时间异常：" + std::to_string(measured_time_ns) + " ns > 基准 " + 
                 std::to_string(baseline_time_ns) + " ns × " + 
                 std::to_string(ANOMALY_THRESHOLD) + " (比率: " + 
                 std::to_string(ratio).substr(0, 4) + ")";
        LOGW("系统调用 %s 时间异常: %lld ns vs 基准 %lld ns (比率: %.2f)",
             syscallTypeToString(type).c_str(), measured_time_ns, baseline_time_ns, ratio);
        return true;
    }
    
    return false;
}

// 测量单个系统调用的执行时间
SyscallTimingResult SyscallTimingDetector::measureSyscall(SyscallType type) {
    SyscallTimingResult result;
    result.type = type;
    result.name = syscallTypeToString(type);
    result.is_anomaly = false;
    
    // 预热
    for (int i = 0; i < 100; i++) {
        switch (type) {
            case SyscallType::OPENAT:
                measureOpenat(10);
                break;
            case SyscallType::READLINKAT:
                measureReadlinkat(10);
                break;
            case SyscallType::GETDENTS64:
                measureGetdents64(10);
                break;
            case SyscallType::STAT:
                measureStat(10);
                break;
        }
    }
    
    // 正式测量
    long long avg_time = 0;
    switch (type) {
        case SyscallType::OPENAT:
            avg_time = measureOpenat(DEFAULT_ITERATIONS);
            break;
        case SyscallType::READLINKAT:
            avg_time = measureReadlinkat(DEFAULT_ITERATIONS);
            break;
        case SyscallType::GETDENTS64:
            avg_time = measureGetdents64(DEFAULT_ITERATIONS);
            break;
        case SyscallType::STAT:
            avg_time = measureStat(DEFAULT_ITERATIONS);
            break;
    }
    
    result.avg_time_ns = avg_time;
    result.min_time_ns = avg_time;  // 简化：使用平均值作为最小值
    result.max_time_ns = avg_time;  // 简化：使用平均值作为最大值
    
    LOGI("系统调用 %s 平均执行时间: %lld ns", result.name.c_str(), avg_time);
    
    return result;
}

// 执行完整的系统调用时间分析检测
SyscallTimingDetectionResult SyscallTimingDetector::detect() {
    SyscallTimingDetectionResult detection_result;
    detection_result.device_arch = getDeviceArch();
    detection_result.android_version = getAndroidVersion();
    
    LOGI("开始系统调用时间分析检测: arch=%s, android=%s", 
         detection_result.device_arch.c_str(), 
         detection_result.android_version.c_str());
    
    // 获取基准数据
    BaselineEntry baseline = getCurrentBaseline();
    detection_result.baseline_found = true;
    
    // 测量所有系统调用
    std::vector<SyscallType> syscall_types = {
        SyscallType::OPENAT,
        SyscallType::READLINKAT,
        SyscallType::GETDENTS64,
        SyscallType::STAT
    };
    
    bool any_anomaly = false;
    std::string anomaly_details;
    
    for (auto type : syscall_types) {
        SyscallTimingResult result = measureSyscall(type);
        
        // 获取对应的基准时间
        long long baseline_time = 0;
        switch (type) {
            case SyscallType::OPENAT:
                baseline_time = baseline.openat_ns;
                break;
            case SyscallType::READLINKAT:
                baseline_time = baseline.readlinkat_ns;
                break;
            case SyscallType::GETDENTS64:
                baseline_time = baseline.getdents64_ns;
                break;
            case SyscallType::STAT:
                baseline_time = baseline.stat_ns;
                break;
        }
        
        // 检测异常
        std::string reason;
        if (detectAnomaly(type, result.avg_time_ns, baseline_time, reason)) {
            result.is_anomaly = true;
            result.anomaly_reason = reason;
            any_anomaly = true;
            anomaly_details += result.name + ": " + reason + "\n";
        }
        
        detection_result.syscall_results.push_back(result);
    }
    
    // 构建总体结果
    detection_result.overall_result.layer = "第5层：系统调用时间分析";
    
    if (any_anomaly) {
        detection_result.overall_result.detected = true;
        detection_result.overall_result.detail = "检测到系统调用时间异常，可能被 Hook：\n" + anomaly_details;
        LOGW("检测到系统调用时间异常");
    } else {
        detection_result.overall_result.detected = false;
        detection_result.overall_result.detail = "系统调用时间正常，未检测到 Hook 痕迹";
        LOGI("系统调用时间正常");
    }
    
    return detection_result;
}
