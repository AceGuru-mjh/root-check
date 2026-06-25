#include "layer10_apatch.h"
#include "../common/syscall.h"
#include <cstring>

bool detectAPatch() {
    const char* paths[] = {
        "/data/adb/ap", "/data/adb/ap/apd",
        "/data/adb/ap/working", "/data/adb/ap/backup",
        "/data/adb/modules/apatch", "/data/adb/apatch"
    };
    for (auto p : paths) {
        int64_t ret;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                     : "=r"(ret) : "i"(__NR_access), "r"(p), "i"(F_OK) : "x0", "x1", "x8");
        if (ret == 0) return true;
    }
    return false;
}

bool detectAPatchKPM() {
    // Check for APatch KPM sysfs node
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/sys/kpm"), "i"(O_DIRECTORY | O_RDONLY), "i"(0));
    if (fd >= 0) {
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        return true;
    }

    // Check for KPM in kallsyms (APatch wipes kallsyms)
    char buf[4096];
    fd = -1;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/kallsyms"), "i"(O_RDONLY), "i"(0));
    if (fd >= 0) {
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        if (n > 0) {
            buf[n < (int64_t)sizeof(buf) ? n : (int64_t)sizeof(buf)-1] = '\0';
            // APatch wipes kallsyms_lookup_name specifically
            if (!strstr(buf, "kallsyms_lookup_name") && n > 100) {
                return true; // Likely wiped by APatch
            }
        }
    }
    return false;
}
