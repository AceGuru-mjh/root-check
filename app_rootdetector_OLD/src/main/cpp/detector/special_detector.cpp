#include "special_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "SpecialDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// ==================== 辅助方法实现 ====================

bool SpecialDetector::checkDirectory(const std::string& path) {
    return SyscallUtils::fileExistsDirect(path);
}

bool SpecialDetector::checkMemoryPattern(const std::string& pattern) {
    // 在 /proc/self/maps 中搜索特定模式
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }
    
    std::string line;
    std::string lowerPattern = pattern;
    std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
    
    while (std::getline(maps, line)) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        
        if (lowerLine.find(lowerPattern) != std::string::npos) {
            LOGI("在内存映射中发现模式: %s in %s", pattern.c_str(), line.c_str());
            return true;
        }
    }
    
    return false;
}

bool SpecialDetector::checkSocket(const std::string& socketPath) {
    return SyscallUtils::fileExistsDirect(socketPath);
}

bool SpecialDetector::checkKernelVersion(const std::string& keyword) {
    std::ifstream version("/proc/version");
    if (!version.is_open()) {
        return false;
    }
    
    std::string versionStr;
    std::getline(version, versionStr);
    
    std::string lowerVersion = versionStr;
    std::string lowerKeyword = keyword;
    std::transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), ::tolower);
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
    
    if (lowerVersion.find(lowerKeyword) != std::string::npos) {
        LOGI("在内核版本中发现关键词: %s in %s", keyword.c_str(), versionStr.c_str());
        return true;
    }
    
    return false;
}

bool SpecialDetector::checkMapsForPattern(const std::string& pattern) {
    return checkMemoryPattern(pattern);
}

bool SpecialDetector::checkZygoteInjection() {
    // 检测 Zygote 进程注入特征
    // 检查 /proc/self/maps 中是否有异常的 zygote 相关映射
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(maps, line)) {
        // 查找 zygote 相关的可执行映射
        if ((line.find("zygote") != std::string::npos || 
             line.find("Zygote") != std::string::npos) &&
            (line.find("r-xp") != std::string::npos || 
             line.find("rwxp") != std::string::npos)) {
            
            // 检查是否是异常的注入路径
            if (line.find("/data/") != std::string::npos ||
                line.find("/dev/") != std::string::npos ||
                line.find("/cache/") != std::string::npos) {
                LOGI("检测到 Zygote 注入特征: %s", line.c_str());
                return true;
            }
        }
    }
    
    return false;
}

bool SpecialDetector::checkOverlayfsMounts() {
    // 检测 overlayfs 挂载点（KernelSU 特征）
    std::string mountInfo = SyscallUtils::readFileDirect("/proc/mounts");
    
    // 检查是否有可疑的 overlay 挂载
    if (mountInfo.find("overlay") != std::string::npos ||
        mountInfo.find("overlayfs") != std::string::npos) {
        
        // 检查是否涉及系统分区
        if (mountInfo.find("/system") != std::string::npos ||
            mountInfo.find("/vendor") != std::string::npos ||
            mountInfo.find("/product") != std::string::npos ||
            mountInfo.find("/system_ext") != std::string::npos) {
            LOGI("检测到可疑的 overlayfs 挂载");
            return true;
        }
    }
    
    return false;
}

bool SpecialDetector::checkArtHookTraces() {
    // 检测 ART 虚拟机 Hook 痕迹
    // 检查 /proc/self/maps 中是否有异常的 ART 相关库
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(maps, line)) {
        // 查找 ART Hook 相关的库
        if (line.find("libart.so") != std::string::npos ||
            line.find("libart-compiler.so") != std::string::npos) {
            
            // 检查是否来自异常路径
            if (line.find("/data/") != std::string::npos ||
                line.find("/dev/") != std::string::npos) {
                LOGI("检测到 ART Hook 痕迹: %s", line.c_str());
                return true;
            }
        }
        
        // 检查 Xposed/LSPosed 相关的 Hook 库
        if (line.find("libxposed") != std::string::npos ||
            line.find("liblspd") != std::string::npos ||
            line.find("libfrida") != std::string::npos ||
            line.find("libsubstrate") != std::string::npos) {
            LOGI("检测到 Hook 框架库: %s", line.c_str());
            return true;
        }
    }
    
    return false;
}

bool SpecialDetector::checkBinderAnomalies() {
    // 检测异常 Binder 调用
    // 检查 /proc/self/binder 相关文件
    std::ifstream binder("/proc/self/binder");
    if (binder.is_open()) {
        // 正常情况下不应该能直接读取 binder 文件
        LOGW("检测到异常的 Binder 访问");
        return true;
    }
    
    // 检查 binder 上下文中的异常
    std::string cmdline = SyscallUtils::readFileDirect("/proc/self/cmdline");
    if (cmdline.find("binder") != std::string::npos &&
        cmdline.find("system_server") == std::string::npos) {
        // 非 system_server 进程却有 binder 相关 cmdline
        LOGI("检测到异常的 Binder cmdline: %s", cmdline.c_str());
        return true;
    }
    
    return false;
}

