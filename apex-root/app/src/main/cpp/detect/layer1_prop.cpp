#include "layer1_prop.h"
#include "../common/syscall.h"
#include "../bare_syscall/syscall_bridge.h"
#include <cstring>

// v1.0.2 P2-1: 提取重复的 open/read/close 逻辑为 read_properties_file 辅助函数
// 旧实现: check_properties_file / detectMagiskProperty / detectKernelSUProperty /
// detectAPatchProperty / detectDebugProperty 各自重复 30+ 行 syscall 代码。
// 新实现: 一个辅助函数枚举 /dev/__properties__/ 下子文件并拼接到 buf。

static bool memmem_wrapper(const char* haystack, size_t hlen, const char* needle, size_t nlen) {
    if (!haystack || !needle || nlen > hlen) return false;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool found = true;
        for (size_t j = 0; j < nlen; j++) {
            if (haystack[i + j] != needle[j]) { found = false; break; }
        }
        if (found) return true;
    }
    return false;
}

/**
 * 枚举 /dev/__properties__/ 目录下的文件并拼接到 buf。
 * 返回实际填充的总字节数。失败返回 -1。
 *
 * FIX-CPP P0-D4: /dev/__properties__ 在 Android 8+ 是目录而非文件。
 *   原实现直接 openat+read 该路径,read 返回 -EISDIR,函数永远返回 -1,
 *   导致 detectSuspiciousProperties 在所有现代设备上 100% 漏报。
 *   现改为 O_DIRECTORY 打开 + getdents64 枚举 + 对每个子文件 openat+read。
 *
 * FIX-CPP P0-S10: 所有 inline asm 均补齐 clobber 列表 (x0/x1/x2/x8/memory)。
 */
// TODO: 切换到 apex::utils::read_file_to_string (P3-4) — 见 common/utils.h。
// 该函数枚举 /dev/__properties__/ 子文件逐个 read, 复杂度高, 后续 v1.2.0
// plugin 重构时统一迁移到 utils::read_file_to_string。
static int64_t read_properties_file(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return -1;

    // 打开 /dev/__properties__ 目录
    int64_t dirfd = bs_openat(AT_FDCWD, "/dev/__properties__",
                              O_RDONLY | O_DIRECTORY, 0);
    if (dirfd < 0) return -1;

    size_t total = 0;
    char dentry_buf[4096];
    int64_t n;

    // 枚举目录项
    while ((n = bs_getdents64(dirfd, dentry_buf, sizeof(dentry_buf))) > 0) {
        size_t pos = 0;
        while (pos + sizeof(apex_dirent64) <= (size_t)n) {
            auto* d = reinterpret_cast<apex_dirent64*>(dentry_buf + pos);
            if (d->d_reclen == 0 || d->d_reclen > (size_t)n - pos) break;

            const char* name = d->d_name;
            // 跳过 "." 与 ".."
            if (name[0] == '.' && (name[1] == '\0' ||
                                   (name[1] == '.' && name[2] == '\0'))) {
                pos += d->d_reclen;
                continue;
            }

            // 使用相对路径 openat(dirfd, name, ...) 读取每个 property 文件
            int64_t fd = bs_openat(dirfd, name, O_RDONLY, 0);
            if (fd >= 0) {
                size_t remaining = buf_size - total;
                if (remaining > 0) {
                    int64_t got = bs_read(fd, buf + total, remaining);
                    if (got > 0) {
                        total += (size_t)got;
                    }
                }
                bs_close(fd);
                // 缓冲区满则停止
                if (total >= buf_size) {
                    bs_close(dirfd);
                    return (int64_t)total;
                }
            }
            pos += d->d_reclen;
        }
    }
    bs_close(dirfd);
    return (int64_t)total;
}

bool detectSuspiciousProperties() {
    char buf[8192];
    int64_t n = read_properties_file(buf, sizeof(buf));
    if (n <= 0) return false;

    const char* patterns[] = {
        "ro.magisk", "ro.ksu", "ro.apatch",
        "persist.sys.root_access", "ro.debuggable=1",
        "ro.secure=0", "ro.build.type=eng",
        "ro.build.type=userdebug", "ro.build.tags=test-keys",
        "init.svc.magiskd", "init.svc.ksud", "init.svc.apd"
    };

    for (auto pat : patterns) {
        if (memmem_wrapper(buf, (size_t)n, pat, strlen(pat))) return true;
    }
    return false;
}

bool detectMagiskProperty() {
    char buf[4096];
    int64_t n = read_properties_file(buf, sizeof(buf));
    if (n <= 0) return false;
    return memmem_wrapper(buf, (size_t)n, "init.svc.magiskd", 16);
}

bool detectKernelSUProperty() {
    char buf[4096];
    int64_t n = read_properties_file(buf, sizeof(buf));
    if (n <= 0) return false;
    return memmem_wrapper(buf, (size_t)n, "ro.ksu", 6);
}

bool detectAPatchProperty() {
    char buf[4096];
    int64_t n = read_properties_file(buf, sizeof(buf));
    if (n <= 0) return false;
    return memmem_wrapper(buf, (size_t)n, "ro.apatch", 9) || memmem_wrapper(buf, (size_t)n, "init.svc.apd", 12);
}

bool detectDebugProperty() {
    char buf[4096];
    int64_t n = read_properties_file(buf, sizeof(buf));
    if (n <= 0) return false;
    return memmem_wrapper(buf, (size_t)n, "ro.debuggable=1", 15) ||
           memmem_wrapper(buf, (size_t)n, "ro.secure=0", 11) ||
           memmem_wrapper(buf, (size_t)n, "ro.build.type=eng", 17) ||
           memmem_wrapper(buf, (size_t)n, "ro.build.type=userdebug", 22) ||
           memmem_wrapper(buf, (size_t)n, "ro.build.tags=test-keys", 22);
}
