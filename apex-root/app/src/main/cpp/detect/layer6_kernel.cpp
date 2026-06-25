#include "layer6_kernel.h"
#include "../common/syscall.h"
#include <cstring>

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    int64_t d;
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

bool detectKernelTampering() {
    char buf[4096];

    // Check kernel version string for custom kernels
    if (read_file("/proc/version", buf, sizeof(buf))) {
        const char* custom = strstr(buf, "kernelsu");
        if (custom) return true;
        custom = strstr(buf, "apatch");
        if (custom) return true;
        custom = strstr(buf, "magisk");
        if (custom) return true;
        custom = strstr(buf, "su");
        if (custom) return true;
        custom = strstr(buf, "dirtypipe");
        if (custom) return true;
        custom = strstr(buf, "dirtycow");
        if (custom) return true;
    }

    // Check kernel command line for root-related flags
    if (read_file("/proc/cmdline", buf, sizeof(buf))) {
        if (strstr(buf, "androidboot.verifiedbootstate=orange")) return true;
        if (strstr(buf, "androidboot.flash.locked=0")) return true;
        if (strstr(buf, "bootloader.unlocked=1")) return true;
        if (strstr(buf, "androidboot.veritymode=enforcing")) return false; // normal
    }

    // Check kallsyms for hooked symbols
    if (read_file("/proc/kallsyms", buf, sizeof(buf))) {
        // If all symbols show 0x0000000000000000, kallsyms has been wiped
        // This is a common Root hiding technique (APatch does this)
        int zero_count = 0;
        int total_lines = 0;
        char* line = buf;
        char* end = buf + strlen(buf);
        while (line < end) {
            char* nl = line;
            while (nl < end && *nl != '\n') nl++;
            *nl = '\0';
            total_lines++;
            if (strncmp(line, "0000000000000000", 16) == 0) zero_count++;
            line = nl + 1;
        }
        if (total_lines > 10 && zero_count > total_lines / 2) return true;
    }

    return false;
}

bool detectKernelModuleTampering() {
    char buf[8192];
    if (!read_file("/proc/modules", buf, sizeof(buf))) return false;

    const char* suspicious[] = {
        "rootkit", "hide_proc", "sec_hook", "magisk",
        "kernelsu", "kpm", "apatch", "ksu",
        "overlay", "wireguard" // WireGuard on non-official kernel
    };
    for (auto s : suspicious) {
        if (strstr(buf, s)) return true;
    }
    return false;
}

bool detectKernelTaint() {
    char buf[64];
    if (!read_file("/proc/sys/kernel/tainted", buf, sizeof(buf))) return false;
    int taint = 0;
    for (int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++) {
        taint = taint * 10 + (buf[i] - '0');
    }
    // Taint flags: bit 0=proprietary module, bit 1=force unload, bit 3=unsigned module
    return taint != 0;
}
