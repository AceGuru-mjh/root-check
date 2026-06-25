#include "lsposed_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "LSPosedDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

std::vector<std::string> LSPosedDetector::listDir(const std::string& path) {
    return SyscallUtils::listFilesDirect(path);
}

std::string LSPosedDetector::getLibContent(const std::string& path) {
    std::string c = SyscallUtils::readFileDirect(path);
    if (!c.empty()) return c;
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool LSPosedDetector::detectOldRiruInjection(std::vector<RiruInjectionTrace>& traces) {
    std::string maps = getLibContent("/proc/self/maps");
    if (maps.empty()) return false;

    std::istringstream stream(maps);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("libriru") != std::string::npos) {
            RiruInjectionTrace trace;
            char libPath[256] = {0};
            unsigned long start = 0;

            sscanf(line.c_str(), "%lx-%*lx %*s %*x %*x:%*x %*d %255s",
                   &start, libPath);

            trace.lib_path = libPath;
            trace.base_addr = start;
            trace.is_old_riru = true;

            if (trace.lib_path.find("libriru_") != std::string::npos) {
                size_t pos = trace.lib_path.find("libriru_");
                if (pos != std::string::npos) {
                    trace.riru_version = trace.lib_path.substr(pos);
                }
            }

            traces.push_back(trace);
            LOGW("旧版 Riru 注入检测: %s @ 0x%lx", libPath, start);
            found = true;
        }

        if (lower.find("riru") != std::string::npos &&
            lower.find("lib") != std::string::npos) {
            RiruInjectionTrace trace;
            unsigned long start = 0;
            char libPath[256] = {0};
            trace.is_old_riru = true;
            sscanf(line.c_str(), "%lx-%*lx %*s %*x %*x:%*x %*d %255s",
                   &start, libPath);
            trace.lib_path = libPath;
            trace.base_addr = start;
            traces.push_back(trace);
            found = true;
        }
    }

    return found;
}

bool LSPosedDetector::detectLSPosedZygiskMode(std::vector<LSPosedZygiskMode>& modes) {
    std::string maps = getLibContent("/proc/self/maps");
    if (maps.empty()) return false;

    std::istringstream stream(maps);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("liblspd.so") != std::string::npos) {
            LSPosedZygiskMode mode;
            unsigned long start = 0;
            char path[256] = {0};
            sscanf(line.c_str(), "%lx-%*lx %*s %*x %*x:%*x %*d %255s",
                   &start, path);
            mode.lib_path = path;
            mode.base_addr = start;

            if (lower.find("zygisk") != std::string::npos ||
                getLibContent("/proc/self/cmdline").find("zygote") != std::string::npos) {
                mode.is_zygisk_mode = true;
                mode.lsposed_version = "zygisk";
                LOGW("LSPosed Zygisk 模式: liblspd.so @ 0x%lx (zygisk)", start);
            } else {
                mode.is_zygisk_mode = false;
                mode.lsposed_version = "legacy";
                LOGW("LSPosed 传统模式: liblspd.so @ 0x%lx (non-zygisk)", start);
            }

            modes.push_back(mode);
            found = true;
        }

        if (lower.find("lsposed") != std::string::npos &&
            lower.find(".so") != std::string::npos) {
            LSPosedZygiskMode mode;
            unsigned long start = 0;
            char path[256] = {0};
            sscanf(line.c_str(), "%lx-%*lx %*s %*x %*x:%*x %*d %255s",
                   &start, path);
            mode.lib_path = path;
            mode.base_addr = start;
            mode.is_zygisk_mode = (lower.find("zygisk") != std::string::npos);
            modes.push_back(mode);
            found = true;
        }
    }

    if (!found) {
        std::string modules = listDir("/data/adb/modules");
        for (const auto& mod : modules) {
            std::string lower = mod;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("riru") != std::string::npos &&
                lower.find("lsposed") != std::string::npos) {
                std::string riruPath = "/data/adb/modules/" + mod;
                std::string config = getLibContent(riruPath + "/config.prop");
                if (!config.empty()) {
                    LSPosedZygiskMode mode;
                    mode.lib_path = riruPath;
                    mode.is_zygisk_mode = false;
                    mode.lsposed_version = "riru_legacy";
                    modes.push_back(mode);

                    RiruInjectionTrace riru;
                    riru.lib_path = riruPath + "/libriru.so";
                    riru.is_old_riru = true;
                    LOGW("LSPosed Riru 旧版模块: %s (LSPosed via Riru)", mod.c_str());
                    found = true;
                }
            }
            if (lower.find("zygisk") != std::string::npos &&
                lower.find("lsposed") != std::string::npos) {
                LSPosedZygiskMode mode;
                mode.lib_path = "/data/adb/modules/" + mod;
                mode.is_zygisk_mode = true;
                mode.lsposed_version = "zygisk_lsposed";
                modes.push_back(mode);
                LOGW("LSPosed Zygisk 模块: %s", mod.c_str());
                found = true;
            }
        }
    }

    return found;
}

