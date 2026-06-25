#include "selinux_jump_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <android/log.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <cstring>

#define LOG_TAG "SelinuxJumpDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// 静态成员初始化
std::vector<ContextSnapshot> SelinuxJumpDetector::s_contextHistory;
std::vector<ContextJumpEvent> SelinuxJumpDetector::s_jumpEvents;
std::mutex SelinuxJumpDetector::s_mutex;

const std::vector<std::string> SelinuxJumpDetector::SUSPICIOUS_KEYWORDS = {
    "magisk",
    "zygisk",
    "ksu",
    "kernelsu",
    "apatch",
    "su ",
    "root ",
    "u:r:magisk",
    "u:r:zygisk",
    "u:r:ksu",
    "u:r:kernelsu",
    "u:r:apatch",
    "u:r:su",
    "u:r:root",
    "u:r:zygote"   // zygote 上下文在应用进程中是异常的
};

const std::vector<std::string> SelinuxJumpDetector::NORMAL_CONTEXT_PREFIXES = {
    "u:r:untrusted_app",
    "u:r:untrusted_app:s0",
    "u:r:untrusted_app_dev",
    "u:r:platform_app",
    "u:r:system_app",
    "u:r:priv_app"
};

// 获取当前时间戳（毫秒）
long long SelinuxJumpDetector::getCurrentTimestampMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<long long>(ts.tv_sec) * 1000LL + ts.tv_nsec / 1000000LL;
}

// 阶段名称转字符串
std::string SelinuxJumpDetector::phaseToString(DetectionPhase phase) {
    switch (phase) {
        case DetectionPhase::APPLICATION_ONCREATE:
            return "Application.onCreate()";
        case DetectionPhase::FIRST_ACTIVITY_CREATE:
            return "First Activity.onCreate()";
        case DetectionPhase::DETECTION_START:
            return "Detection Start";
        case DetectionPhase::DETECTION_RUNNING:
            return "Detection Running";
        default:
            return "Unknown";
    }
}

// 获取当前 SELinux 上下文
std::string SelinuxJumpDetector::getCurrentContext() {
    // 方法1：通过 /proc/self/attr/current 读取
    std::ifstream context_file("/proc/self/attr/current");
    if (context_file.is_open()) {
        std::string ctx;
        std::getline(context_file, ctx);
        context_file.close();
        if (!ctx.empty()) {
            return ctx;
        }
    }

    // 方法2：通过 syscall 直接读取（绕过可能的 Hook）
    int fd = syscall(__NR_openat, AT_FDCWD, "/proc/self/attr/current",
                     O_RDONLY | O_CLOEXEC, 0);
    if (fd >= 0) {
        char buffer[256];
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // 去除末尾换行符
            std::string ctx(buffer);
            while (!ctx.empty() && (ctx.back() == '\n' || ctx.back() == '\r')) {
                ctx.pop_back();
            }
            return ctx;
        }
    }

    return "";
}

