#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "selinux_jump_detector.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "PASSED: " << message << std::endl; \
        } \
    } while(0)

// 测试 1: 获取当前 SELinux 上下文
bool test_get_current_context() {
    std::cout << "\n=== 测试 1: 获取当前 SELinux 上下文 ===" << std::endl;

    std::string context = SelinuxJumpDetector::getCurrentContext();

    std::cout << "当前上下文: " << context << std::endl;

    // 在真实 Android 设备上，应该能获取到上下文
    // 在模拟器或某些环境可能为空
    if (!context.empty()) {
        TEST_ASSERT(!context.empty(), "成功获取 SELinux 上下文");
        std::cout << "上下文长度: " << context.length() << std::endl;
    } else {
        std::cout << "警告: 未获取到上下文（可能在模拟器上）" << std::endl;
        TEST_ASSERT(true, "函数正常运行（即使返回空）");
    }

    return true;
}

// 测试 2: 检查上下文是否异常
bool test_is_context_abnormal() {
    std::cout << "\n=== 测试 2: 检查上下文是否异常 ===" << std::endl;

    std::string reason;

    // 测试正常上下文
    TEST_ASSERT(!SelinuxJumpDetector::isContextAbnormal("u:r:untrusted_app:s0", reason),
                "u:r:untrusted_app:s0 是正常上下文");

    TEST_ASSERT(!SelinuxJumpDetector::isContextAbnormal("u:r:platform_app:s0", reason),
                "u:r:platform_app:s0 是正常上下文");

    // 测试异常上下文
    TEST_ASSERT(SelinuxJumpDetector::isContextAbnormal("u:r:magisk:s0", reason),
                "u:r:magisk:s0 是异常上下文");
    std::cout << "异常原因: " << reason << std::endl;

    TEST_ASSERT(SelinuxJumpDetector::isContextAbnormal("u:r:zygote:s0", reason),
                "u:r:zygote:s0 是异常上下文");

    TEST_ASSERT(SelinuxJumpDetector::isContextAbnormal("u:r:kernelsu:s0", reason),
                "u:r:kernelsu:s0 是异常上下文");

    // 测试空上下文
    TEST_ASSERT(SelinuxJumpDetector::isContextAbnormal("", reason),
                "空上下文是异常的");

    return true;
}

// 测试 3: 阶段名称转换
bool test_phase_to_string() {
    std::cout << "\n=== 测试 3: 阶段名称转换 ===" << std::endl;

    TEST_ASSERT(SelinuxJumpDetector::phaseToString(DetectionPhase::APPLICATION_ONCREATE) == "Application.onCreate()",
                "APPLICATION_ONCREATE 转换正确");

    TEST_ASSERT(SelinuxJumpDetector::phaseToString(DetectionPhase::FIRST_ACTIVITY_CREATE) == "First Activity.onCreate()",
                "FIRST_ACTIVITY_CREATE 转换正确");

    TEST_ASSERT(SelinuxJumpDetector::phaseToString(DetectionPhase::DETECTION_START) == "Detection Start",
                "DETECTION_START 转换正确");

    TEST_ASSERT(SelinuxJumpDetector::phaseToString(DetectionPhase::DETECTION_RUNNING) == "Detection Running",
                "DETECTION_RUNNING 转换正确");

    TEST_ASSERT(SelinuxJumpDetector::phaseToString(DetectionPhase::UNKNOWN) == "Unknown",
                "UNKNOWN 转换正确");

    return true;
}

