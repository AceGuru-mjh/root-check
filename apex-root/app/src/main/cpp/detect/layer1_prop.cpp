#include "layer1_prop.h"
#include "../common/syscall.h"
#include <cstring>

static bool memmem_wrapper(const char* haystack, size_t hlen, const char* needle, size_t nlen) {
    if (nlen > hlen) return false;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool found = true;
        for (size_t j = 0; j < nlen; j++) {
            if (haystack[i + j] != needle[j]) { found = false; break; }
        }
        if (found) return true;
    }
    return false;
}

static bool check_properties_file() {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;

    char buf[8192];
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");

    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");

    if (n <= 0) return false;

    const char* patterns[] = {
        "ro.magisk", "ro.ksu", "ro.apatch",
        "persist.sys.root_access", "ro.debuggable=1",
        "ro.secure=0", "ro.build.type=eng",
        "ro.build.type=userdebug", "ro.build.tags=test-keys",
        "init.svc.magiskd", "init.svc.ksud", "init.svc.apd"
    };

    for (auto pat : patterns) {
        if (memmem_wrapper(buf, n, pat, strlen(pat))) return true;
    }
    return false;
}

bool detectSuspiciousProperties() {
    return check_properties_file();
}

bool detectMagiskProperty() {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    char buf[4096];
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    return memmem_wrapper(buf, n, "init.svc.magiskd", 16);
}

bool detectKernelSUProperty() {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    char buf[4096];
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    return memmem_wrapper(buf, n, "ro.ksu", 6);
}

bool detectAPatchProperty() {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    char buf[4096];
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    return memmem_wrapper(buf, n, "ro.apatch", 9) || memmem_wrapper(buf, n, "init.svc.apd", 12);
}

bool detectDebugProperty() {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    char buf[4096];
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    return memmem_wrapper(buf, n, "ro.debuggable=1", 15) ||
           memmem_wrapper(buf, n, "ro.secure=0", 11) ||
           memmem_wrapper(buf, n, "ro.build.type=eng", 17) ||
           memmem_wrapper(buf, n, "ro.build.type=userdebug", 22) ||
           memmem_wrapper(buf, n, "ro.build.tags=test-keys", 22);
}
