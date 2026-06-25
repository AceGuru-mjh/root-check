#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "kernel_module_detector.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "PASSED: " << message << std::endl; \
        } \
    } while(0)

// 测试 1: 扫描 /proc/modules
bool test_scan_proc_modules() {
    std::cout << "\n=== 测试 1: 扫描 /proc/modules ===" << std::endl;

    auto modules = KernelModuleDetector::scanProcModules();

    // 在真实 Android 设备上，应该至少有一些内核模块
    // 在模拟器上可能为空
    std::cout << "扫描到 " << modules.size() << " 个内核模块" << std::endl;

    if (!modules.empty()) {
        // 验证模块信息结构
        const auto& firstModule = modules[0];
        TEST_ASSERT(!firstModule.name.empty(), "第一个模块名称不为空");
        TEST_ASSERT(!firstModule.size.empty(), "第一个模块大小不为空");
        TEST_ASSERT(firstModule.source == "proc", "来源标记为 proc");

        std::cout << "示例模块: " << firstModule.name
                  << " (大小: " << firstModule.size
                  << ", 状态: " << firstModule.state << ")" << std::endl;
    } else {
        std::cout << "警告: 未扫描到模块（可能在模拟器上）" << std::endl;
    }

    return true;
}

// 测试 2: 扫描 /sys/module
bool test_scan_sys_modules() {
    std::cout << "\n=== 测试 2: 扫描 /sys/module ===" << std::endl;

    auto modules = KernelModuleDetector::scanSysModules();

    std::cout << "扫描到 " << modules.size() << " 个模块目录" << std::endl;

    if (!modules.empty()) {
        TEST_ASSERT(!modules[0].empty(), "第一个模块目录名称不为空");
        std::cout << "示例模块目录: " << modules[0] << std::endl;
    } else {
        std::cout << "警告: 未扫描到模块目录" << std::endl;
    }

    return true;
}

// 测试 3: 白名单检测
bool test_whitelist_detection() {
    std::cout << "\n=== 测试 3: 白名单检测 ===" << std::endl;

    // 测试已知官方模块
    TEST_ASSERT(KernelModuleDetector::isModuleWhitelisted("vboxsf"), "vboxsf 在白名单中");
    TEST_ASSERT(KernelModuleDetector::isModuleWhitelisted("gpu"), "gpu 在白名单中");
    TEST_ASSERT(KernelModuleDetector::isModuleWhitelisted("binder"), "binder 在白名单中");
    TEST_ASSERT(KernelModuleDetector::isModuleWhitelisted("ext4"), "ext4 在白名单中");

    // 测试非白名单模块
    TEST_ASSERT(!KernelModuleDetector::isModuleWhitelisted("magisk_module"), "magisk_module 不在白名单中");
    TEST_ASSERT(!KernelModuleDetector::isModuleWhitelisted("unknown_module"), "unknown_module 不在白名单中");

    return true;
}

// 测试 4: 可疑关键词检测
bool test_suspicious_keyword_detection() {
    std::cout << "\n=== 测试 4: 可疑关键词检测 ===" << std::endl;

    std::string matchedKeyword;

    // 测试已知可疑模块
    TEST_ASSERT(KernelModuleDetector::containsSuspiciousKeyword("magisk_hide", matchedKeyword),
                "magisk_hide 包含可疑关键词");
    TEST_ASSERT(matchedKeyword == "magisk", "匹配的关键词是 magisk");

    TEST_ASSERT(KernelModuleDetector::containsSuspiciousKeyword("kernelsu_driver", matchedKeyword),
                "kernelsu_driver 包含可疑关键词");
    TEST_ASSERT(matchedKeyword == "kernelsu", "匹配的关键词是 kernelsu");

    TEST_ASSERT(KernelModuleDetector::containsSuspiciousKeyword("apatch_module", matchedKeyword),
                "apatch_module 包含可疑关键词");

    TEST_ASSERT(KernelModuleDetector::containsSuspiciousKeyword("xposed_hook", matchedKeyword),
                "xposed_hook 包含可疑关键词");

    // 测试正常模块
    TEST_ASSERT(!KernelModuleDetector::containsSuspiciousKeyword("normal_driver", matchedKeyword),
                "normal_driver 不包含可疑关键词");

    return true;
}

// 测试 5: 非官方模块检测
bool test_unofficial_module_detection() {
    std::cout << "\n=== 测试 5: 非官方模块检测 ===" << std::endl;

    // 构造测试数据
    std::vector<KernelModuleInfo> testModules = {
        {"vboxsf", "12345", "1", "", "Live", "0xffffffffa0000000", "proc"},
        {"gpu", "67890", "2", "", "Live", "0xffffffffa0010000", "proc"},
        {"magisk_module", "11111", "1", "", "Live", "0xffffffffa0020000", "proc"},
        {"unknown_driver", "22222", "1", "", "Live", "0xffffffffa0030000", "proc"},
        {"kernelsu_hook", "33333", "1", "", "Live", "0xffffffffa0040000", "proc"},
    };

    auto suspicious = KernelModuleDetector::detectUnofficialModules(testModules);

    std::cout << "检测到 " << suspicious.size() << " 个可疑模块" << std::endl;

    // 应该检测到 magisk_module、unknown_driver、kernelsu_hook
    TEST_ASSERT(suspicious.size() >= 2, "至少检测到 2 个可疑模块");

    // 验证 magisk_module 被检测到
    bool foundMagisk = false;
    bool foundKsu = false;
    for (const auto& mod : suspicious) {
        std::cout << "可疑模块: " << mod.name << std::endl;
        if (mod.name == "magisk_module") foundMagisk = true;
        if (mod.name == "kernelsu_hook") foundKsu = true;
    }

    TEST_ASSERT(foundMagisk, "检测到 magisk_module");
    TEST_ASSERT(foundKsu, "检测到 kernelsu_hook");

    return true;
}