bool SpecialDetector::checkSelinuxContextChanges() {
    // 检测 SELinux 上下文异常变化
    std::string context = SyscallUtils::readFileDirect("/proc/self/attr/current");
    
    // 检查是否是异常的 SELinux 上下文
    if (context.find("magisk") != std::string::npos ||
        context.find("ksu") != std::string::npos ||
        context.find("shamiko") != std::string::npos ||
        context.find("lspd") != std::string::npos) {
        LOGI("检测到 SELinux 上下文异常: %s", context.c_str());
        return true;
    }
    
    // 检查是否是 permissive 模式
    std::string enforce = SyscallUtils::readFileDirect("/sys/fs/selinux/enforce");
    if (enforce == "0") {
        LOGW("SELinux 处于 permissive 模式");
        return true;
    }
    
    return false;
}

bool SpecialDetector::checkSyscallHookPatterns() {
    // 检测系统调用 Hook 模式（Shamiko 独特模式）
    // 检查 /proc/self/maps 中是否有可疑的 Hook 库
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(maps, line)) {
        // Shamiko 特征：特定的 Hook 库
        if (line.find("libshamiko") != std::string::npos ||
            line.find("libriru") != std::string::npos ||
            line.find("libzygisk") != std::string::npos) {
            LOGI("检测到系统调用 Hook 模式: %s", line.c_str());
            return true;
        }
        
        // 检查是否有异常的 libc 映射（可能被 Hook）
        if (line.find("libc.so") != std::string::npos &&
            line.find("/system/") == std::string::npos &&
            line.find("/apex/") == std::string::npos) {
            LOGI("检测到异常的 libc 映射: %s", line.c_str());
            return true;
        }
    }
    
    return false;
}

bool SpecialDetector::checkZygiskModuleList() {
    // 检测 Zygisk 模块列表中的 Shamiko
    std::string modulesPath = "/data/adb/modules";
    
    // 列出模块目录
    auto modules = SyscallUtils::listFilesDirect(modulesPath);
    
    for (const auto& module : modules) {
        if (module.find("shamiko") != std::string::npos ||
            module.find("Shamiko") != std::string::npos) {
            LOGI("在 Zygisk 模块列表中发现 Shamiko: %s", module.c_str());
            return true;
        }
    }
    
    return false;
}

// ==================== Magisk 专项检测 ====================

SpecialDetectionResult SpecialDetector::detectMagisk() {
    SpecialDetectionResult result;
    result.toolName = "Magisk";
    result.detected = false;
    
    // 检测项 1: /data/adb/magisk 目录存在性
    if (checkDirectory("/data/adb/magisk")) {
        result.evidences.push_back("Magisk 目录存在: /data/adb/magisk");
        LOGI("Magisk 检测: 发现 Magisk 目录");
    }
    
    // 检测项 2: 内存中 libmagisk.so 特征
    if (checkMemoryPattern("libmagisk.so")) {
        result.evidences.push_back("内存中发现 libmagisk.so");
        LOGI("Magisk 检测: 发现 libmagisk.so");
    }
    
    // 检测项 3: Magisk 套接字
    if (checkSocket("/dev/socket/magisk")) {
        result.evidences.push_back("Magisk 套接字存在: /dev/socket/magisk");
        LOGI("Magisk 检测: 发现 magisk 套接字");
    }
    
    if (checkSocket("/dev/socket/magiskd")) {
        result.evidences.push_back("Magisk 守护进程套接字存在: /dev/socket/magiskd");
        LOGI("Magisk 检测: 发现 magiskd 套接字");
    }
    
    // 检测项 4: /proc/self/maps 中的 Magisk 相关映射
    if (checkMapsForPattern("magisk")) {
        result.evidences.push_back("内存映射中发现 Magisk 相关映射");
        LOGI("Magisk 检测: 发现 Magisk 内存映射");
    }
    
    // 设置检测结果
    result.detected = !result.evidences.empty();
    
    if (result.detected) {
        LOGI("Magisk 检测完成: 发现 %zu 个证据", result.evidences.size());
    } else {
        LOGI("Magisk 检测完成: 未发现证据");
    }
    
    return result;
}

// ==================== Zygisk 专项检测 ====================

