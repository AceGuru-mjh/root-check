#include "zygisknext_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <android/log.h>

#define LOG_TAG "ZygiskNextDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

std::string ZygiskNextDetector::readProcFile(const std::string& path) {
    std::string c = SyscallUtils::readFileDirect(path);
    if (!c.empty()) return c;
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::vector<std::string> ZygiskNextDetector::listDir(const std::string& path) {
    return SyscallUtils::listFilesDirect(path);
}

bool ZygiskNextDetector::scanMemoryIsolationMarks(std::vector<MemoryIsolationMark>& marks) {
    std::string maps = readProcFile("/proc/self/maps");
    if (maps.empty()) return false;

    std::istringstream stream(maps);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        MemoryIsolationMark mark;
        char perms[8] = {0}, path[256] = {0};
        unsigned long start = 0, end = 0;
        int offset = 0, inode = 0;

        int parsed = sscanf(line.c_str(), "%lx-%lx %4s %x %*x:%*x %d %255s",
                            &start, &end, perms, &offset, &inode, path);
        if (parsed < 5) continue;

        mark.start_addr = start;
        mark.end_addr = end;
        mark.perms = perms;
        mark.path = (parsed >= 6) ? path : "";

        std::string lowerPath = mark.path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        if (lowerPath.find("zygisk") != std::string::npos) {
            mark.is_isolated = true;
            mark.reason = "zygisknext 内存映射";
            marks.push_back(mark);
            LOGW("ZygiskNext 隔离内存: %lx-%lx %s %s",
                 start, end, perms, path);
            found = true;
        }

        if (mark.perms.find("rw") != std::string::npos &&
            mark.perms.find("p") != std::string::npos &&
            mark.path.empty()) {
            if (end - start >= 0x100000) {
                std::string status = readProcFile("/proc/self/status");
                if (status.find("SigBlk:") != std::string::npos) {
                    mark.is_isolated = true;
                    mark.reason = "ZygiskNext 匿名隔离区 (>=1MB rw-p)";
                    marks.push_back(mark);
                    found = true;
                }
            }
        }

        if (lowerPath.find("memfd:") != std::string::npos &&
            lowerPath.find("zygisk") != std::string::npos) {
            mark.is_isolated = true;
            mark.reason = "zygisknext memfd 隔离通道";
            marks.push_back(mark);
            found = true;
        }
    }

    pid_t ppid = getppid();
    std::string parentMaps = readProcFile("/proc/" + std::to_string(ppid) + "/maps");
    if (!parentMaps.empty() && parentMaps != maps) {
        std::istringstream pstream(parentMaps);
        while (std::getline(pstream, line)) {
            if (line.find("zygisk") != std::string::npos ||
                line.find("Zygisk") != std::string::npos) {
                if (maps.find(line) == std::string::npos) {
                    LOGW("父进程存在 zygisknext 映射但本进程无: %s", line.c_str());
                    found = true;
                }
            }
        }
    }

    return found;
}

bool ZygiskNextDetector::detectNamespaceAutoEscape(std::vector<AnonymousNamespaceInfo>& entries) {
    auto procDirs = listDir("/proc");
    bool found = false;

    std::string selfNs = readProcFile("/proc/self/ns/pid");
    for (const auto& dir : procDirs) {
        int pid = 0;
        try { pid = std::stoi(dir); } catch (...) { continue; }
        if (pid <= 0 || pid == getpid()) continue;

        std::string targetNs = readProcFile("/proc/" + dir + "/ns/pid");
        if (targetNs.empty() || selfNs.empty()) continue;

        std::string selfMnt = readProcFile("/proc/self/ns/mnt");
        std::string targetMnt = readProcFile("/proc/" + dir + "/ns/mnt");

        if (selfMnt != targetMnt && selfNs == targetNs) {
            AnonymousNamespaceInfo info;
            info.ns_type = "mnt";
            info.target_proc = dir;
            info.has_escape_intercept = false;

            std::string targetMaps = readProcFile("/proc/" + dir + "/maps");
            if (targetMaps.find("zygisk") != std::string::npos) {
                info.has_escape_intercept = true;
                info.ns_fd = 0;
                entries.push_back(info);
                LOGW("ZygiskNext 命名空间逃逸拦截: pid=%s (不同 mnt ns, 相同 pid ns)", dir.c_str());
                found = true;
            }
        }
    }

    std::ifstream fdDir("/proc/self/fd");
    if (fdDir.is_open()) {
        std::string fdEntry;
        while (std::getline(fdDir, fdEntry)) {
            if (fdEntry.find("ns:") != std::string::npos) {
                AnonymousNamespaceInfo info;
                info.ns_type = "anon";
                info.has_escape_intercept = true;
                entries.push_back(info);
                found = true;
            }
        }
    }

    return found;
}

