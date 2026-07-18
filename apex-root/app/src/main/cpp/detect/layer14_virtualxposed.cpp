#include "layer14_virtualxposed.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第十四层 · 虚拟框架 / 双开分身检测（root 级）
// ----------------------------------------------------------------
//  VirtualXposed / 太极 / 分身大师等虚拟化框架可在不 root 的
//  情况下注入 Xposed 模块到目标进程，绕过传统 root 检测。
//  本层从 root 级路径 + 进程 cmdline 两条线检测。
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

// 扫描 /proc 找特定 cmdline
static bool scan_proc_for(const char* needle) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_DIRECTORY | O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    char dentry_buf[8192];
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_getdents64), "r"(fd), "r"(dentry_buf), "r"((int64_t)sizeof(dentry_buf)) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    struct linux_dirent64 {
        uint64_t d_ino;
        int64_t d_off;
        unsigned short d_reclen;
        unsigned char d_type;
        char d_name[];
    };
    size_t pos = 0;
    while (pos + sizeof(linux_dirent64) <= (size_t)n) {
        auto* dirent = (linux_dirent64*)(dentry_buf + pos);
        // Defensive validation — see layer6_kernel.cpp for rationale.
        if (dirent->d_reclen == 0 || dirent->d_reclen > (size_t)n - pos) {
            break;
        }
        if (dirent->d_type == 4) {
            const char* name_end = (const char*)dirent + dirent->d_reclen;
            bool all_digits = true;
            bool any_char = false;
            for (const char* c = dirent->d_name; c < name_end && *c; c++) {
                any_char = true;
                if (*c < '0' || *c > '9') {
                    all_digits = false; break;
                }
            }
            if (any_char && all_digits) {
                char cmdline_path[64];
                int idx = 0;
                const char* pfx = "/proc/";
                for (int i = 0; pfx[i] && idx < (int)sizeof(cmdline_path) - 1; i++) cmdline_path[idx++] = pfx[i];
                for (int i = 0; dirent->d_name[i] && idx < (int)sizeof(cmdline_path) - 1; i++) cmdline_path[idx++] = dirent->d_name[i];
                const char* sfx = "/cmdline";
                for (int i = 0; sfx[i] && idx < (int)sizeof(cmdline_path) - 1; i++) cmdline_path[idx++] = sfx[i];
                cmdline_path[idx] = '\0';
                char buf[512];
                if (read_file(cmdline_path, buf, sizeof(buf)) && strstr(buf, needle)) {
                    return true;
                }
            }
        }
        pos += dirent->d_reclen;
    }
    return false;
}

bool detectVirtualXposed() {
    // VirtualXposed 是最早的无 root Xposed 框架
    static const char* vx_paths[] = {
        "/data/data/io.va.exposed",
        "/data/data/io.va.exposed64",
        "/data/data/com.va.exposed",
        "/data/data/io.virtualapp.exposed",
        "/data/user/0/io.va.exposed",
        nullptr
    };
    for (auto p = vx_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    // 进程内 maps 检测
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "VirtualXposed")) return true;
        if (strstr(buf, "io.va.exposed")) return true;
    }
    // 进程扫描
    if (scan_proc_for("io.va.exposed")) return true;
    return false;
}

bool detectTaiChi() {
    // 太极·阳（免 root 版本，可作为分身运行 Xposed 模块）
    static const char* taichi_paths[] = {
        "/data/data/me.weishu.exp",
        "/data/data/me.weishu.exp.app",
        "/data/data/me.weishu.taichi",
        "/data/data/me.weishu.taichi.app",
        "/data/user/0/me.weishu.exp",
        nullptr
    };
    for (auto p = taichi_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    if (scan_proc_for("me.weishu.exp")) return true;
    if (scan_proc_for("me.weishu.taichi")) return true;
    return false;
}

bool detectDualSpaceApps() {
    // 各类双开/分身应用（用户态虚拟化）
    static const char* dual_paths[] = {
        // 分身大师
        "/data/data/com.lbe.parallel",
        "/data/data/com.lbe.parallel.intl",
        // 平行空间
        "/data/data/com.excelliance.dualaid",
        "/data/data/com.excelliance.dualaid.friend",
        // 双开大师
        "/data/data/com.ludashi.dualspace",
        // Multiple Accounts
        "/data/data/com.multiple.accounts",
        // Parallel Space (国际版)
        "/data/data/com.excelliance.multiaccount",
        // 360 分身
        "/data/data/com.qihoo360.dualspace",
        // 微信分身
        "/data/data/com.tencent.mobileqqdual",
        nullptr
    };
    for (auto p = dual_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

bool detectAppCloningFrameworks() {
    // 现代 Android 12+ 原生支持应用克隆，但旧克隆框架仍可能被滥用
    static const char* cloning_indicators[] = {
        // Island (应用沙箱)
        "/data/data/com.oasisfeng.island",
        // Shelter
        "/data/data/com.oasisfeng.shelter",
        // Insular
        "/data/data/com.oasisfeng.island.fdroid",
        // App Cloner
        "/data/data/com.applisto.apps.cloner",
        // 双开助手
        "/data/data/com.boly.wxhacker",
        nullptr
    };
    for (auto p = cloning_indicators; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

int virtualXposedFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectVirtualXposed()) {
        append("[VirtualXposed] VirtualXposed framework detected\n"); findings++;
    }
    if (detectTaiChi()) {
        append("[TaiChi] TaiChi framework detected\n"); findings++;
    }
    if (detectDualSpaceApps()) {
        append("[DualSpace] Dual space app detected\n"); findings++;
    }
    if (detectAppCloningFrameworks()) {
        append("[AppClone] App cloning framework detected\n"); findings++;
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
