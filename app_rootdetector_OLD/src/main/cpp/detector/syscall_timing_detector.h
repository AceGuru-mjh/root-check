#ifndef ROOTDETECTOR_SYSCALL_TIMING_DETECTOR_H
#define ROOTDETECTOR_SYSCALL_TIMING_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>
#include <map>

// 系统调用类型枚举
enum class SyscallType {
    OPENAT,
    READLINKAT,
    GETDENTS64,
    STAT
};

// 单个系统调用的时间测量结果
struct SyscallTimingResult {
    SyscallType type;
    std::string name;
    long long avg_time_ns;      // 平均执行时间（纳秒）
    long long min_time_ns;      // 最小执行时间
    long long max_time_ns;      // 最大执行时间
    bool is_anomaly;            // 是否异常
    std::string anomaly_reason; // 异常原因
};

// 基准数据条目
struct BaselineEntry {
    std::string arch;           // CPU 架构：arm64-v8a, armeabi-v7a
    std::string android_version; // Android 版本
    long long openat_ns;        // openat 基准时间
    long long readlinkat_ns;    // readlinkat 基准时间
    long long getdents64_ns;    // getdents64 基准时间
    long long stat_ns;          // stat 基准时间
};

// 系统调用时间分析检测结果
struct SyscallTimingDetectionResult {
    DetectionResult overall_result;
    std::vector<SyscallTimingResult> syscall_results;
    std::string device_arch;
    std::string android_version;
    bool baseline_found;
};

class SyscallTimingDetector {
public:
    // 执行完整的系统调用时间分析检测
    static SyscallTimingDetectionResult detect();
    
    // 测量单个系统调用的执行时间
    static SyscallTimingResult measureSyscall(SyscallType type);
    
    // 获取当前设备的基准数据
    static BaselineEntry getCurrentBaseline();
    
    // 检测时间异常
    static bool detectAnomaly(SyscallType type, long long measured_time_ns, 
                              long long baseline_time_ns, std::string& reason);

private:
    // 高精度时间获取（纳秒）
    static long long getTimeNs();
    
    // 测量 openat 系统调用时间
    static long long measureOpenat(int iterations);
    
    // 测量 readlinkat 系统调用时间
    static long long measureReadlinkat(int iterations);
    
    // 测量 getdents64 系统调用时间
    static long long measureGetdents64(int iterations);
    
    // 测量 stat 系统调用时间
    static long long measureStat(int iterations);
    
    // 获取基准数据库
    static std::map<std::string, BaselineEntry> getBaselineDatabase();
    
    // 获取设备 CPU 架构
    static std::string getDeviceArch();
    
    // 获取 Android 版本
    static std::string getAndroidVersion();
    
    // 查找匹配的基准数据
    static bool findBaseline(const std::string& arch, const std::string& version, 
                            BaselineEntry& baseline);
    
    // 系统调用类型转字符串
    static std::string syscallTypeToString(SyscallType type);
    
    // 默认测量迭代次数
    static constexpr int DEFAULT_ITERATIONS = 1000;
    
    // 异常阈值倍数（3 倍）
    static constexpr double ANOMALY_THRESHOLD = 3.0;
};

#endif // ROOTDETECTOR_SYSCALL_TIMING_DETECTOR_H
