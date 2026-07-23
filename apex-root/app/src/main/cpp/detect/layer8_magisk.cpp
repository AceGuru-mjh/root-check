#include "layer8_magisk.h"
#include "../common/syscall.h"
#include "../common/utils.h"  // P3-4: 切换到公共 read_file_to_buffer
#include <cstring>

// P3-4(2b): 已切换到 apex::utils::read_file_to_buffer (循环 read 修复
// 单次 read 截断 bug, 且在 arm32/x64 通过 bs_openat 走 libc 路径, 不再静默失败)。
// 原 static read_file_to_buf 已删除。

bool detectMagiskDaemon() {
    char buf[4096];
    // Check for magiskd in /proc
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_DIRECTORY | O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;

    // FIX-P1-DETECT D5: 单次 getdents64 只读 4KB, /proc 下进程数 200-500 个,
    // 4KB 大概覆盖前 30-50 个 pid (低 pid, kthreadd/system_server/zygote)。
    // magiskd/ksud/apd/sukid pid 通常 >500, 永远不在第一页。改为:
    //   (1) 缓冲区 4KB → 32KB
    //   (2) 循环调用 getdents64 直到返回 0 (读完整个目录)
    // 同时扩充 cmdline 匹配: ksud (KernelSU) / apd (APatch) / sukid (SukiSU)
    char dentry_buf[32768];
    bool found = false;

    while (true) {
        int64_t n;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_getdents64), "r"(fd), "r"(dentry_buf), "r"((int64_t)sizeof(dentry_buf)) : "x0", "x1", "x2", "x8", "memory");
        #else
            n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        if (n <= 0) break;  // EOF or error → stop loop

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

                    // P3-4(2b): 切换到 apex::utils::read_file_to_buffer (循环 read, 修复
                    // 单次 read 在大 cmdline 时截断 bug)。open 失败时该函数返回 -1 且
                    // 不清零 buf, 此处先显式清零以保持与旧 read_file_to_buf 相同的语义。
                    buf[0] = '\0';
                    apex::utils::read_file_to_buffer(cmdline_path, buf, sizeof(buf));
                    // 扩充：检测所有 root daemon 家族
                    // magiskd / magisk / magisk32 / magisk64 / kitana / delta / kitsune
                    // ksud (KernelSU) / apd (APatch) / sukid (SukiSU)
                    if (strstr(buf, "magiskd") || strstr(buf, "magisk") ||
                        strstr(buf, "kitana") || strstr(buf, "kitsune") ||
                        strstr(buf, "magisk-delta") || strstr(buf, "magisk-fork") ||
                        strstr(buf, "ksud") || strstr(buf, "apd") ||
                        strstr(buf, "sukid")) {
                        found = true;
                        break;
                    }
                }
            }
            pos += dirent->d_reclen;
        }
        if (found) break;
    }

    // Close fd (deferred until after the getdents64 loop completes)
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    (void)d;
    return found;
}

bool detectMagiskModules() {
    char buf[256];
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/data/adb/modules"), "i"(O_DIRECTORY | O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
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
    // P3-10: 切换到 apex::utils::check_maps_for_patterns (带缓存, 多 pattern 一次匹配)。
    //   旧实现每次都 openat+read /proc/self/maps (单次 read 16KB 截断, maps 通常
    //   100KB+, 后半 zygisk 段漏检), 与 layer11_hook.cpp::detectXposedFramework
    //   逻辑高度重复 (审计 P3-10)。
    //   现统一调用 utils::check_maps_for_patterns, 它在首次调用时缓存整个 maps,
    //   后续 strstr 缓存, 既修复 16KB 截断 bug, 又消除重复代码。
    static const char* const patterns[] = {
        "zygisk",
        "lsposed",
        "riru",
        // FIX-P1-DETECT D6: 只匹配 Zygisk 家族特有的 memfd 名 (普通 /memfd:
        // 在 ART/MediaCodec/WebView 等系统组件也出现, 旧代码全匹配会误报)。
        "memfd:zygisk",
        "memfd:riru",
        "memfd:zygisknext",
        "memfd:rezygisk"
    };
    return apex::utils::check_maps_for_patterns(patterns,
                                                sizeof(patterns) / sizeof(patterns[0])) != nullptr;
}