bool LSPosedDetector::detectLiblspdBinderChannel(std::vector<LiblspdBinderChannel>& channels) {
    std::string maps = getLibContent("/proc/self/maps");
    if (maps.empty()) return false;

    std::istringstream stream(maps);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        if (line.find("binder") != std::string::npos &&
            line.find("liblspd") != std::string::npos) {
            LiblspdBinderChannel ch;
            ch.channel_type = "binder_mmap";
            ch.endpoint = line;
            ch.is_active = true;
            ch.binder_context = line;
            channels.push_back(ch);
            LOGW("LSPosed liblspd binder 通道: %s", line.c_str());
            found = true;
        }

        if (line.find("anon_inode") != std::string::npos &&
            line.find("binder") != std::string::npos) {
            LiblspdBinderChannel ch;
            ch.channel_type = "anon_binder";
            ch.endpoint = line;
            ch.is_active = true;
            char context[64] = {0};
            sscanf(line.c_str(), "%*lx-%*lx %*s %*x %*x:%*x %*d %63s", context);
            ch.binder_context = context;
            channels.push_back(ch);
            found = true;
        }
    }

    std::string fdDir = getLibContent("/proc/self/fd/");
    if (!fdDir.empty()) {
        for (int fd = 0; fd < 256; fd++) {
            std::string linkPath = "/proc/self/fd/" + std::to_string(fd);
            std::string target = getLibContent(linkPath);
            if (target.find("binder") != std::string::npos) {
                if (target.find("lspd") != std::string::npos ||
                    target.find("lsposed") != std::string::npos) {
                    LiblspdBinderChannel ch;
                    ch.channel_type = "binder_fd";
                    ch.endpoint = target;
                    ch.is_active = true;
                    ch.binder_context = "fd:" + std::to_string(fd);
                    channels.push_back(ch);
                    LOGW("LSPosed binder fd: %d -> %s", fd, target.c_str());
                    found = true;
                }
            }
        }
    }

    std::string selfBinder = getLibContent("/proc/self/context");
    if (!selfBinder.empty()) {
        std::string lower = selfBinder;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("lspd") != std::string::npos ||
            lower.find("lsposed") != std::string::npos) {
            LiblspdBinderChannel ch;
            ch.channel_type = "binder_context";
            ch.endpoint = "/proc/self/context";
            ch.is_active = true;
            ch.binder_context = selfBinder;
            channels.push_back(ch);
            found = true;
        }
    }

    if (!found) {
        std::ifstream binderProcFile("/proc/binder/proc");
        if (binderProcFile.is_open()) {
            std::string bline;
            while (std::getline(binderProcFile, bline)) {
                if (bline.find("lspd") != std::string::npos ||
                    bline.find("lsposed") != std::string::npos) {
                    LiblspdBinderChannel ch;
                    ch.channel_type = "binder_proc_node";
                    ch.endpoint = bline;
                    ch.is_active = true;
                    channels.push_back(ch);
                    found = true;
                }
            }
        }
    }

    return found;
}

