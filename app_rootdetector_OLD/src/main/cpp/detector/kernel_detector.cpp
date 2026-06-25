#include "kernel_detector.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <android/log.h>

#define LOG_TAG "KernelDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

std::vector<std::string> getOfficialKernelModules() {
    // 官方内核模块白名单（示例）
    return {
        "vboxsf", "vboxguest", "vmw_vsock_vmci_transport",
        // 添加更多官方模块
    };
}

bool checkKallsymsAnomaly() {
    std::ifstream kallsyms("/proc/kallsyms");
    if (!kallsyms.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(kallsyms, line)) {
        // 检查 kallsyms_lookup_name 是否被导出
        if (line.find("kallsyms_lookup_name") != std::string::npos) {
            // 如果权限是 T（全局符号），可能是异常
            if (line.find(" T ") != std::string::npos) {
                LOGI("检测到 kallsyms_lookup_name 被导出: %s", line.c_str());
                return true;
            }
        }
    }
    
    return false;
}

std::vector<std::string> checkSuspiciousModules() {
    std::vector<std::string> suspicious;
    std::ifstream modules("/proc/modules");
    
    if (!modules.is_open()) {
        return suspicious;
    }
    
    auto officialModules = getOfficialKernelModules();
    
    std::string line;
    while (std::getline(modules, line)) {
        // 提取模块名
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            std::string moduleName = line.substr(0, spacePos);
            
            // 检查是否在白名单中
            bool isOfficial = false;
            for (const auto& official : officialModules) {
                if (moduleName.find(official) != std::string::npos) {
                    isOfficial = true;
                    break;
                }
            }
            
            if (!isOfficial) {
                suspicious.push_back(moduleName);
                LOGI("检测到可疑内核模块: %s", moduleName.c_str());
            }
        }
    }
    
    return suspicious;
}

DetectionResult KernelDetector::detect() {
    DetectionResult result;
    result.layer = "第6层：内核完整性检测";
    result.detected = false;
    
    std::vector<std::string> findings;
    
    // 检查 kallsyms 异常
    if (checkKallsymsAnomaly()) {
        findings.push_back("kallsyms_lookup_name 被导出");
    }
    
    // 检查可疑内核模块
    auto suspiciousModules = checkSuspiciousModules();
    if (!suspiciousModules.empty()) {
        findings.push_back("发现 " + std::to_string(suspiciousModules.size()) + " 个可疑内核模块");
    }
    
    if (!findings.empty()) {
        result.detected = true;
        result.detail = "内核异常: ";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.detail += findings[i];
            if (i < findings.size() - 1) {
                result.detail += "; ";
            }
        }
    } else {
        result.detail = "未发现内核异常";
    }
    
    return result;
}
