#include "layer17_modern_root_forks.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

// ═══════════════════════════════════════════════════════════
//  第十七层 · 现代 Root 方案 Fork 检测 (root 级)
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
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
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
    #else
        d = -1;
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

// 扫描 /proc 枚举所有进程 cmdline
static bool scan_proc_cmdline(const char* needle) {
    if (!needle) return false;
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_DIRECTORY | O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1;
    #endif
    if (fd < 0) return false;
    char dentry_buf[8192];
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_getdents64), "r"(fd), "r"(dentry_buf), "r"((int64_t)sizeof(dentry_buf)) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1;
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
    #else
        d = -1;
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
        if (dirent->d_reclen == 0 || dirent->d_reclen > (size_t)n - pos) break;
        if (dirent->d_type == 4) { // DT_DIR
            const char* name_end = (const char*)dirent + dirent->d_reclen;
            bool all_digits = true;
            bool any_char = false;
            for (const char* c = dirent->d_name; c < name_end && *c; c++) {
                any_char = true;
                if (*c < '0' || *c > '9') { all_digits = false; break; }
            }
            if (any_char && all_digits) {
                char cmdline_path[64];
                int idx = 0;
                const char* pfx = "/proc/";
                for (int i = 0; pfx[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = pfx[i];
                for (int i = 0; dirent->d_name[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = dirent->d_name[i];
                const char* sfx = "/cmdline";
                for (int i = 0; sfx[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = sfx[i];
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

// ─── SukiSU / SukiSU-Ultra ─────────────────────────────────
// SukiSU 是 KernelSU 的活跃 fork,在中文社区流行,有 Ultra 变体。
bool detectSukiSU() {
    static const char* paths[] = {
        "/data/adb/suki",
        "/data/adb/sukisu",
        "/data/adb/suki/bin/ksud",
        "/data/adb/sukisu/bin/ksud",
        "/data/adb/suKSU",
        "/data/adb/sukisu/conf",
        "/data/adb/sukisu/log",
        "/data/adb/suki/working",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("sukid") || scan_proc_cmdline("sukisu");
}

bool detectSukiSUUltra() {
    static const char* paths[] = {
        "/data/adb/sukiultra",
        "/data/adb/sukisu-ultra",
        "/data/adb/suki/ultra",
        "/data/adb/sukisu/ultra",
        "/data/adb/suKSUUltra",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("sukiultra") || scan_proc_cmdline("sukisu-ultra");
}

// ─── ReZygisk (Rust 实现的 Zygisk 替代) ────────────────────
bool detectReZygisk() {
    static const char* paths[] = {
        "/data/adb/rezygisk",
        "/data/adb/modules/rezygisk",
        "/data/adb/rezygisk/bin",
        "/data/adb/rezygisk/manifest.json",
        "/data/adb/rezygisk/loader.so",
        "/data/adb/rezygisk/librezygisk.so",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("rezygisk") || scan_proc_cmdline("rezygiskd");
}

bool detectReZygiskVariants() {
    // 社区 fork 变体
    static const char* paths[] = {
        "/data/adb/rezygisk-next",
        "/data/adb/zygisk-rs",
        "/data/adb/rzygisk",
        "/data/adb/rezygisk/manifest.json.next",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("rezygisk-next");
}

// ─── Magisk Delta / Kitsune / Alpha / Beta ────────────────
// Magisk Delta (原 Magisk Kitsune) 是 Magisk 的活跃 fork,
// 增加了 Shamiko 兼容性、MagiskHide 复活等功能。
bool detectMagiskDelta() {
    static const char* paths[] = {
        "/data/adb/magisk/delta",
        "/data/adb/magisk/.delta",
        "/data/adb/magisk-delta",
        "/data/adb/magiskdelta",
        "/data/adb/magisk/version-delta",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    // 进程扫描 (Magisk Delta 守护进程名可能保持 magiskd,但有些版本改为 magiskd-delta)
    return scan_proc_cmdline("magisk-delta") || scan_proc_cmdline("magiskdelta");
}

bool detectMagiskKitsune() {
    static const char* paths[] = {
        "/data/adb/magisk/kitsune",
        "/data/adb/magisk/.kitsune",
        "/data/adb/magisk-kitsune",
        "/data/adb/magiskkitsune",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("magisk-kitsune") || scan_proc_cmdline("kitsune");
}

bool detectMagiskAlphaBeta() {
    // Alpha/Beta channel 的 Magisk 通常有独立的 config 文件
    static const char* paths[] = {
        "/data/adb/magisk/.alpha",
        "/data/adb/magisk/.beta",
        "/data/adb/magisk/alpha",
        "/data/adb/magisk/beta",
        "/data/adb/magisk/canary",
        "/data/adb/magisk/.canary",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── ZygiskNext Delta / variants ──────────────────────────
bool detectZygiskNextDelta() {
    static const char* paths[] = {
        "/data/adb/zygisknext-delta",
        "/data/adb/modules/zygisknext_delta",
        "/data/adb/modules/zygisk-next-delta",
        "/data/adb/zygisknext/next",
        "/data/adb/zygisknext/delta",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return scan_proc_cmdline("zygisknext-delta");
}

// ─── 主检测入口 ───────────────────────────────────────────
bool detectModernRootForks() {
    return detectSukiSU() ||
           detectSukiSUUltra() ||
           detectReZygisk() ||
           detectReZygiskVariants() ||
           detectMagiskDelta() ||
           detectMagiskKitsune() ||
           detectMagiskAlphaBeta() ||
           detectZygiskNextDelta();
}

int modernRootForksFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectSukiSU()) { append("[L17] SukiSU detected\n"); findings++; }
    if (detectSukiSUUltra()) { append("[L17] SukiSU-Ultra detected\n"); findings++; }
    if (detectReZygisk()) { append("[L17] ReZygisk detected\n"); findings++; }
    if (detectReZygiskVariants()) { append("[L17] ReZygisk variant detected\n"); findings++; }
    if (detectMagiskDelta()) { append("[L17] Magisk Delta detected\n"); findings++; }
    if (detectMagiskKitsune()) { append("[L17] Magisk Kitsune detected\n"); findings++; }
    if (detectMagiskAlphaBeta()) { append("[L17] Magisk Alpha/Beta/Canary detected\n"); findings++; }
    if (detectZygiskNextDelta()) { append("[L17] ZygiskNext Delta detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
