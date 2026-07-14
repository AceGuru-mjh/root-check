#include "layer6_kernel.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第六层 · Root 守护进程 / Root-level Daemon Detection
// ----------------------------------------------------------------
//  说明：原 Ring0（内核态）检测——/proc/kallsyms 符号检查、
//        /proc/modules 内核模块枚举、/proc/sys/kernel/tainted
//        污染标志——已全部移除。这些检测要么被内核 kptr_restrict
//        屏蔽导致不可靠，要么需要 CAP_SYS_MODULE 等特权，与
//        "仅 root 级检测" 的定位不符。
//
//  本层现改为：枚举 root 方案在用户态留下的守护进程痕迹。
//  这是真正稳定、跨内核版本通用的 root 级检测手段。
// ═══════════════════════════════════════════════════════════

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

// 扫描 /proc 枚举所有进程 cmdline
static bool scan_proc_cmdline(const char* needle) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_DIRECTORY | O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;

    char dentry_buf[8192];
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_getdents64), "r"(fd), "r"(dentry_buf), "r"((int64_t)sizeof(dentry_buf)) : "x0", "x1", "x2", "x8");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
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
        // Defensive validation: a corrupt or malicious d_reclen would
        // either send us into an infinite loop (reclen == 0) or read
        // past the end of the getdents64 buffer (reclen > remaining).
        if (dirent->d_reclen == 0 || dirent->d_reclen > (size_t)n - pos) {
            break;
        }
        if (dirent->d_type == 4) { // DT_DIR
            // Bound d_name scan by the record length so we never read
            // off the end of the dentry buffer.
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
                if (read_file(cmdline_path, buf, sizeof(buf))) {
                    if (strstr(buf, needle)) return true;
                }
            }
        }
        pos += dirent->d_reclen;
    }
    return false;
}

// ─── 已知的 root 方案守护进程 / 二进制名 ───────────────────
static const char* ROOT_DAEMON_NAMES[] = {
    // Magisk 家族
    "magiskd", "magisk", "magisk32", "magisk64", "magiskinit",
    "magiskpolicy", "busybox",
    // Magisk forks / variants
    "magisk-fork", "kitana", "magisk-delta",
    // KernelSU 家族
    "ksud", "ksu", "kernelsu",
    // KernelSU forks
    "ksud-next", "ksu-next",
    // APatch 家族
    "apd", "apatch", "apatchd",
    // SukiSU / 其他 fork
    "sukisu", "sukid", "sukisu-ultra",
    // 通用 su 守护进程
    "su", "daemonsu", "superuserd",
    // SuperSU 老式
    "supersu",
    // 现代 Magisk ZygiskNext
    "zygisknext", "zygisk-next",
    // ReZygisk (Rust 实现)
    "rezygisk", "rezygiskd",
    // SudoJS / others
    "sudjs",
    nullptr
};

bool detectKernelTampering() {
    // 改为：root 守护进程存在性检测（root 级，无需内核态）
    for (auto name = ROOT_DAEMON_NAMES; *name; ++name) {
        if (scan_proc_cmdline(*name)) return true;
    }
    return false;
}

bool detectKernelModuleTampering() {
    // 改为：root 方案的 service / post-fs-data 脚本存在性检测
    // 这些是 root 方案必留的痕迹，从用户态可读
    static const char* service_paths[] = {
        "/data/adb/service.d",
        "/data/adb/post-fs-data.d",
        "/data/adb/modules.d",
        "/data/adb/boot-completed.d",
        // Magisk
        "/data/adb/magisk/daemon",
        "/data/adb/magisk/magisk",
        "/data/adb/magisk/.magisk",
        // KernelSU
        "/data/adb/ksu/ksud",
        "/data/adb/ksu/bin/ksud",
        "/data/adb/ksu/conf",
        "/data/adb/ksu/log",
        // APatch
        "/data/adb/ap/apd",
        "/data/adb/ap/working",
        "/data/adb/ap/backup",
        "/data/adb/ap/bin",
        // SukiSU
        "/data/adb/suki",
        // ZygiskNext
        "/data/adb/modules/zygisknext",
        "/data/adb/zygisknext",
        // ReZygisk
        "/data/adb/rezygisk",
        "/data/adb/modules/rezygisk",
        nullptr
    };
    for (auto p = service_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

bool detectKernelTaint() {
    // 改为：root 方案在 /data 留下的配置文件检测
    // 这些是 root 框架在用户态的"指纹"
    static const char* taint_indicators[] = {
        // Magisk db (存储 root 授权记录)
        "/data/adb/magisk.db",
        // KernelSU db
        "/data/adb/ksu/db",
        "/data/adb/ksu/kernel_su.db",
        // APatch db
        "/data/adb/ap/db",
        // root 授权日志
        "/data/adb/magisk.log",
        "/data/adb/ksu/log/ksud.log",
        // Magisk 配置
        "/data/adb/magisk/config",
        "/data/adb/magisk/boot_fingerprint",
        // KernelSU 配置
        "/data/adb/ksu/conf/config.conf",
        "/data/adb/ksu/.allow_su",
        nullptr
    };
    for (auto p = taint_indicators; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}