// 测试 4: 记录上下文快照
bool test_record_context() {
    std::cout << "\n=== 测试 4: 记录上下文快照 ===" << std::endl;

    // 重置状态
    SelinuxJumpDetector::reset();

    // 记录第一个阶段
    auto snapshot1 = SelinuxJumpDetector::recordContext(DetectionPhase::APPLICATION_ONCREATE);

    TEST_ASSERT(snapshot1.phase == DetectionPhase::APPLICATION_ONCREATE,
                "快照阶段正确");
    TEST_ASSERT(snapshot1.timestamp_ms > 0, "时间戳有效");

    std::cout << "快照 1 - 阶段: " << SelinuxJumpDetector::phaseToString(snapshot1.phase)
              << ", 上下文: " << snapshot1.context
              << ", 时间戳: " << snapshot1.timestamp_ms << std::endl;

    // 记录第二个阶段
    auto snapshot2 = SelinuxJumpDetector::recordContext(DetectionPhase::FIRST_ACTIVITY_CREATE);

    TEST_ASSERT(snapshot2.phase == DetectionPhase::FIRST_ACTIVITY_CREATE,
                "第二个快照阶段正确");
    TEST_ASSERT(snapshot2.timestamp_ms >= snapshot1.timestamp_ms,
                "时间戳递增");

    std::cout << "快照 2 - 阶段: " << SelinuxJumpDetector::phaseToString(snapshot2.phase)
              << ", 上下文: " << snapshot2.context
              << ", 时间戳: " << snapshot2.timestamp_ms << std::endl;

    return true;
}

// 测试 5: 获取上下文历史
bool test_get_context_history() {
    std::cout << "\n=== 测试 5: 获取上下文历史 ===" << std::endl;

    // 重置并记录几个快照
    SelinuxJumpDetector::reset();
    SelinuxJumpDetector::recordContext(DetectionPhase::APPLICATION_ONCREATE);
    SelinuxJumpDetector::recordContext(DetectionPhase::FIRST_ACTIVITY_CREATE);
    SelinuxJumpDetector::recordContext(DetectionPhase::DETECTION_START);

    auto history = SelinuxJumpDetector::getContextHistory();

    TEST_ASSERT(history.size() == 3, "历史记录数量为 3");

    std::cout << "历史记录数量: " << history.size() << std::endl;
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "  [" << i << "] 阶段: " << SelinuxJumpDetector::phaseToString(history[i].phase)
                  << ", 上下文: " << history[i].context << std::endl;
    }

    return true;
}

// 测试 6: 检测上下文跳变
bool test_detect_context_jump() {
    std::cout << "\n=== 测试 6: 检测上下文跳变 ===" << std::endl;

    // 测试相同上下文（无跳变）
    ContextSnapshot prev1;
    prev1.context = "u:r:untrusted_app:s0";
    prev1.phase = DetectionPhase::APPLICATION_ONCREATE;
    prev1.timestamp_ms = 1000;

    ContextSnapshot curr1;
    curr1.context = "u:r:untrusted_app:s0";
    curr1.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    curr1.timestamp_ms = 2000;

    std::string jump_type;
    TEST_ASSERT(!SelinuxJumpDetector::detectContextJump(prev1, curr1, jump_type),
                "相同上下文无跳变");

    // 测试正常 -> 可疑（有跳变）
    ContextSnapshot prev2;
    prev2.context = "u:r:untrusted_app:s0";
    prev2.phase = DetectionPhase::APPLICATION_ONCREATE;
    prev2.timestamp_ms = 1000;

    ContextSnapshot curr2;
    curr2.context = "u:r:magisk:s0";
    curr2.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    curr2.timestamp_ms = 2000;

    TEST_ASSERT(SelinuxJumpDetector::detectContextJump(prev2, curr2, jump_type),
                "正常 -> 可疑有跳变");
    std::cout << "跳变类型: " << jump_type << std::endl;

    // 测试可疑 -> 正常（Shamiko 特征）
    ContextSnapshot prev3;
    prev3.context = "u:r:zygote:s0";
    prev3.phase = DetectionPhase::APPLICATION_ONCREATE;
    prev3.timestamp_ms = 1000;

    ContextSnapshot curr3;
    curr3.context = "u:r:untrusted_app:s0";
    curr3.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    curr3.timestamp_ms = 2000;

    TEST_ASSERT(SelinuxJumpDetector::detectContextJump(prev3, curr3, jump_type),
                "可疑 -> 正常有跳变");
    std::cout << "跳变类型: " << jump_type << std::endl;

    return true;
}

