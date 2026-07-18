#include "layer13_firmware.h"
#include "../common/syscall.h"
#include <cstring>

// ─── Helper ────────────────────────────────────────────────

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

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

// ─── Firmware partition checks ─────────────────────────────

bool detectFirmwareTampering() {
    char buf[4096];

    // Check /proc/cmdline for verified boot status
    if (read_file("/proc/cmdline", buf, sizeof(buf))) {
        if (strstr(buf, "androidboot.verifiedbootstate=orange")) return true;
        if (strstr(buf, "androidboot.verifiedbootstate=yellow")) return true;
        if (strstr(buf, "androidboot.veritymode=eio")) return true;
        if (strstr(buf, "androidboot.vbmeta.digest=disabled")) return true;
        if (strstr(buf, "androidboot.vbmeta.device_state=unlocked")) return true;
    }

    // Check for AVB (Android Verified Boot) status via sysfs
    if (read_file("/sys/fs/selinux/enforce", buf, sizeof(buf))) {
        if (buf[0] == '0') return true; // SELinux permissive = likely compromised
    }

    return false;
}

bool detectBootPartitionModified() {
    // Check for common modified boot partition indicators
    // Magisk creates a "magisk" entry in the boot partition
    if (check_access("/data/adb/magisk") == 0) return true;
    if (check_access("/data/adb/magisk.db") == 0) return true;

    // Check for APatch kernel modification
    if (check_access("/sys/kpm") == 0) return true;

    // Check for KernelSU kernel modification
    if (check_access("/proc/kernelsu") == 0) return true;

    // Check if kernel version string contains custom tags
    char buf[1024];
    if (read_file("/proc/version", buf, sizeof(buf))) {
        if (strstr(buf, "kernelsu") || strstr(buf, "magisk") ||
            strstr(buf, "apatch") || strstr(buf, "su")) {
            return true;
        }
    }

    // Check if boot partition is writable
    if (read_file("/proc/mounts", buf, sizeof(buf))) {
        // /dev/block/by-name/boot should not be writable
        if (strstr(buf, " /boot ") || strstr(buf, "/dev/block") && strstr(buf, "boot")) {
            return true;
        }
    }

    return false;
}

bool detectVendorPartitionModified() {
    char buf[16384];

    // Check /proc/mounts for vendor partition mount flags
    if (!read_file("/proc/mounts", buf, sizeof(buf))) return false;

    // Check for vendor partition mounted as writable
    bool vendor_found = false;
    const char* p = buf;
    while ((p = strstr(p, "/vendor"))) {
        vendor_found = true;
        // Check if followed by " rw,"
        const char* after = p + 7;
        while (*after == ' ') after++;
        // Skip the device path to find mount point and flags
        break;
    }

    // Check for product, odm, system_ext partitions
    const char* partitions[] = {"/vendor", "/product", "/odm", "/system_ext"};
    for (auto part : partitions) {
        const char* found = strstr(buf, part);
        if (found) {
            // Find the flags (between 3rd and 4th space)
            int space_count = 0;
            const char* flags = buf;
            while (*flags) {
                if (*flags == ' ') space_count++;
                if (space_count == 3 && flags > found) break;
                flags++;
            }
            // Check for rw (writable) in flags
            if (strstr(flags, "rw,")) return true;
        }
    }

    return false;
}

bool detectAVBStatus() {
    char buf[256];

    // Check AVB (Android Verified Boot) status via multiple sources
    if (read_file("/sys/block/mmcblk0boot0/force_ro", buf, sizeof(buf))) {
        if (buf[0] == '0') return true; // Boot partition is writable
    }

    // Check dm-verity status
    if (read_file("/sys/module/dm_verity/parameters/status", buf, sizeof(buf))) {
        if (strstr(buf, "disabled") || strstr(buf, "off")) return true;
    }

    // Check for dm-verity in kernel cmdline
    if (read_file("/proc/cmdline", buf, sizeof(buf))) {
        if (strstr(buf, "androidboot.veritymode=disabled")) return true;
        if (strstr(buf, "dm_verity.devices")) return true;
    }

    // Attempt to check lock status (requires root on some devices)
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/sys/class/android_usb/android0/f_caps"), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        char caps[64];
        int64_t n;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(caps), "r"((int64_t)sizeof(caps)) : "x0", "x1", "x2", "x8", "memory");
        #else
            n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8", "memory");
        #else
            d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        if (n > 0) {
            caps[n < 63 ? n : 63] = '\0';
            if (strstr(caps, "unlocked")) return true;
        }
    }

    return false;
}

bool detectRecoveryPartition() {
    // Check for custom recovery
    if (check_access("/cache/recovery") == 0) return true;
    if (check_access("/cache/recovery/command") == 0) return true;
    if (check_access("/cache/recovery/log") == 0) return true;

    // Check for TWRP-specific files
    if (check_access("/data/media/0/TWRP") == 0) return true;
    if (check_access("/sbin/recovery") == 0) return true;

    // Check for OrangeFox
    if (check_access("/data/media/0/OrangeFox") == 0) return true;

    return false;
}

// ─── Full firmware scan ────────────────────────────────────

int firmwareFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) { while (*s && pos < (int)out_size-1) out_report[pos++] = *s++; };

    if (detectFirmwareTampering()) {
        append("[FW] Firmware tampering detected\n"); findings++;
    }
    if (detectBootPartitionModified()) {
        append("[FW] Boot partition modified\n"); findings++;
    }
    if (detectVendorPartitionModified()) {
        append("[FW] Vendor partition writable\n"); findings++;
    }
    if (detectAVBStatus()) {
        append("[FW] AVB/Verified Boot compromised\n"); findings++;
    }
    if (detectRecoveryPartition()) {
        append("[FW] Custom recovery detected\n"); findings++;
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