bool LSPosedDetector::detectLSPosedModule() {
    auto modules = listDir("/data/adb/modules");
    for (const auto& mod : modules) {
        std::string lower = mod;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("lsposed") != std::string::npos) {
            LOGW("检测到 LSPosed 模块: %s", mod.c_str());
            return true;
        }
    }

    if (SyscallUtils::fileExistsDirect("/data/adb/lspd")) {
        LOGW("检测到 LSPosed 目录: /data/adb/lspd");
        return true;
    }

    std::string socket = getLibContent("/dev/socket/lspd");
    if (!socket.empty()) {
        LOGW("检测到 LSPosed 套接字");
        return true;
    }

    std::string prop = getLibContent("/data/adb/lspd/config.conf");
    if (!prop.empty()) {
        LOGW("检测到 LSPosed 配置文件");
        return true;
    }

    return false;
}

LSPosedDetectionDetail LSPosedDetector::detectDetailed() {
    LSPosedDetectionDetail detail;
    detail.has_old_riru = false;
    detail.has_new_zygisk_mode = false;
    detail.has_liblspd_binder = false;
    detail.has_lsposed_module = false;

    LOGI("=== LSPosed Riru 双模式区分检测 ===");

    if (detectOldRiruInjection(detail.riru_traces)) {
        detail.has_old_riru = true;
        detail.findings.push_back("检测到旧版 Riru 注入 (" +
                                  std::to_string(detail.riru_traces.size()) + " 个库)");
    }

    if (detectLSPosedZygiskMode(detail.zygisk_modes)) {
        for (const auto& mode : detail.zygisk_modes) {
            if (mode.is_zygisk_mode) {
                detail.has_new_zygisk_mode = true;
            } else {
                detail.has_old_riru = true;
            }
        }
        if (detail.has_new_zygisk_mode) {
            detail.findings.push_back("检测到 LSPosed Zygisk 模式（新版）");
        }
        if (detail.has_old_riru) {
            detail.findings.push_back("检测到 LSPosed Riru 模式（旧版）");
        }
    }

    if (detectLiblspdBinderChannel(detail.binder_channels)) {
        detail.has_liblspd_binder = true;
        detail.findings.push_back("检测到 liblspd binder 私有通信通道 (" +
                                  std::to_string(detail.binder_channels.size()) + " 个)");
    }

    if (detectLSPosedModule()) {
        detail.has_lsposed_module = true;
        detail.findings.push_back("检测到 LSPosed 模块/目录残留");
    }

    return detail;
}

DetectionResult LSPosedDetector::detect() {
    DetectionResult result;
    result.layer = "对抗隐藏-LSPosed Riru双模式区分探针";
    result.detected = false;

    auto detail = detectDetailed();

    if (detail.has_old_riru || detail.has_new_zygisk_mode ||
        detail.has_liblspd_binder || detail.has_lsposed_module) {
        result.detected = true;
        std::string summary;
        if (detail.has_old_riru) summary += "旧版Riru模式; ";
        if (detail.has_new_zygisk_mode) summary += "新版Zygisk模式; ";
        if (detail.has_liblspd_binder) summary += "binder通道; ";
        if (detail.has_lsposed_module) summary += "模块残留; ";

        for (size_t i = 0; i < detail.findings.size(); i++) {
            if (summary.find(detail.findings[i]) == std::string::npos) {
                if (!summary.empty() && summary.back() != ';') summary += "; ";
                summary += detail.findings[i];
            }
        }

        result.detail = "LSPosed 异常: " + summary;
    } else {
        result.detail = "未发现 LSPosed/Riru 特征";
    }

    return result;
}
