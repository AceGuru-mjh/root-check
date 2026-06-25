#include "layer9_ksu.h"
#include "../common/syscall.h"
#include <cstring>

bool detectKernelSU() {
    const char* paths[] = {
        "/data/adb/ksu", "/data/adb/ksu/ksud",
        "/data/adb/modules/kernelsu", "/data/adb/ksu.db",
        "/data/adb/modules/kernelsu/kernelsu",
        "/data/adb/modules/kernelsu/post-fs-data.sh",
        "/data/adb/modules/kernelsu/service.sh",
        "/data/adb/modules/kernelsu/uninstall.sh"
    };
    for (auto p : paths) {
        int64_t ret;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                     : "=r"(ret) : "i"(__NR_access), "r"(p), "i"(F_OK) : "x0", "x1", "x8");
        if (ret == 0) return true;
    }

    // Check for KSU proc node
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/kernelsu"), "i"(O_RDONLY), "i"(0));
    if (fd >= 0) {
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        return true;
    }
    return false;
}

bool detectKernelSUModule() {
    char buf[8192];
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/modules"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t d;
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    if (n <= 0) return false;
    buf[n < (int64_t)sizeof(buf) ? n : (int64_t)sizeof(buf)-1] = '\0';

    if (strstr(buf, "kernelsu")) return true;
    if (strstr(buf, "ksu")) return true;
    return false;
}
