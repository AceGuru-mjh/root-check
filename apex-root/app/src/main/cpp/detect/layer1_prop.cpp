#include "layer1_prop.h"
#include "../common/syscall.h"
#include <cstring>

// v1.0.2 P2-1: 提取重复的 open/read/close 逻辑为 read_properties_file 辅助函数
// 旧实现: check_properties_file / detectMagiskProperty / detectKernelSUProperty /
// detectAPatchProperty / detectDebugProperty 各自重复 30+ 行 syscall 代码。
// 新实现: 一个辅助函数读 /dev/__properties__ 到 buf,各检测函数只需调用并搜索。

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
 * 读取 /dev/__properties__ 到 buf,返回实际读取字节数。
 * 失败返回 -1。调用者负责保证 buf 足够大。
 */
static int64_t read_properties_file(char* buf, size_t buf_size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1;
    #endif
    if (fd < 0) return -1;

    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)buf_size) : "x0", "x1", "x2", "x8");
    #else
        n = -1;
    #endif

    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    #else
        d = -1;
    #endif

    return n;
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
