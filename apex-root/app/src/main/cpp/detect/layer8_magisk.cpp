#include "layer8_magisk.h"
#include "../common/syscall.h"
#include <cstring>

static void read_file_to_buf(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) { if (size > 0) buf[0] = '\0'; return; }
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
    if (n <= 0) { if (size > 0) buf[0] = '\0'; return; }
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
}

bool detectMagiskDaemon() {
    char buf[4096];
    // Check for magiskd in /proc
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_DIRECTORY | O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;

    // Scan /proc for magiskd
    char dentry_buf[4096];
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

    // Parse dirent64 entries for pid directories
    // For each pid, check cmdline
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
        // P0-1 修复: 校验 d_reclen 防止无限循环和越界读取
        if (dirent->d_reclen == 0 || dirent->d_reclen > (size_t)n - pos) {
            break;
        }
        if (dirent->d_type == 4) { // DT_DIR
            // Check if name is all digits (pid)
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
                // Build /proc/<pid>/cmdline — 带边界检查
                int idx = 0;
                const char* pfx = "/proc/";
                for (int i = 0; pfx[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = pfx[i];
                for (int i = 0; dirent->d_name[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = dirent->d_name[i];
                const char* sfx = "/cmdline";
                for (int i = 0; sfx[i] && idx < (int)sizeof(cmdline_path)-1; i++) cmdline_path[idx++] = sfx[i];
                cmdline_path[idx] = '\0';

                read_file_to_buf(cmdline_path, buf, sizeof(buf));
                // 扩充：检测所有 Magisk 家族 daemon
                // magiskd / magisk / magisk32 / magisk64 / kitana / delta / kitsune
                if (strstr(buf, "magiskd") || strstr(buf, "magisk") ||
                    strstr(buf, "kitana") || strstr(buf, "kitsune") ||
                    strstr(buf, "magisk-delta") || strstr(buf, "magisk-fork")) {
                    return true;
                }
            }
        }
        pos += dirent->d_reclen;
    }
    return false;
}

bool detectMagiskModules() {
    char buf[256];
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/data/adb/modules"), "i"(O_DIRECTORY | O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        #else
            d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        return true;
    }
    return false;
}

bool detectMagiskFiles() {
    // 主流 Magisk 路径
    static const char* paths[] = {
        // 经典 Magisk 路径
        "/data/adb/magisk", "/data/adb/magisk.db",
        "/data/adb/magisk.img", "/data/adb/modules",
        "/data/adb/post-fs-data.d", "/data/adb/service.d",
        "/sbin/.magisk", "/data/adb/stock_boot.img",
        "/data/adb/magisk/magisk", "/data/adb/magisk/magisk64",
        "/data/adb/magisk/magiskpolicy", "/data/adb/magisk/busybox",
        // 扩充：Magisk 内部目录
        "/data/adb/magisk/.magisk",
        "/data/adb/magisk/config",
        "/data/adb/magisk/flags",
        "/data/adb/magisk.db-journal",
        "/data/adb/magisk.db-shm",
        "/data/adb/magisk.db-wal",
        // Magisk boot fingerprint
        "/data/adb/magisk/boot_fingerprint",
        // Magisk log
        "/data/adb/magisk.log",
        "/data/adb/magisk.log.old",
        // Magisk Delta / Kitsune (fork)
        "/data/adb/magisk/.delta",
        "/data/adb/magisk/.kitsune",
        "/data/adb/magisk/delta",
        "/data/adb/magisk/kitsune",
        // Magisk Kitsune Manager APP
        "/data/data/io.github.huskydg.magisk",
        // Magisk Alpha / Beta
        "/data/data/com.topjohnwu.magisk.alpha",
        "/data/data/com.topjohnwu.magisk.beta",
        // Magisk WebUI X
        "/data/adb/modules/magisk-webui",
        // 经典 Magisk Manager
        "/data/data/com.topjohnwu.magisk",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        int64_t ret;
        ret = apex_check_access(*p);
        if (ret == 0) return true;
    }
    return false;
}

bool detectZygiskInjection() {
    char buf[16384];
    // Check for Zygisk in /proc/self/maps
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/maps"), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
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
    buf[n < (int64_t)sizeof(buf) ? n : (int64_t)sizeof(buf)-1] = '\0';

    // Check for Zygisk libraries
    // Zygisk is inherently detectable from within the same process
    if (strstr(buf, "zygisk")) return true;
    if (strstr(buf, "lsposed")) return true;
    if (strstr(buf, "riru")) return true;

    // Check for memfd created by ZygiskNext
    if (strstr(buf, "/memfd:")) return true;
    if (strstr(buf, "anon_inode:dmabuf")) {
        // Check for Zygisk-specific dmabuf
    }
    return false;
}
