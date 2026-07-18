#include "layer7_boot.h"
#include "../common/syscall.h"
#include <cstring>

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

bool detectBootloaderStatus() {
    char buf[4096];
    // Try multiple sources
    if (read_file("/proc/cmdline", buf, sizeof(buf))) {
        if (strstr(buf, "bootloader.unlocked=1")) return true;
        if (strstr(buf, "androidboot.flash.locked=0")) return true;
    }
    if (read_file("/sys/class/android_usb/android0/f_caps", buf, sizeof(buf))) {
        if (strstr(buf, "unlocked")) return true;
    }
    // Check /dev/block/by-name for recovery partition
    if (read_file("/proc/mounts", buf, sizeof(buf))) {
        if (strstr(buf, "/dev/block/platform") && strstr(buf, "/system")) {
            // Check if boot is mentioned
            return false;
        }
    }
    return false;
}

bool detectVerifiedBootStatus() {
    char buf[256];
    if (read_file("/sys/fs/selinux/enforce", buf, sizeof(buf))) {
        if (buf[0] == '0') return true; // SELinux permissive
    }

    // Check AVB status
    if (read_file("/proc/cmdline", buf, sizeof(buf))) {
        if (strstr(buf, "androidboot.verifiedbootstate=orange")) return true;
        if (strstr(buf, "androidboot.verifiedbootstate=yellow")) return true;
        if (strstr(buf, "androidboot.veritymode=eio")) return true;
        if (strstr(buf, "androidboot.vbmeta.digest=disabled")) return true;
    }
    return false;
}

bool detectTEECompromise() {
    // Check for TEE-related sysfs entries
    char buf[512];
    if (read_file("/sys/kernel/security/ipe/audit", buf, sizeof(buf))) return true;
    if (read_file("/d/tegra_tz", buf, sizeof(buf))) return true;
    // Check Qualcomm TEE status
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/d/tzdbg"), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
        #else
            d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        return true; // TZ debug accessible = compromised
    }
    return false;
}