// 检查上下文是否包含可疑关键词
bool SelinuxJumpDetector::containsSuspiciousKeyword(const std::string& context) {
    std::string lower_ctx = context;
    std::transform(lower_ctx.begin(), lower_ctx.end(), lower_ctx.begin(), ::tolower);

    for (const auto& keyword : SUSPICIOUS_KEYWORDS) {
        if (lower_ctx.find(keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// 判断是否为正常的 untrusted_app 上下文
bool SelinuxJumpDetector::isNormalAppContext(const std::string& context) {
    for (const auto& prefix : NORMAL_CONTEXT_PREFIXES) {
        if (context.find(prefix) == 0) {
            return true;
        }
    }
    return false;
}

// 检查上下文是否异常
bool SelinuxJumpDetector::isContextAbnormal(const std::string& context, std::string& reason) {
    if (context.empty()) {
        reason = "无法获取 SELinux 上下文";
        return true;
    }

    // 检查是否包含可疑关键词
    if (containsSuspiciousKeyword(context)) {
        reason = "上下文包含可疑关键词: " + context;
        return true;
    }

    // 检查是否为正常的应用上下文
    if (!isNormalAppContext(context)) {
        reason = "非正常应用上下文: " + context;
        return true;
    }

    return false;
}

// 检测上下文跳变
bool SelinuxJumpDetector::detectContextJump(const ContextSnapshot& prev,
                                            const ContextSnapshot& curr,
                                            std::string& jump_type) {
    if (prev.context == curr.context) {
        return false;
    }

    // 判断跳变类型
    bool prev_normal = isNormalAppContext(prev.context);
    bool curr_normal = isNormalAppContext(curr.context);
    bool prev_suspicious = containsSuspiciousKeyword(prev.context);
    bool curr_suspicious = containsSuspiciousKeyword(curr.context);

    if (prev_normal && curr_suspicious) {
        jump_type = "正常 -> 可疑 (可能被 Root 工具注入)";
    } else if (prev_suspicious && curr_normal) {
        jump_type = "可疑 -> 正常 (Shamiko 隐藏恢复特征)";
    } else if (prev_normal && !curr_normal && !curr_suspicious) {
        jump_type = "正常 -> 异常 (上下文切换)";
    } else if (!prev_normal && curr_normal) {
        jump_type = "异常 -> 正常 (上下文恢复)";
    } else {
        jump_type = "上下文变化: " + prev.context + " -> " + curr.context;
    }

    return true;
}

// 检测 Shamiko 特征模式
bool SelinuxJumpDetector::detectShamikoPattern(const std::vector<ContextSnapshot>& history) {
    if (history.size() < 2) {
        return false;
    }

    // Shamiko 特征模式：
    // 1. 初始上下文为可疑上下文（如 magisk/zygote）
    // 2. 后续变为正常上下文（untrusted_app）
    // 3. 这种"先异常后正常"的模式是 Shamiko 隐藏后恢复的典型特征

    bool had_abnormal = false;
    bool then_became_normal = false;

    for (size_t i = 0; i < history.size(); ++i) {
        if (containsSuspiciousKeyword(history[i].context)) {
            had_abnormal = true;
        } else if (had_abnormal && isNormalAppContext(history[i].context)) {
            then_became_normal = true;
            break;
        }
    }

    if (had_abnormal && then_became_normal) {
        LOGW("检测到 Shamiko 特征模式: 先出现可疑上下文，后恢复为正常上下文");
        return true;
    }

    // 另一种 Shamiko 特征：频繁的上下文跳变
    int jump_count = 0;
    for (size_t i = 1; i < history.size(); ++i) {
        if (history[i].context != history[i - 1].context) {
            jump_count++;
        }
    }

    if (jump_count >= 3) {
        LOGW("检测到频繁上下文跳变 (%d 次)，可能是 Shamiko 活动特征", jump_count);
        return true;
    }

    return false;
}

// 在指定阶段记录 SELinux 上下文
ContextSnapshot SelinuxJumpDetector::recordContext(DetectionPhase phase) {
    std::lock_guard<std::mutex> lock(s_mutex);

    ContextSnapshot snapshot;
    snapshot.phase = phase;
    snapshot.context = getCurrentContext();
    snapshot.timestamp_ms = getCurrentTimestampMs();
    snapshot.is_abnormal = isContextAbnormal(snapshot.context, snapshot.abnormal_reason);

    LOGI("记录上下文快照 [%s]: %s%s",
         phaseToString(phase).c_str(),
         snapshot.context.c_str(),
         snapshot.is_abnormal ? " (异常)" : "");

    // 检测与上一个快照相比是否有跳变
    if (!s_contextHistory.empty()) {
        const ContextSnapshot& prev = s_contextHistory.back();
        std::string jump_type;
        if (detectContextJump(prev, snapshot, jump_type)) {
            ContextJumpEvent event;
            event.before = prev;
            event.after = snapshot;
            event.jump_type = jump_type;
            event.is_shamiko_pattern = false; // 稍后在 detect() 中统一判断

            s_jumpEvents.push_back(event);
            LOGW("检测到上下文跳变: %s", jump_type.c_str());
        }
    }

    s_contextHistory.push_back(snapshot);
    return snapshot;
}

// 获取上下文变化历史
std::vector<ContextSnapshot> SelinuxJumpDetector::getContextHistory() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_contextHistory;
}

// 获取检测到的跳变事件
std::vector<ContextJumpEvent> SelinuxJumpDetector::getJumpEvents() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_jumpEvents;
}

// 重置检测状态
void SelinuxJumpDetector::reset() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_contextHistory.clear();
    s_jumpEvents.clear();
    LOGI("SELinux 跳变检测器已重置");
}

// 监控 /sys/fs/selinux/access 文件
SelinuxAccessMonitorResult SelinuxJumpDetector::monitorSelinuxAccess() {
    SelinuxAccessMonitorResult result;
    result.accessible = false;
    result.access_count = 0;
    result.avg_access_time_ns = 0;
    result.has_anomaly_pattern = false;

    const char* access_path = "/sys/fs/selinux/access";

    // 检查文件是否存在和可访问
    struct stat statbuf;
    int stat_ret = syscall(__NR_stat, access_path, &statbuf);

    if (stat_ret != 0) {
        // 文件不存在或无法访问
        LOGD("/sys/fs/selinux/access 不存在或无法访问");
        return result;
    }

    result.accessible = true;

    // 测量多次访问时间，检测异常模式
    const int ITERATIONS = 100;
    std::vector<long long> access_times;
    access_times.reserve(ITERATIONS);

    struct timespec start_ts, end_ts;

    for (int i = 0; i < ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start_ts);

        int fd = syscall(__NR_openat, AT_FDCWD, access_path, O_RDONLY | O_CLOEXEC, 0);
        if (fd >= 0) {
            char buffer[64];
            read(fd, buffer, sizeof(buffer));
            close(fd);
            result.access_count++;
        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &end_ts);
        long long elapsed = (end_ts.tv_sec - start_ts.tv_sec) * 1000000000LL
                          + (end_ts.tv_nsec - start_ts.tv_nsec);
        access_times.push_back(elapsed);
    }

    if (access_times.empty()) {
        return result;
    }

    // 计算平均访问时间
    std::sort(access_times.begin(), access_times.end());
    // 去除极端值（取中间 80%）
    size_t start_idx = access_times.size() / 10;
    size_t end_idx = access_times.size() * 9 / 10;
    if (end_idx <= start_idx) {
        start_idx = 0;
        end_idx = access_times.size();
    }

    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; ++i) {
        sum += access_times[i];
    }
    result.avg_access_time_ns = sum / static_cast<long long>(end_idx - start_idx);

    LOGI("/sys/fs/selinux/access 平均访问时间: %lld ns (共 %d 次)",
         result.avg_access_time_ns, result.access_count);

    // 检测异常访问模式
    // 1. 访问时间异常短（可能是被 Hook 返回了伪造数据）
    if (result.avg_access_time_ns < 100) {
        result.has_anomaly_pattern = true;
        result.anomaly_description = "访问时间异常短 (" +
            std::to_string(result.avg_access_time_ns) + " ns)，可能被 Hook";
        LOGW("SELinux access 访问时间异常短");
    }

    // 2. 访问时间方差过大（可能是 Root 工具在拦截并动态决策）
    if (access_times.size() >= 2) {
        long long min_time = access_times.front();
        long long max_time = access_times.back();
        if (max_time > 0 && min_time > 0) {
            double ratio = static_cast<double>(max_time) / min_time;
            if (ratio > 10.0) {
                result.has_anomaly_pattern = true;
                result.anomaly_description += " 访问时间波动过大 (最大/最小比: " +
                    std::to_string(ratio).substr(0, 5) + ")，可能有拦截行为";
                LOGW("SELinux access 访问时间波动过大");
            }
        }
    }

    // 3. 读取文件内容检查异常
    int fd = syscall(__NR_openat, AT_FDCWD, access_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd >= 0) {
        char buffer[1024];
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string content(buffer);
            std::string lower_content = content;
            std::transform(lower_content.begin(), lower_content.end(),
                          lower_content.begin(), ::tolower);

            // 检查是否包含可疑内容
            if (lower_content.find("magisk") != std::string::npos ||
                lower_content.find("zygisk") != std::string::npos ||
                lower_content.find("kernelsu") != std::string::npos) {
                result.has_anomaly_pattern = true;
                result.anomaly_description += " 文件内容包含可疑关键词";
                LOGW("SELinux access 文件内容包含可疑关键词");
            }
        }
    }

    return result;
}