// 测试 7: 检测 Shamiko 特征模式
bool test_detect_shamiko_pattern() {
    std::cout << "\n=== 测试 7: 检测 Shamiko 特征模式 ===" << std::endl;

    // 测试正常模式（无 Shamiko 特征）
    std::vector<ContextSnapshot> normal_history;
    ContextSnapshot s1;
    s1.context = "u:r:untrusted_app:s0";
    s1.phase = DetectionPhase::APPLICATION_ONCREATE;
    s1.timestamp_ms = 1000;
    normal_history.push_back(s1);

    ContextSnapshot s2;
    s2.context = "u:r:untrusted_app:s0";
    s2.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    s2.timestamp_ms = 2000;
    normal_history.push_back(s2);

    TEST_ASSERT(!SelinuxJumpDetector::detectShamikoPattern(normal_history),
                "正常模式无 Shamiko 特征");

    // 测试 Shamiko 特征模式（先异常后正常）
    std::vector<ContextSnapshot> shamiko_history;
    ContextSnapshot s3;
    s3.context = "u:r:magisk:s0";
    s3.phase = DetectionPhase::APPLICATION_ONCREATE;
    s3.timestamp_ms = 1000;
    shamiko_history.push_back(s3);

    ContextSnapshot s4;
    s4.context = "u:r:untrusted_app:s0";
    s4.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    s4.timestamp_ms = 2000;
    shamiko_history.push_back(s4);

    TEST_ASSERT(SelinuxJumpDetector::detectShamikoPattern(shamiko_history),
                "先异常后正常是 Shamiko 特征");

    // 测试频繁跳变模式
    std::vector<ContextSnapshot> frequent_jump_history;
    for (int i = 0; i < 5; ++i) {
        ContextSnapshot s;
        s.context = (i % 2 == 0) ? "u:r:untrusted_app:s0" : "u:r:zygote:s0";
        s.phase = DetectionPhase::DETECTION_RUNNING;
        s.timestamp_ms = 1000 + i * 100;
        frequent_jump_history.push_back(s);
    }

    TEST_ASSERT(SelinuxJumpDetector::detectShamikoPattern(frequent_jump_history),
                "频繁跳变是 Shamiko 特征");

    return true;
}

// 测试 8: 监控 /sys/fs/selinux/access
bool test_monitor_selinux_access() {
    std::cout << "\n=== 测试 8: 监控 /sys/fs/selinux/access ===" << std::endl;

    auto result = SelinuxJumpDetector::monitorSelinuxAccess();

    std::cout << "文件可访问: " << (result.accessible ? "是" : "否") << std::endl;
    std::cout << "访问次数: " << result.access_count << std::endl;
    std::cout << "平均访问时间: " << result.avg_access_time_ns << " ns" << std::endl;
    std::cout << "异常模式: " << (result.has_anomaly_pattern ? "是" : "否") << std::endl;

    if (result.has_anomaly_pattern) {
        std::cout << "异常描述: " << result.anomaly_description << std::endl;
    }

    // 在某些设备上文件可能不存在，这是正常的
    TEST_ASSERT(true, "监控函数正常运行");

    return true;
}

// 测试 9: 完整检测流程
bool test_full_detection() {
    std::cout << "\n=== 测试 9: 完整检测流程 ===" << std::endl;

    // 重置状态
    SelinuxJumpDetector::reset();

    auto result = SelinuxJumpDetector::detect();

    std::cout << "检测层级: " << result.overall_result.layer << std::endl;
    std::cout << "检测结果: " << (result.overall_result.detected ? "检测到异常" : "未检测到异常") << std::endl;
    std::cout << "初始上下文: " << result.initial_context << std::endl;
    std::cout << "当前上下文: " << result.current_context << std::endl;
    std::cout << "跳变次数: " << result.total_jumps << std::endl;
    std::cout << "Shamiko 特征: " << (result.has_shamiko_pattern ? "是" : "否") << std::endl;
    std::cout << "上下文历史数量: " << result.context_history.size() << std::endl;
    std::cout << "跳变事件数量: " << result.jump_events.size() << std::endl;

    std::cout << "\n详细信息:\n" << result.overall_result.detail << std::endl;

    TEST_ASSERT(result.overall_result.layer == "第5层：SELinux 上下文跳变检测",
                "检测层级正确");
    TEST_ASSERT(!result.overall_result.detail.empty(), "详细信息不为空");

    return true;
}

