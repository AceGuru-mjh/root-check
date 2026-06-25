#include "process_detector.h"
#include "../utils/syscall_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <android/log.h>

#define LOG_TAG "ProcessDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// linux_dirent64 结构体定义（用于 getdents64 系统调用）
struct linux_dirent64 {
    unsigned long long d_ino;
    long long          d_off;
    unsigned short     d_reclen;
    unsigned char      d_type;
    char               d_name[];
};

// ==================== 辅助方法实现 ====================

std::vector<std::string> ProcessDetector::getSuspiciousProcessNames() {
    return {
        // Magisk 相关
        "magiskd", "magisk", "magisk64", "magisk32",
        "magiskboot", "magiskinit", "magiskpolicy",
        // KernelSU 相关
        "ksud", "ksu", "kernelsu", "ksu_daemon",
        // APatch 相关
        "apd", "apatch", "apatchd",
        // SuperSU 相关
        "daemonsu", "supersu", "su",
        // 其他 Root 工具
        "frida", "frida-server", "gum-js-loop",
        "xposed", "lspd", "lsposedd",
        "riru", "rirud",
        "shamiko",
        // 通用 Root 工具关键词
        "busybox", "cidpu", "msr_daemon",
        "supolicy", "sepolicy-inject"
    };
}

std::vector<std::string> ProcessDetector::getSuspiciousSelinuxContexts() {
    return {
        "u:r:magisk:s0",
        "u:r:magisk_file:s0",
        "u:r:ksu:s0",
        "u:r:kernelsu:s0",
        "u:r:apatch:s0",
        "u:r:zygisk:s0",
        "u:r:zygisk_file:s0",
        "u:r:shamiko:s0",
        "u:r:lspd:s0",
        "u:r:riru:s0",
        "u:r:su:s0",
        "u:r:frida:s0",
        "u:r:xposed:s0"
    };
}

bool ProcessDetector::isProcessNameSuspicious(const std::string& name, std::string& reason) {
    if (name.empty()) {
        return false;
    }

    // 提取进程名（去除路径前缀）
    std::string baseName = name;
    size_t slashPos = baseName.rfind('/');
    if (slashPos != std::string::npos) {
        baseName = baseName.substr(slashPos + 1);
    }

    // 转小写进行比较
    std::string lowerName = baseName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto suspiciousNames = getSuspiciousProcessNames();
    for (const auto& suspicious : suspiciousNames) {
        if (lowerName == suspicious || lowerName.find(suspicious) != std::string::npos) {
            reason = "进程名称匹配: " + baseName + " (匹配: " + suspicious + ")";
            return true;
        }
    }

    return false;
}

bool ProcessDetector::isSelinuxContextSuspicious(const std::string& context, std::string& reason) {
    if (context.empty()) {
        return false;
    }

    // 去除尾部换行符
    std::string cleanCtx = context;
    while (!cleanCtx.empty() && (cleanCtx.back() == '\n' || cleanCtx.back() == '\r')) {
        cleanCtx.pop_back();
    }

    std::string lowerCtx = cleanCtx;
    std::transform(lowerCtx.begin(), lowerCtx.end(), lowerCtx.begin(), ::tolower);

    // 检查精确匹配
    auto suspiciousContexts = getSuspiciousSelinuxContexts();
    for (const auto& suspicious : suspiciousContexts) {
        if (lowerCtx == suspicious) {
            reason = "SELinux 上下文匹配: " + cleanCtx;
            return true;
        }
    }

    // 检查关键词匹配（更宽泛的检测）
    std::vector<std::string> keywords = {
        "magisk", "zygisk", "ksu", "kernelsu", "apatch",
        "shamiko", "lspd", "lsposed", "riru", "frida",
        "xposed", "supersu", "su:s0"
    };

    for (const auto& keyword : keywords) {
        if (lowerCtx.find(keyword) != std::string::npos) {
            reason = "SELinux 上下文包含关键词: " + cleanCtx + " (匹配: " + keyword + ")";
            return true;
        }
    }

    return false;
}