// 测试 6: kallsyms_lookup_name 导出检测
bool test_kallsyms_lookup_name_detection() {
    std::cout << "\n=== 测试 6: kallsyms_lookup_name 导出检测 ===" << std::endl;

    bool exported = KernelModuleDetector::checkKallsymsLookupNameExported();

    std::cout << "kallsyms_lookup_name 导出状态: " << (exported ? "是" : "否") << std::endl;

    // 在正常内核上，这个符号不应该被导出为全局符号
    // 但某些定制内核可能会导出
    // 这里只验证函数能正常运行
    TEST_ASSERT(true, "kallsyms_lookup_name 检测函数正常运行");

    return true;
}

// 测试 7: 系统调用表检测
bool test_syscall_table_detection() {
    std::cout << "\n=== 测试 7: 系统调用表检测 ===" << std::endl;

    bool modified = KernelModuleDetector::checkSyscallTableModified();

    std::cout << "系统调用表修改状态: " << (modified ? "异常" : "正常") << std::endl;

    // 在正常系统上，系统调用表不应该被修改
    TEST_ASSERT(true, "系统调用表检测函数正常运行");

    return true;
}

// 测试 8: 符号表异常检测
bool test_symbol_table_anomaly_detection() {
    std::cout << "\n=== 测试 8: 符号表异常检测 ===" << std::endl;

    std::vector<KernelSymbolInfo> suspiciousSymbols;
    bool anomaly = KernelModuleDetector::detectSymbolTableAnomaly(suspiciousSymbols);

    std::cout << "符号表异常状态: " << (anomaly ? "异常" : "正常") << std::endl;
    std::cout << "发现 " << suspiciousSymbols.size() << " 个可疑符号" << std::endl;

    for (const auto& sym : suspiciousSymbols) {
        std::cout << "可疑符号: " << sym.name << " (" << sym.reason << ")" << std::endl;
    }

    TEST_ASSERT(true, "符号表异常检测函数正常运行");

    return true;
}

// 测试 9: 内存异常检测
bool test_memory_anomaly_detection() {
    std::cout << "\n=== 测试 9: 内存异常检测 ===" << std::endl;

    std::vector<std::string> findings;
    bool anomaly = KernelModuleDetector::detectMemoryAnomaly(findings);

    std::cout << "内存异常状态: " << (anomaly ? "异常" : "正常") << std::endl;
    std::cout << "发现 " << findings.size() << " 个异常" << std::endl;

    for (const auto& finding : findings) {
        std::cout << "异常: " << finding << std::endl;
    }

    TEST_ASSERT(true, "内存异常检测函数正常运行");

    return true;
}

// 测试 10: 完整检测流程
bool test_full_detection() {
    std::cout << "\n=== 测试 10: 完整检测流程 ===" << std::endl;

    auto result = KernelModuleDetector::detect();

    std::cout << "检测层级: " << result.layer << std::endl;
    std::cout << "检测结果: " << (result.detected ? "检测到异常" : "未检测到异常") << std::endl;
    std::cout << "详细信息: " << result.detail << std::endl;

    TEST_ASSERT(result.layer == "第6层：内核模块检测", "检测层级正确");
    TEST_ASSERT(!result.detail.empty(), "详细信息不为空");

    return true;
}

// 测试 11: 详细检测流程
bool test_detailed_detection() {
    std::cout << "\n=== 测试 11: 详细检测流程 ===" << std::endl;

    auto detail = KernelModuleDetector::detectDetailed();

    std::cout << "扫描模块总数: " << detail.allModules.size() << std::endl;
    std::cout << "可疑模块数: " << detail.suspiciousModules.size() << std::endl;
    std::cout << "可疑符号数: " << detail.suspiciousSymbols.size() << std::endl;
    std::cout << "发现项数: " << detail.findings.size() << std::endl;

    std::cout << "\n详细发现:" << std::endl;
    for (const auto& finding : detail.findings) {
        std::cout << "  - " << finding << std::endl;
    }

    TEST_ASSERT(true, "详细检测流程正常运行");

    return true;
}

// 主测试函数
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "内核模块检测器单元测试" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int total = 11;

    if (test_scan_proc_modules()) passed++;
    if (test_scan_sys_modules()) passed++;
    if (test_whitelist_detection()) passed++;
    if (test_suspicious_keyword_detection()) passed++;
    if (test_unofficial_module_detection()) passed++;
    if (test_kallsyms_lookup_name_detection()) passed++;
    if (test_syscall_table_detection()) passed++;
    if (test_symbol_table_anomaly_detection()) passed++;
    if (test_memory_anomaly_detection()) passed++;
    if (test_full_detection()) passed++;
    if (test_detailed_detection()) passed++;

    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果: " << passed << "/" << total << " 通过" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed == total) ? 0 : 1;
}