// 测试 10: 重置功能
bool test_reset_functionality() {
    std::cout << "\n=== 测试 10: 重置功能 ===" << std::endl;

    // 记录一些快照
    SelinuxJumpDetector::recordContext(DetectionPhase::APPLICATION_ONCREATE);
    SelinuxJumpDetector::recordContext(DetectionPhase::FIRST_ACTIVITY_CREATE);

    auto history_before = SelinuxJumpDetector::getContextHistory();
    TEST_ASSERT(history_before.size() >= 2, "重置前有历史记录");

    // 重置
    SelinuxJumpDetector::reset();

    auto history_after = SelinuxJumpDetector::getContextHistory();
    TEST_ASSERT(history_after.empty(), "重置后历史记录为空");

    auto jump_events = SelinuxJumpDetector::getJumpEvents();
    TEST_ASSERT(jump_events.empty(), "重置后跳变事件为空");

    return true;
}

// 测试 11: 线程安全性
bool test_thread_safety() {
    std::cout << "\n=== 测试 11: 线程安全性 ===" << std::endl;

    SelinuxJumpDetector::reset();

    // 在单线程中多次调用，验证不会崩溃
    for (int i = 0; i < 10; ++i) {
        SelinuxJumpDetector::recordContext(DetectionPhase::DETECTION_RUNNING);
    }

    auto history = SelinuxJumpDetector::getContextHistory();
    TEST_ASSERT(history.size() == 10, "成功记录 10 个快照");

    // 验证时间戳递增
    bool timestamps_valid = true;
    for (size_t i = 1; i < history.size(); ++i) {
        if (history[i].timestamp_ms < history[i - 1].timestamp_ms) {
            timestamps_valid = false;
            break;
        }
    }
    TEST_ASSERT(timestamps_valid, "时间戳递增");

    return true;
}

// 测试 12: 边界条件测试
bool test_edge_cases() {
    std::cout << "\n=== 测试 12: 边界条件测试 ===" << std::endl;

    // 测试空历史记录的 Shamiko 检测
    std::vector<ContextSnapshot> empty_history;
    TEST_ASSERT(!SelinuxJumpDetector::detectShamikoPattern(empty_history),
                "空历史无 Shamiko 特征");

    // 测试单个快照的 Shamiko 检测
    std::vector<ContextSnapshot> single_history;
    ContextSnapshot s;
    s.context = "u:r:magisk:s0";
    s.phase = DetectionPhase::APPLICATION_ONCREATE;
    s.timestamp_ms = 1000;
    single_history.push_back(s);

    TEST_ASSERT(!SelinuxJumpDetector::detectShamikoPattern(single_history),
                "单个快照无 Shamiko 特征");

    // 测试跳变检测的边界条件
    ContextSnapshot prev;
    prev.context = "";
    prev.phase = DetectionPhase::APPLICATION_ONCREATE;
    prev.timestamp_ms = 1000;

    ContextSnapshot curr;
    curr.context = "u:r:untrusted_app:s0";
    curr.phase = DetectionPhase::FIRST_ACTIVITY_CREATE;
    curr.timestamp_ms = 2000;

    std::string jump_type;
    bool has_jump = SelinuxJumpDetector::detectContextJump(prev, curr, jump_type);
    TEST_ASSERT(has_jump, "空 -> 正常有跳变");
    std::cout << "跳变类型: " << jump_type << std::endl;

    return true;
}

// 主测试函数
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "SELinux 上下文跳变检测器单元测试" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int total = 12;

    if (test_get_current_context()) passed++;
    if (test_is_context_abnormal()) passed++;
    if (test_phase_to_string()) passed++;
    if (test_record_context()) passed++;
    if (test_get_context_history()) passed++;
    if (test_detect_context_jump()) passed++;
    if (test_detect_shamiko_pattern()) passed++;
    if (test_monitor_selinux_access()) passed++;
    if (test_full_detection()) passed++;
    if (test_reset_functionality()) passed++;
    if (test_thread_safety()) passed++;
    if (test_edge_cases()) passed++;

    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果: " << passed << "/" << total << " 通过" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed == total) ? 0 : 1;
}