SpecialDetectionResult SpecialDetector::detectZygisk() {
    SpecialDetectionResult result;
    result.toolName = "Zygisk";
    result.detected = false;
    
    // 检测项 1: 内存中 libzygisk.so 特征
    if (checkMemoryPattern("libzygisk.so")) {
        result.evidences.push_back("内存中发现 libzygisk.so");
        LOGI("Zygisk 检测: 发现 libzygisk.so");
    }
    
    // 检测项 2: Zygote 进程注入检测
    if (checkZygoteInjection()) {
        result.evidences.push_back("检测到 Zygote 进程注入特征");
        LOGI("Zygisk 检测: 发现 Zygote 注入");
    }
    
    // 检测项 3: Zygisk 套接字
    if (checkSocket("/dev/socket/zygisk")) {
        result.evidences.push_back("Zygisk 套接字存在: /dev/socket/zygisk");
        LOGI("Zygisk 检测: 发现 zygisk 套接字");
    }
    
    if (checkSocket("/dev/socket/zygisk_pipe")) {
        result.evidences.push_back("Zygisk 管道套接字存在: /dev/socket/zygisk_pipe");
        LOGI("Zygisk 检测: 发现 zygisk_pipe 套接字");
    }
    
    // 检测项 4: /proc/self/maps 中的 Zygisk 相关映射
    if (checkMapsForPattern("zygisk")) {
        result.evidences.push_back("内存映射中发现 Zygisk 相关映射");
        LOGI("Zygisk 检测: 发现 Zygisk 内存映射");
    }
    
    // 检测项 5: Zygisk 模块目录
    if (checkDirectory("/data/adb/magisk/zygisk")) {
        result.evidences.push_back("Zygisk 模块目录存在: /data/adb/magisk/zygisk");
        LOGI("Zygisk 检测: 发现 Zygisk 模块目录");
    }
    
    // 设置检测结果
    result.detected = !result.evidences.empty();
    
    if (result.detected) {
        LOGI("Zygisk 检测完成: 发现 %zu 个证据", result.evidences.size());
    } else {
        LOGI("Zygisk 检测完成: 未发现证据");
    }
    
    return result;
}

// ==================== KernelSU 专项检测 ====================

SpecialDetectionResult SpecialDetector::detectKernelSU() {
    SpecialDetectionResult result;
    result.toolName = "KernelSU";
    result.detected = false;
    
    // 检测项 1: /data/adb/ksu 目录存在性
    if (checkDirectory("/data/adb/ksu")) {
        result.evidences.push_back("KernelSU 目录存在: /data/adb/ksu");
        LOGI("KernelSU 检测: 发现 KernelSU 目录");
    }
    
    // 检测项 2: 内核版本字符串中的 "KernelSU"、"KSU" 关键词
    if (checkKernelVersion("KernelSU")) {
        result.evidences.push_back("内核版本中发现 KernelSU 关键词");
        LOGI("KernelSU 检测: 内核版本包含 KernelSU");
    }
    
    if (checkKernelVersion("KSU")) {
        result.evidences.push_back("内核版本中发现 KSU 关键词");
        LOGI("KernelSU 检测: 内核版本包含 KSU");
    }
    
    // 检测项 3: overlayfs 挂载点特征
    if (checkOverlayfsMounts()) {
        result.evidences.push_back("检测到可疑的 overlayfs 挂载（KernelSU 特征）");
        LOGI("KernelSU 检测: 发现 overlayfs 挂载");
    }
    
    // 检测项 4: 内存中的 KernelSU 特征
    if (checkMemoryPattern("kernelsu")) {
        result.evidences.push_back("内存中发现 KernelSU 特征");
        LOGI("KernelSU 检测: 发现 kernelsu 内存特征");
    }
    
    if (checkMemoryPattern("libkernelsu.so")) {
        result.evidences.push_back("内存中发现 libkernelsu.so");
        LOGI("KernelSU 检测: 发现 libkernelsu.so");
    }
    
    // 检测项 5: /proc/self/maps 中的 KernelSU 相关映射
    if (checkMapsForPattern("ksu")) {
        result.evidences.push_back("内存映射中发现 KernelSU 相关映射");
        LOGI("KernelSU 检测: 发现 KernelSU 内存映射");
    }
    
    // 设置检测结果
    result.detected = !result.evidences.empty();
    
    if (result.detected) {
        LOGI("KernelSU 检测完成: 发现 %zu 个证据", result.evidences.size());
    } else {
        LOGI("KernelSU 检测完成: 未发现证据");
    }
    
    return result;
}

// ==================== LSPosed 专项检测 ====================