bool ZygiskNextDetector::detectPrivateMemfdChannels(std::vector<MemfdChannel>& channels) {
    std::string maps = readProcFile("/proc/self/maps");
    if (maps.empty()) return false;

    std::istringstream stream(maps);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        if (line.find("memfd:") != std::string::npos) {
            MemfdChannel ch;
            char name[128] = {0};
            unsigned long addr = 0;
            size_t size = 0;

            sscanf(line.c_str(), "%lx-%lx %*s %*x %*x:%*x %*d %127s",
                   &addr, &size, name);

            if (size > addr) size = size - addr;
            ch.name = name;
            ch.address = addr;
            ch.size = size;
            ch.is_private = true;

            std::string lower = ch.name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find("zygisk") != std::string::npos ||
                lower.find("module") != std::string::npos ||
                lower.find("loader") != std::string::npos) {
                ch.module_name = ch.name;
                channels.push_back(ch);
                LOGW("ZygiskNext 私有 memfd 通道: %s @ %lx (%zu bytes)",
                     ch.name.c_str(), ch.address, ch.size);
                found = true;
            }
        }
    }

    std::string fdDir = readProcFile("/proc/self/fd/");
    if (!fdDir.empty()) {
        for (int fd = 0; fd < 1024; fd++) {
            std::string linkPath = "/proc/self/fd/" + std::to_string(fd);
            struct stat st;
            if (stat(linkPath.c_str(), &st) != 0) continue;
            if (S_ISSOCK(st.st_mode)) {
                std::string target = readProcFile(linkPath);
                if (target.find("memfd:") != std::string::npos ||
                    target.find("anon_inode:") != std::string::npos) {
                    MemfdChannel ch;
                    ch.name = "fd:" + std::to_string(fd);
                    ch.is_private = true;
                    channels.push_back(ch);
                    found = true;
                }
            }
        }
    }

    return found;
}

bool ZygiskNextDetector::detectZygiskNextModule() {
    auto modules = listDir("/data/adb/modules");
    for (const auto& mod : modules) {
        std::string lower = mod;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("zygisknext") != std::string::npos ||
            lower.find("zygisk_next") != std::string::npos) {
            LOGW("检测到 ZygiskNext 模块: %s", mod.c_str());
            return true;
        }
    }

    std::string prop = readProcFile("/data/adb/zygisknext/zygisknext.prop");
    if (!prop.empty()) {
        LOGW("检测到 ZygiskNext 配置文件");
        return true;
    }

    if (SyscallUtils::fileExistsDirect("/data/adb/zygisknext")) {
        LOGW("检测到 ZygiskNext 目录");
        return true;
    }

    if (SyscallUtils::fileExistsDirect("/data/adb/modules/zygisknext")) {
        LOGW("检测到 ZygiskNext 模块目录");
        return true;
    }

    return false;
}

ZygiskNextDetectionDetail ZygiskNextDetector::detectDetailed() {
    ZygiskNextDetectionDetail detail;
    detail.has_memory_isolation = false;
    detail.has_ns_escape_intercept = false;
    detail.has_private_memfd = false;
    detail.has_zygisknext_module = false;

    LOGI("=== ZygiskNext 隔离层检测 ===");

    if (scanMemoryIsolationMarks(detail.isolated_regions)) {
        detail.has_memory_isolation = true;
        detail.findings.push_back("检测到 ZygiskNext 内存隔离标记 (" +
                                  std::to_string(detail.isolated_regions.size()) + " 处)");
    }

    if (detectNamespaceAutoEscape(detail.ns_entries)) {
        detail.has_ns_escape_intercept = true;
        detail.findings.push_back("检测到匿名命名空间自动逃逸拦截逻辑");
    }

    if (detectPrivateMemfdChannels(detail.memfd_channels)) {
        detail.has_private_memfd = true;
        detail.findings.push_back("检测到 ZygiskNext 私有 memfd 模块加载通道 (" +
                                  std::to_string(detail.memfd_channels.size()) + " 个)");
    }

    if (detectZygiskNextModule()) {
        detail.has_zygisknext_module = true;
        detail.findings.push_back("检测到 ZygiskNext 模块/目录残留");
    }

    return detail;
}

DetectionResult ZygiskNextDetector::detect() {
    DetectionResult result;
    result.layer = "对抗隐藏-ZygiskNext隔离层探针";
    result.detected = false;

    auto detail = detectDetailed();

    if (detail.has_memory_isolation || detail.has_ns_escape_intercept ||
        detail.has_private_memfd || detail.has_zygisknext_module) {
        result.detected = true;
        std::string summary;
        for (size_t i = 0; i < detail.findings.size(); i++) {
            if (i > 0) summary += "; ";
            summary += detail.findings[i];
        }
        result.detail = "ZygiskNext 异常: " + summary;
    } else {
        result.detail = "未发现 ZygiskNext 隔离层特征";
    }

    return result;
}
