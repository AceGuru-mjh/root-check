#include "selinux_detector.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "SelinuxDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// 存储不同阶段的 SELinux 上下文
static std::vector<std::string> g_contextHistory;

std::string getCurrentSelinuxContext() {
    std::ifstream context("/proc/self/attr/current");
    if (!context.is_open()) {
        LOGW("无法打开 /proc/self/attr/current");
        return "";
    }
    
    std::string ctx;
    std::getline(context, ctx);
    return ctx;
}

bool checkSelinuxAnomaly() {
    std::string ctx = getCurrentSelinuxContext();
    
    if (ctx.empty()) {
        return false;
    }
    
    // 检查异常的 SELinux 上下文
    std::vector<std::string> suspiciousContexts = {
        "magisk", "zygisk", "ksu", "kernelsu", "apatch",
        "u:r:magisk", "u:r:zygisk", "u:r:ksu", "u:r:apatch",
        "u:r:magisk:s0", "u:r:zygisk:s0"
    };
    
    std::string lowerCtx = ctx;
    std::transform(lowerCtx.begin(), lowerCtx.end(), lowerCtx.begin(), ::tolower);
    
    for (const auto& suspicious : suspiciousContexts) {
        if (lowerCtx.find(suspicious) != std::string::npos) {
            LOGI("检测到异常 SELinux 上下文: %s", ctx.c_str());
            return true;
        }
    }
    
    return false;
}

bool checkContextJump() {
    std::string currentCtx = getCurrentSelinuxContext();
    
    if (currentCtx.empty()) {
        return false;
    }
    
    // 记录当前上下文
    g_contextHistory.push_back(currentCtx);
    
    // 如果历史记录少于2条，无法判断跳变
    if (g_contextHistory.size() < 2) {
        return false;
    }
    
    // 对比最新两次上下文
    const std::string& prevCtx = g_contextHistory[g_contextHistory.size() - 2];
    const std::string& currCtx = g_contextHistory[g_contextHistory.size() - 1];
    
    if (prevCtx != currCtx) {
        LOGW("检测到 SELinux 上下文跳变: %s -> %s", prevCtx.c_str(), currCtx.c_str());
        return true;
    }
    
    return false;
}

bool checkSelinuxAccess() {
    // 尝试读取 /sys/fs/selinux/access
    std::ifstream access("/sys/fs/selinux/access");
    if (!access.is_open()) {
        // 文件不存在或无法访问，可能是正常情况
        return false;
    }
    
    // 检查文件大小和内容
    access.seekg(0, std::ios::end);
    std::streamsize size = access.tellg();
    access.seekg(0, std::ios::beg);
    
    // 异常的 SELinux access 文件通常有非零大小
    if (size > 0) {
        std::string content;
        std::getline(access, content);
        
        // 检查是否包含可疑内容
        std::string lowerContent = content;
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
        
        if (lowerContent.find("allow") != std::string::npos ||
            lowerContent.find("magisk") != std::string::npos ||
            lowerContent.find("zygisk") != std::string::npos) {
            LOGW("检测到 /sys/fs/selinux/access 异常内容");
            return true;
        }
    }
    
    return false;
}

bool checkShamikoPattern() {
    // Shamiko 的特征模式检测
    std::string ctx = getCurrentSelinuxContext();
    
    if (ctx.empty()) {
        return false;
    }
    
    // Shamiko 通常会尝试伪装成正常的 zygote 上下文
    // 但可能会留下一些痕迹
    std::vector<std::string> shamikoIndicators = {
        "u:r:zygote:s0",  // 可能是伪装的
        "u:r:untrusted_app:s0"
    };
    
    // 检查是否在不应该的阶段出现特定上下文
    // 例如，在 native 层检测到纯 zygote 上下文可能是异常的
    for (const auto& indicator : shamikoIndicators) {
        if (ctx == indicator) {
            // 进一步检查是否有其他异常迹象
            if (checkSelinuxAccess()) {
                LOGW("检测到可能的 Shamiko 模式: %s", ctx.c_str());
                return true;
            }
        }
    }
    
    return false;
}

DetectionResult SelinuxDetector::detect() {
    DetectionResult result;
    result.layer = "第5层：SELinux 上下文检测";
    result.detected = false;
    
    std::vector<std::string> findings;
    
    // 阶段1：获取并记录当前上下文
    std::string currentCtx = getCurrentSelinuxContext();
    if (!currentCtx.empty()) {
        LOGI("当前 SELinux 上下文: %s", currentCtx.c_str());
    } else {
        LOGW("无法获取 SELinux 上下文");
    }
    
    // 阶段2：检查上下文异常
    if (checkSelinuxAnomaly()) {
        findings.push_back("异常 SELinux 上下文");
    }
    
    // 阶段3：检查上下文跳变
    if (checkContextJump()) {
        findings.push_back("SELinux 上下文跳变");
    }
    
    // 阶段4：检查 /sys/fs/selinux/access
    if (checkSelinuxAccess()) {
        findings.push_back("SELinux access 异常");
    }
    
    // 阶段5：检查 Shamiko 特征模式
    if (checkShamikoPattern()) {
        findings.push_back("Shamiko 特征模式");
    }
    
    // 构建结果
    if (!findings.empty()) {
        result.detected = true;
        result.detail = "SELinux 异常: ";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.detail += findings[i];
            if (i < findings.size() - 1) {
                result.detail += "; ";
            }
        }
        LOGW("检测到 %zu 项 SELinux 异常", findings.size());
    } else {
        result.detail = "未发现 SELinux 异常";
        LOGI("SELinux 检测正常");
    }
    
    return result;
}