SpecialDetectionResult SpecialDetector::detectLSPosed() {
    SpecialDetectionResult result;
    result.toolName = "LSPosed";
    result.detected = false;
    
    // 检测项 1: /data/adb/lspd 目录存在性
    if (checkDirectory("/data/adb/lspd")) {
        result.evidences.push_back("LSPosed 目录存在: /data/adb/lspd");
        LOGI("LSPosed 检测: 发现 LSPosed 目录");
    }
    
    // 检测项 2: 内存中 liblspd.so 特征
    if (checkMemoryPattern("liblspd.so")) {
        result.evidences.push_back("内存中发现 liblspd.so");
        LOGI("LSPosed 检测: 发现 liblspd.so");
    }
    
    if (checkMemoryPattern("lspd")) {
        result.evidences.push_back("内存中发现 LSPosed 特征");
        LOGI("LSPosed 检测: 发现 lspd 内存特征");
    }
    
    // 检测项 3: ART 虚拟机 Hook 痕迹
    if (checkArtHookTraces()) {
        result.evidences.push_back("检测到 ART 虚拟机 Hook 痕迹");
        LOGI("LSPosed 检测: 发现 ART Hook 痕迹");
    }
    
    // 检测项 4: 异常 Binder 调用检测
    if (checkBinderAnomalies()) {
        result.evidences.push_back("检测到异常 Binder 调用");
        LOGI("LSPosed 检测: 发现异常 Binder 调用");
    }
    
    // 检测项 5: LSPosed 套接字
    if (checkSocket("/dev/socket/lsposed")) {
        result.evidences.push_back("LSPosed 套接字存在: /dev/socket/lsposed");
        LOGI("LSPosed 检测: 发现 lsposed 套接字");
    }
    
    if (checkSocket("/dev/socket/lspd")) {
        result.evidences.push_back("LSPosed 守护进程套接字存在: /dev/socket/lspd");
        LOGI("LSPosed 检测: 发现 lspd 套接字");
    }
    
    // 检测项 6: /proc/self/maps 中的 LSPosed 相关映射
    if (checkMapsForPattern("lsposed")) {
        result.evidences.push_back("内存映射中发现 LSPosed 相关映射");
        LOGI("LSPosed 检测: 发现 LSPosed 内存映射");
    }
    
    if (checkMapsForPattern("xposed")) {
        result.evidences.push_back("内存映射中发现 Xposed 相关映射");
        LOGI("LSPosed 检测: 发现 Xposed 内存映射");
    }
    
    // 设置检测结果
    result.detected = !result.evidences.empty();
    
    if (result.detected) {
        LOGI("LSPosed 检测完成: 发现 %zu 个证据", result.evidences.size());
    } else {
        LOGI("LSPosed 检测完成: 未发现证据");
    }
    
    return result;
}

// ==================== Shamiko 专项检测 ====================

SpecialDetectionResult SpecialDetector::detectShamiko() {
    SpecialDetectionResult result;
    result.toolName = "Shamiko";
    result.detected = false;
    
    // 检测项 1: Zygisk 模块列表中的 Shamiko
    if (checkZygiskModuleList()) {
        result.evidences.push_back("在 Zygisk 模块列表中发现 Shamiko");
        LOGI("Shamiko 检测: 发现 Shamiko 模块");
    }
    
    // 检测项 2: 内存中的 Shamiko 特征
    if (checkMemoryPattern("shamiko")) {
        result.evidences.push_back("内存中发现 Shamiko 特征");
        LOGI("Shamiko 检测: 发现 shamiko 内存特征");
    }
    
    if (checkMemoryPattern("libshamiko.so")) {
        result.evidences.push_back("内存中发现 libshamiko.so");
        LOGI("Shamiko 检测: 发现 libshamiko.so");
    }
    
    // 检测项 3: SELinux 上下文异常变化
    if (checkSelinuxContextChanges()) {
        result.evidences.push_back("检测到 SELinux 上下文异常变化");
        LOGI("Shamiko 检测: 发现 SELinux 上下文异常");
    }
    
    // 检测项 4: 系统调用 Hook 模式（Shamiko 独特模式）
    if (checkSyscallHookPatterns()) {
        result.evidences.push_back("检测到系统调用 Hook 模式（Shamiko 特征）");
        LOGI("Shamiko 检测: 发现系统调用 Hook 模式");
    }
    
    // 检测项 5: Shamiko 目录
    if (checkDirectory("/data/adb/shamiko")) {
        result.evidences.push_back("Shamiko 目录存在: /data/adb/shamiko");
        LOGI("Shamiko 检测: 发现 Shamiko 目录");
    }
    
    // 检测项 6: /proc/self/maps 中的 Shamiko 相关映射
    if (checkMapsForPattern("shamiko")) {
        result.evidences.push_back("内存映射中发现 Shamiko 相关映射");
        LOGI("Shamiko 检测: 发现 Shamiko 内存映射");
    }
    
    // 设置检测结果
    result.detected = !result.evidences.empty();
    
    if (result.detected) {
        LOGI("Shamiko 检测完成: 发现 %zu 个证据", result.evidences.size());
    } else {
        LOGI("Shamiko 检测完成: 未发现证据");
    }
    
    return result;
}