// 执行完整的 SELinux 上下文跳变检测
SelinuxJumpDetectionResult SelinuxJumpDetector::detect() {
    SelinuxJumpDetectionResult result;
    result.overall_result.layer = "第5层：SELinux 上下文跳变检测";
    result.overall_result.detected = false;
    result.total_jumps = 0;
    result.has_shamiko_pattern = false;

    LOGI("========== 开始 SELinux 上下文跳变检测 ==========");

    // 阶段1：记录检测开始时的上下文
    LOGI("阶段1: 记录检测开始时的上下文");
    ContextSnapshot start_snapshot = recordContext(DetectionPhase::DETECTION_START);
    result.initial_context = start_snapshot.context;
    result.current_context = start_snapshot.context;

    // 阶段2：记录检测过程中的上下文
    LOGI("阶段2: 记录检测过程中的上下文");
    ContextSnapshot running_snapshot = recordContext(DetectionPhase::DETECTION_RUNNING);
    result.current_context = running_snapshot.context;

    // 阶段3：获取上下文历史
    result.context_history = getContextHistory();

    // 阶段4：获取跳变事件
    result.jump_events = getJumpEvents();
    result.total_jumps = static_cast<int>(result.jump_events.size());

    // 阶段5：检测 Shamiko 特征
    result.has_shamiko_pattern = detectShamikoPattern(result.context_history);

    // 标记跳变事件中的 Shamiko 特征
    if (result.has_shamiko_pattern) {
        for (auto& event : result.jump_events) {
            // 可疑 -> 正常的跳变是 Shamiko 特征
            bool before_suspicious = containsSuspiciousKeyword(event.before.context);
            bool after_normal = isNormalAppContext(event.after.context);
            if (before_suspicious && after_normal) {
                event.is_shamiko_pattern = true;
            }
        }
    }

    // 阶段6：监控 /sys/fs/selinux/access
    LOGI("阶段3: 监控 /sys/fs/selinux/access");
    result.access_monitor = monitorSelinuxAccess();

    // 构建检测结果
    std::vector<std::string> findings;

    // 检查初始上下文是否异常
    if (start_snapshot.is_abnormal) {
        findings.push_back("初始上下文异常: " + start_snapshot.context +
                          " (" + start_snapshot.abnormal_reason + ")");
    }

    // 检查上下文跳变
    if (result.total_jumps > 0) {
        std::string jump_detail = "检测到 " + std::to_string(result.total_jumps) + " 次上下文跳变:";
        for (const auto& event : result.jump_events) {
            jump_detail += "\n  - " + event.jump_type;
            if (event.is_shamiko_pattern) {
                jump_detail += " [Shamiko 特征]";
            }
        }
        findings.push_back(jump_detail);
    }

    // 检查 Shamiko 特征
    if (result.has_shamiko_pattern) {
        findings.push_back("检测到 Shamiko 特征模式（上下文从可疑恢复为正常）");
    }

    // 检查 access 文件异常
    if (result.access_monitor.has_anomaly_pattern) {
        findings.push_back("/sys/fs/selinux/access 异常: " +
                          result.access_monitor.anomaly_description);
    }

    // 检查当前上下文是否异常
    std::string current_reason;
    if (isContextAbnormal(result.current_context, current_reason)) {
        findings.push_back("当前上下文异常: " + result.current_context +
                          " (" + current_reason + ")");
    }

    // 设置最终结果
    if (!findings.empty()) {
        result.overall_result.detected = true;
        result.overall_result.detail = "检测到 SELinux 上下文跳变异常:\n";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.overall_result.detail += std::to_string(i + 1) + ". " + findings[i];
            if (i < findings.size() - 1) {
                result.overall_result.detail += "\n";
            }
        }
        LOGW("SELinux 跳变检测完成: 发现 %zu 项异常", findings.size());
    } else {
        result.overall_result.detected = false;
        result.overall_result.detail = "SELinux 上下文正常，未检测到跳变或 Shamiko 特征\n"
                                       "当前上下文: " + result.current_context;
        LOGI("SELinux 跳变检测完成: 正常");
    }

    LOGI("========== SELinux 上下文跳变检测结束 ==========");
    return result;
}
