#ifndef ROOTDETECTOR_SELINUX_JUMP_DETECTOR_H
#define ROOTDETECTOR_SELINUX_JUMP_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>
#include <chrono>
#include <mutex>

// 检测阶段枚举
enum class DetectionPhase {
    APPLICATION_ONCREATE,   // Application.onCreate() 阶段
    FIRST_ACTIVITY_CREATE,  // 第一个 Activity 创建阶段
    DETECTION_START,        // 检测开始时
    DETECTION_RUNNING,      // 检测过程中
    UNKNOWN                 // 未知阶段
};

// 单次上下文快照
struct ContextSnapshot {
    DetectionPhase phase;           // 检测阶段
    std::string context;            // SELinux 上下文
    long long timestamp_ms;         // 时间戳（毫秒）
    bool is_abnormal;               // 是否异常上下文
    std::string abnormal_reason;    // 异常原因
};

// 上下文跳变事件
struct ContextJumpEvent {
    ContextSnapshot before;         // 跳变前快照
    ContextSnapshot after;          // 跳变后快照
    std::string jump_type;          // 跳变类型描述
    bool is_shamiko_pattern;        // 是否为 Shamiko 特征
};

// /sys/fs/selinux/access 访问监控结果
struct SelinuxAccessMonitorResult {
    bool accessible;                // 文件是否可访问
    int access_count;               // 访问次数
    long long avg_access_time_ns;   // 平均访问时间（纳秒）
    bool has_anomaly_pattern;       // 是否有异常访问模式
    std::string anomaly_description;// 异常描述
};

// 完整检测结果
struct SelinuxJumpDetectionResult {
    DetectionResult overall_result;
    std::vector<ContextSnapshot> context_history;       // 上下文变化历史
    std::vector<ContextJumpEvent> jump_events;          // 跳变事件列表
    SelinuxAccessMonitorResult access_monitor;          // access 文件监控结果
    std::string initial_context;                        // 初始上下文
    std::string current_context;                        // 当前上下文
    int total_jumps;                                    // 总跳变次数
    bool has_shamiko_pattern;                           // 是否检测到 Shamiko 特征
};

class SelinuxJumpDetector {
public:
    // 执行完整的 SELinux 上下文跳变检测
    static SelinuxJumpDetectionResult detect();

    // 在指定阶段记录 SELinux 上下文
    static ContextSnapshot recordContext(DetectionPhase phase);

    // 获取上下文变化历史
    static std::vector<ContextSnapshot> getContextHistory();

    // 获取检测到的跳变事件
    static std::vector<ContextJumpEvent> getJumpEvents();

    // 重置检测状态（用于新一轮检测）
    static void reset();

    // 获取当前 SELinux 上下文
    static std::string getCurrentContext();

    // 检查上下文是否异常
    static bool isContextAbnormal(const std::string& context, std::string& reason);

    // 检测上下文跳变
    static bool detectContextJump(const ContextSnapshot& prev, const ContextSnapshot& curr,
                                  std::string& jump_type);

    // 检测 Shamiko 特征模式
    static bool detectShamikoPattern(const std::vector<ContextSnapshot>& history);

    // 监控 /sys/fs/selinux/access 文件
    static SelinuxAccessMonitorResult monitorSelinuxAccess();

    // 阶段名称转字符串
    static std::string phaseToString(DetectionPhase phase);

private:
    // 获取当前时间戳（毫秒）
    static long long getCurrentTimestampMs();

    // 检查上下文是否包含可疑关键词
    static bool containsSuspiciousKeyword(const std::string& context);

    // 判断是否为正常的 untrusted_app 上下文
    static bool isNormalAppContext(const std::string& context);

    // 历史快照存储
    static std::vector<ContextSnapshot> s_contextHistory;

    // 跳变事件存储
    static std::vector<ContextJumpEvent> s_jumpEvents;

    // 线程安全锁
    static std::mutex s_mutex;

    // 可疑上下文关键词列表
    static const std::vector<std::string> SUSPICIOUS_KEYWORDS;

    // 正常应用上下文前缀
    static const std::vector<std::string> NORMAL_CONTEXT_PREFIXES;
};

#endif // ROOTDETECTOR_SELINUX_JUMP_DETECTOR_H
