#include "property_detector.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "PropertyDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Android 属性共享内存二进制文件路径
static const char* PROPERTIES_SHM_PATH = "/dev/__properties__";

std::vector<std::string> PropertyDetector::getRootKeywords() {
    return {
        "magisk", "zygisk", "su", "ksu", "apatch",
        "lspd", "lsposed", "xposed", "riru", "shamiko",
        "supersu", "kingroot", "kernelsu", "substratum",
        "selinux", "permissive"
    };
}

// 将字符串转为小写
static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// 检查字符串中是否包含关键词（不区分大小写）
bool PropertyDetector::containsKeyword(const std::string& text,
                                       const std::vector<std::string>& keywords,
                                       std::string& matchedKeyword) {
    std::string lowerText = toLower(text);
    for (const auto& keyword : keywords) {
        if (lowerText.find(keyword) != std::string::npos) {
            matchedKeyword = keyword;
            return true;
        }
    }
    return false;
}

/*
 * 解析 /dev/__properties__ 二进制文件
 *
 * Android 属性共享内存格式（prop_area）:
 *   - 偏移 0x00: uint32_t magic   (0x504f5250 = "PROP")
 *   - 偏移 0x04: uint32_t version
 *   - 偏移 0x08: uint32_t reserved[2]
 *   - 偏移 0x10: uint32_t max_serial
 *   - 偏移 0x14: uint32_t max_name_len  (通常 32 或 96)
 *   - 偏移 0x18: uint32_t max_value_len  (通常 92)
 *   - 之后是 prop_bt 节点树，每个节点包含:
 *       uint32_t name_len
 *       char     name[name_len]  (不含 '\0')
 *       uint32_t prop_offset     (指向 prop_info, 0 表示内部节点)
 *       uint32_t left, right, children  (trie 树指针偏移)
 *   - prop_info 结构:
 *       uint32_t serial
 *       uint32_t name_len
 *       uint32_t value_len
 *       char     name[name_len]
 *       char     value[value_len]
 *
 * 这里采用简化扫描方式：在整个文件中搜索可打印字符串序列，
 * 提取连续的键值对进行关键词检测。这样即使格式版本不同也能工作。
 */
std::vector<std::pair<std::string, std::string>> PropertyDetector::readProperties() {
    std::vector<std::pair<std::string, std::string>> properties;

    std::ifstream file(PROPERTIES_SHM_PATH, std::ios::binary);
    if (!file.is_open()) {
        LOGW("无法打开属性文件: %s", PROPERTIES_SHM_PATH);
        return properties;
    }

    // 读取整个文件内容
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0 || fileSize > 4 * 1024 * 1024) {
        LOGW("属性文件大小异常: %lld", (long long)fileSize);
        return properties;
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.read(buffer.data(), fileSize);
    file.close();

    LOGI("读取属性文件, 大小: %lld 字节", (long long)fileSize);

    // 扫描二进制文件，提取可打印字符串
    // Android 属性键格式: "key\0value\0" 交替排列
    // 我们提取所有长度 >= 2 的可打印字符串序列
    std::vector<std::string> strings;
    std::string current;

    for (std::streamsize i = 0; i < fileSize; ++i) {
        unsigned char c = static_cast<unsigned char>(buffer[static_cast<size_t>(i)]);
        if (c >= 0x20 && c < 0x7f) {
            current += static_cast<char>(c);
        } else {
            if (current.length() >= 2) {
                strings.push_back(current);
            }
            current.clear();
        }
    }
    if (current.length() >= 2) {
        strings.push_back(current);
    }

    // 尝试配对键值对：属性键通常包含 '.' 分隔符（如 ro.build.fingerprint）
    // 在提取的字符串列表中，键后面紧跟的通常是值
    for (size_t i = 0; i < strings.size(); ++i) {
        const std::string& s = strings[i];
        // 判断是否为属性键：包含至少一个 '.' 且全部由合法字符组成
        bool isKey = false;
        if (s.length() >= 3 && s.length() <= 128) {
            int dotCount = 0;
            bool validKey = true;
            for (char c : s) {
                if (c == '.') {
                    dotCount++;
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                           (c >= '0' && c <= '9') || c == '_' || c == '-') {
                    // 合法键字符
                } else {
                    validKey = false;
                    break;
                }
            }
            isKey = validKey && dotCount >= 1;
        }

        if (isKey && i + 1 < strings.size()) {
            const std::string& value = strings[i + 1];
            // 值不应包含 '.' 分隔的域名格式，长度合理
            if (value.length() <= 256) {
                properties.emplace_back(s, value);
                i++; // 跳过已配对的值
            }
        }
    }

    LOGI("解析出 %zu 个属性键值对", properties.size());
    return properties;
}

DetectionResult PropertyDetector::detect() {
    DetectionResult result;
    result.layer = "第5层：属性检测";
    result.detected = false;

    auto keywords = getRootKeywords();
    auto properties = readProperties();

    std::vector<std::string> suspiciousProps;

    for (const auto& prop : properties) {
        const std::string& key = prop.first;
        const std::string& value = prop.second;
        std::string matched;

        // 检查键名
        if (containsKeyword(key, keywords, matched)) {
            std::string finding = "键名包含关键词 [" + matched + "]: " + key + "=" + value;
            suspiciousProps.push_back(finding);
            LOGI("检测到可疑属性: %s", finding.c_str());
        }
        // 检查值
        else if (containsKeyword(value, keywords, matched)) {
            std::string finding = "值包含关键词 [" + matched + "]: " + key + "=" + value;
            suspiciousProps.push_back(finding);
            LOGI("检测到可疑属性: %s", finding.c_str());
        }
    }

    if (!suspiciousProps.empty()) {
        result.detected = true;
        result.detail = "发现 " + std::to_string(suspiciousProps.size()) + " 个可疑属性: ";
        for (size_t i = 0; i < std::min(suspiciousProps.size(), size_t(5)); ++i) {
            if (i > 0) {
                result.detail += "; ";
            }
            result.detail += suspiciousProps[i];
        }
        if (suspiciousProps.size() > 5) {
            result.detail += "...";
        }
    } else {
        result.detail = "未发现可疑系统属性";
    }

    return result;
}
