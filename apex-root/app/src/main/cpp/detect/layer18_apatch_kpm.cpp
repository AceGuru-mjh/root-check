#include "layer18_apatch_kpm.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1;
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1;
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
    #else
        d = -1;
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

// ─── APatch KPM 模块目录 ──────────────────────────────────
bool detectAPatchKPMUsermode() {
    static const char* kpm_paths[] = {
        // KPM 模块目录
        "/data/adb/ap/kpm",
        "/data/adb/ap/kpm/modules",
        "/data/adb/ap/kpm/loaded",
        "/data/adb/ap/kpm/installed",
        // KPM 配置
        "/data/adb/ap/kpm.conf",
        "/data/adb/ap/kpm_config",
        "/data/adb/ap/.kpm",
        // KPM 加载日志
        "/data/adb/ap/kpm.log",
        "/data/adb/ap/log/kpm.log",
        // KPM 备份
        "/data/adb/ap/kpm/backup",
        nullptr
    };
    for (auto p = kpm_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── APatch Trampoline 后门 ───────────────────────────────
// APatch 通过特定 syscall 参数 (truncate 的扩展用法) 触发内核代码执行。
// 用户态无法直接检测后门是否激活,但可以检测相关配置和守护进程。
bool detectAPatchTrampoline() {
    static const char* tramp_paths[] = {
        // Trampoline 配置
        "/data/adb/ap/trampoline",
        "/data/adb/ap/.trampoline",
        "/data/adb/ap/tramp",
        // Sigpatch (signature verification bypass)
        "/data/adb/ap/sigpatch",
        "/data/adb/ap/.sigpatch",
        "/data/adb/ap/sig",
        // KPM selinux bypass
        "/data/adb/ap/kpmselinux",
        "/data/adb/ap/.kpmselinux",
        nullptr
    };
    for (auto p = tramp_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── APatch 完整组件 ──────────────────────────────────────
bool detectAPatchComponents() {
    static const char* ap_paths[] = {
        // 主守护进程
        "/data/adb/ap/apd",
        "/data/adb/ap/bin/apd",
        "/data/adb/ap/apd.bin",
        // 工作目录
        "/data/adb/ap/working",
        "/data/adb/ap/working_dir",
        // 备份
        "/data/adb/ap/backup",
        "/data/adb/ap/bak",
        // 数据库 (root 授权记录)
        "/data/adb/ap/db",
        "/data/adb/ap/apatch.db",
        // 配置
        "/data/adb/ap/conf",
        "/data/adb/ap/config",
        "/data/adb/ap/.config",
        // 日志
        "/data/adb/ap/log",
        "/data/adb/ap/logs",
        "/data/adb/ap/apd.log",
        // 临时文件
        "/data/adb/ap/tmp",
        "/data/adb/ap/.tmp",
        nullptr
    };
    for (auto p = ap_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── KernelPatch 项目 (APatch 的内核部分) ─────────────────
// APatch 基于 KernelPatch 项目 (kernelpatch.kernelsu.org)。
// KernelPatch 本身也有用户态痕迹。
bool detectKernelPatchProject() {
    static const char* kp_paths[] = {
        // KernelPatch 用户态
        "/data/adb/kp",
        "/data/adb/kpatch",
        "/data/adb/kernelpatch",
        "/data/adb/kp/bin",
        "/data/adb/kp/conf",
        "/data/adb/kp/log",
        // KernelPatch 模块
        "/data/adb/kp/modules",
        "/data/adb/kp/installed",
        // SuperKey (KernelPatch 的特权密钥)
        "/data/adb/kp/superkey",
        "/data/adb/kp/.superkey",
        "/data/adb/kp/super_key",
        nullptr
    };
    for (auto p = kp_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

int apatchKpmFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectAPatchKPMUsermode()) { append("[L18] APatch KPM modules detected\n"); findings++; }
    if (detectAPatchTrampoline()) { append("[L18] APatch trampoline/sigpatch detected\n"); findings++; }
    if (detectAPatchComponents()) { append("[L18] APatch components detected\n"); findings++; }
    if (detectKernelPatchProject()) { append("[L18] KernelPatch project detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
