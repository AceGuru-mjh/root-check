#include "art_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <android/log.h>

#define LOG_TAG "ArtDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool ArtDetector::checkSuspiciousDexSources() {
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(maps, line)) {
        // 检查是否有来自 /data/adb 的 Dex 文件
        if (line.find("/data/adb") != std::string::npos &&
            (line.find(".dex") != std::string::npos ||
             line.find(".odex") != std::string::npos ||
             line.find(".vdex") != std::string::npos)) {
            LOGI("检测到可疑 Dex 来源: %s", line.c_str());
            return true;
        }
    }

    return false;
}

bool ArtDetector::checkArtHookSignatures() {
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(maps, line)) {
        // 检查是否有 Xposed 相关的库
        if (line.find("libxposed") != std::string::npos ||
            line.find("libriru") != std::string::npos ||
            line.find("liblspd") != std::string::npos) {
            LOGI("检测到 ART Hook 特征: %s", line.c_str());
            return true;
        }
    }

    return false;
}

bool ArtDetector::checkXposedFrameworks() {
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        return false;
    }

    std::vector<std::string> xposedKeywords = {
        "xposed", "lspd", "riru", "edxposed", "dreamland"
    };

    std::string line;
    while (std::getline(maps, line)) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        for (const auto& keyword : xposedKeywords) {
            if (lowerLine.find(keyword) != std::string::npos) {
                LOGI("检测到 Xposed 框架特征: %s", line.c_str());
                return true;
            }
        }
    }

    return false;
}

DetectionResult ArtDetector::detect() {
    DetectionResult result;
    result.layer = "第2层：ART 虚拟机异常检测";
    result.detected = false;

    std::vector<std::string> findings;

    if (checkSuspiciousDexSources()) {
        findings.push_back("可疑 Dex 文件来源");
    }

    if (checkArtHookSignatures()) {
        findings.push_back("ART Hook 特征");
    }

    if (checkXposedFrameworks()) {
        findings.push_back("Xposed 框架特征");
    }

    if (!findings.empty()) {
        result.detected = true;
        result.detail = "ART 异常: ";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.detail += findings[i];
            if (i < findings.size() - 1) {
                result.detail += "; ";
            }
        }
    } else {
        result.detail = "未发现 ART 异常";
    }

    return result;
}