// ==================== 核心系统调用方法 ====================

std::vector<pid_t> ProcessDetector::getAllPids() {
    std::vector<pid_t> pids;

    // 使用直接 syscall 打开 /proc 目录，绕过可能被 Hook 的标准库函数
    int fd = syscall(__NR_openat, AT_FDCWD, "/proc",
                     O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    if (fd < 0) {
        LOGW("无法通过 syscall 打开 /proc 目录");
        return pids;
    }

    char buf[4096];
    ssize_t nread;

    // 使用 getdents64 系统调用直接读取目录项
    while ((nread = syscall(__NR_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (long bpos = 0; bpos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + bpos);

            // 只处理目录类型且名称为数字的条目（即进程 PID）
            if (d->d_type == DT_DIR) {
                // 检查是否全部为数字字符
                bool isPid = true;
                for (int i = 0; d->d_name[i] != '\0'; i++) {
                    if (d->d_name[i] < '0' || d->d_name[i] > '9') {
                        isPid = false;
                        break;
                    }
                }
                if (isPid && d->d_name[0] != '\0') {
                    pid_t pid = (pid_t)atoi(d->d_name);
                    if (pid > 0) {
                        pids.push_back(pid);
                    }
                }
            }

            bpos += d->d_reclen;
        }
    }

    close(fd);
    LOGI("通过 syscall(getdents64) 扫描到 %zu 个进程", pids.size());
    return pids;
}

std::string ProcessDetector::getProcessSelinuxContext(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/attr/current", pid);

    // 使用直接 syscall 读取 SELinux 上下文
    return SyscallUtils::readFileDirect(path);
}

std::string ProcessDetector::getThreadSelinuxContext(pid_t tid) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/self/task/%d/attr/current", tid);

    // 使用直接 syscall 读取线程 SELinux 上下文
    return SyscallUtils::readFileDirect(path);
}

ProcessInfo ProcessDetector::getProcessInfo(pid_t pid) {
    ProcessInfo info;
    info.pid = pid;
    info.isSuspicious = false;

    // 读取进程命令行（/proc/[pid]/cmdline）
    char cmdlinePath[64];
    snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);
    std::string cmdline = SyscallUtils::readFileDirect(cmdlinePath);

    // cmdline 以 null 分隔参数，取第一个作为进程名
    if (!cmdline.empty()) {
        size_t nullPos = cmdline.find('\0');
        if (nullPos != std::string::npos) {
            info.name = cmdline.substr(0, nullPos);
        } else {
            info.name = cmdline;
        }
        info.cmdline = cmdline;
        // 将 cmdline 中的 null 替换为空格以便显示
        std::replace(info.cmdline.begin(), info.cmdline.end(), '\0', ' ');
    }

    // 读取可执行文件路径（/proc/[pid]/exe）
    char exePath[64];
    snprintf(exePath, sizeof(exePath), "/proc/%d/exe", pid);
    // 使用 readlink 通过 syscall 读取
    char linkBuf[256];
    int linkFd = syscall(__NR_openat, AT_FDCWD, exePath, O_RDONLY | O_CLOEXEC, 0);
    if (linkFd >= 0) {
        close(linkFd);
        // exe 是符号链接，尝试读取
        ssize_t len = readlink(exePath, linkBuf, sizeof(linkBuf) - 1);
        if (len > 0) {
            linkBuf[len] = '\0';
            info.exe = linkBuf;
        }
    }

    // 如果 name 为空，尝试从 exe 路径提取
    if (info.name.empty() && !info.exe.empty()) {
        size_t lastSlash = info.exe.rfind('/');
        if (lastSlash != std::string::npos) {
            info.name = info.exe.substr(lastSlash + 1);
        }
    }

    // 读取 SELinux 上下文
    info.selinuxContext = getProcessSelinuxContext(pid);
    // 清理换行符
    while (!info.selinuxContext.empty() &&
           (info.selinuxContext.back() == '\n' || info.selinuxContext.back() == '\r')) {
        info.selinuxContext.pop_back();
    }

    // 检查进程名称是否可疑
    std::string nameReason;
    if (isProcessNameSuspicious(info.name, nameReason)) {
        info.isSuspicious = true;
        info.suspiciousReason = nameReason;
    }

    // 检查 SELinux 上下文是否可疑
    std::string ctxReason;
    if (isSelinuxContextSuspicious(info.selinuxContext, ctxReason)) {
        info.isSuspicious = true;
        if (!info.suspiciousReason.empty()) {
            info.suspiciousReason += "; ";
        }
        info.suspiciousReason += ctxReason;
    }

    return info;
}

std::vector<ThreadInfo> ProcessDetector::getThreadInfos(pid_t pid) {
    std::vector<ThreadInfo> threads;

    char taskPath[64];
    snprintf(taskPath, sizeof(taskPath), "/proc/%d/task", pid);

    // 使用直接 syscall 读取 task 目录
    int fd = syscall(__NR_openat, AT_FDCWD, taskPath,
                     O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    if (fd < 0) {
        return threads;
    }

    char buf[4096];
    ssize_t nread;

    while ((nread = syscall(__NR_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (long bpos = 0; bpos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + bpos);

            if (d->d_type == DT_DIR && d->d_name[0] >= '0' && d->d_name[0] <= '9') {
                // 验证全部为数字
                bool isTid = true;
                for (int i = 0; d->d_name[i] != '\0'; i++) {
                    if (d->d_name[i] < '0' || d->d_name[i] > '9') {
                        isTid = false;
                        break;
                    }
                }

                if (isTid) {
                    ThreadInfo thread;
                    thread.tid = (pid_t)atoi(d->d_name);
                    thread.isSuspicious = false;

                    // 读取线程名称
                    char commPath[128];
                    snprintf(commPath, sizeof(commPath), "/proc/%d/task/%d/comm",
                             pid, thread.tid);
                    thread.name = SyscallUtils::readFileDirect(commPath);
                    while (!thread.name.empty() &&
                           (thread.name.back() == '\n' || thread.name.back() == '\r')) {
                        thread.name.pop_back();
                    }

                    // 读取线程 SELinux 上下文
                    thread.selinuxContext = getThreadSelinuxContext(thread.tid);
                    while (!thread.selinuxContext.empty() &&
                           (thread.selinuxContext.back() == '\n' ||
                            thread.selinuxContext.back() == '\r')) {
                        thread.selinuxContext.pop_back();
                    }

                    // 检查线程 SELinux 上下文是否可疑
                    std::string ctxReason;
                    if (isSelinuxContextSuspicious(thread.selinuxContext, ctxReason)) {
                        thread.isSuspicious = true;
                        thread.suspiciousReason = ctxReason;
                    }

                    // 检查线程名称是否可疑
                    std::string nameReason;
                    if (isProcessNameSuspicious(thread.name, nameReason)) {
                        thread.isSuspicious = true;
                        if (!thread.suspiciousReason.empty()) {
                            thread.suspiciousReason += "; ";
                        }
                        thread.suspiciousReason += nameReason;
                    }

                    threads.push_back(thread);
                }
            }

            bpos += d->d_reclen;
        }
    }

    close(fd);
    return threads;
}

// ==================== 公开检测方法 ====================

std::vector<ProcessInfo> ProcessDetector::detectSuspiciousProcesses() {
    std::vector<ProcessInfo> suspiciousProcs;

    auto pids = getAllPids();
    for (pid_t pid : pids) {
        ProcessInfo info = getProcessInfo(pid);
        if (info.isSuspicious) {
            LOGW("发现可疑进程: PID=%d, 名称=%s, 原因=%s",
                 info.pid, info.name.c_str(), info.suspiciousReason.c_str());
            suspiciousProcs.push_back(info);
        }
    }

    return suspiciousProcs;
}

std::vector<ThreadInfo> ProcessDetector::detectSuspiciousThreads() {
    std::vector<ThreadInfo> suspiciousThreads;

    auto pids = getAllPids();
    for (pid_t pid : pids) {
        auto threads = getThreadInfos(pid);
        for (const auto& thread : threads) {
            if (thread.isSuspicious) {
                LOGW("发现可疑线程: TID=%d, 名称=%s, 原因=%s",
                     thread.tid, thread.name.c_str(), thread.suspiciousReason.c_str());
                suspiciousThreads.push_back(thread);
            }
        }
    }

    return suspiciousThreads;
}

ProcessDetectionResult ProcessDetector::detect() {
    ProcessDetectionResult result;
    result.layer = "第5层增强：进程与线程深度检测";
    result.detected = false;
    result.totalProcessesScanned = 0;
    result.totalThreadsScanned = 0;

    LOGI("开始进程与线程深度检测...");

    // 阶段1：扫描所有进程
    auto pids = getAllPids();
    result.totalProcessesScanned = (int)pids.size();
    LOGI("扫描进程总数: %d", result.totalProcessesScanned);

    // 阶段2：检测可疑进程
    for (pid_t pid : pids) {
        ProcessInfo info = getProcessInfo(pid);
        if (info.isSuspicious) {
            LOGW("发现可疑进程: PID=%d, 名称=%s, SELinux=%s, 原因=%s",
                 info.pid, info.name.c_str(),
                 info.selinuxContext.c_str(), info.suspiciousReason.c_str());
            result.suspiciousProcesses.push_back(info);
        }
    }

    // 阶段3：检测可疑线程（扫描所有进程的线程）
    for (pid_t pid : pids) {
        auto threads = getThreadInfos(pid);
        result.totalThreadsScanned += (int)threads.size();
        for (const auto& thread : threads) {
            if (thread.isSuspicious) {
                LOGW("发现可疑线程: TID=%d (PID=%d), 名称=%s, SELinux=%s, 原因=%s",
                     thread.tid, pid, thread.name.c_str(),
                     thread.selinuxContext.c_str(), thread.suspiciousReason.c_str());
                result.suspiciousThreads.push_back(thread);
            }
        }
    }

    LOGI("扫描线程总数: %d", result.totalThreadsScanned);

    // 构建检测结果
    std::vector<std::string> findings;

    if (!result.suspiciousProcesses.empty()) {
        std::string procDetail = "发现 " + std::to_string(result.suspiciousProcesses.size()) +
                                 " 个可疑进程: ";
        for (size_t i = 0; i < result.suspiciousProcesses.size(); i++) {
            if (i > 0) procDetail += "; ";
            const auto& proc = result.suspiciousProcesses[i];
            procDetail += proc.name.empty() ? ("PID:" + std::to_string(proc.pid)) : proc.name;
            procDetail += " [" + proc.suspiciousReason + "]";
        }
        findings.push_back(procDetail);
    }

    if (!result.suspiciousThreads.empty()) {
        std::string threadDetail = "发现 " + std::to_string(result.suspiciousThreads.size()) +
                                   " 个可疑线程: ";
        for (size_t i = 0; i < result.suspiciousThreads.size(); i++) {
            if (i > 0) threadDetail += "; ";
            const auto& thread = result.suspiciousThreads[i];
            threadDetail += "TID:" + std::to_string(thread.tid);
            if (!thread.name.empty()) {
                threadDetail += "(" + thread.name + ")";
            }
            threadDetail += " [" + thread.suspiciousReason + "]";
        }
        findings.push_back(threadDetail);
    }

    if (!findings.empty()) {
        result.detected = true;
        result.detail = "";
        for (size_t i = 0; i < findings.size(); i++) {
            if (i > 0) result.detail += " | ";
            result.detail += findings[i];
        }
        LOGW("进程与线程深度检测完成: 发现 %zu 项异常", findings.size());
    } else {
        result.detail = "未发现可疑进程或线程（扫描进程: " +
                        std::to_string(result.totalProcessesScanned) +
                        ", 扫描线程: " + std::to_string(result.totalThreadsScanned) + "）";
        LOGI("进程与线程深度检测完成: 未发现异常");
    }

    return result;
}
