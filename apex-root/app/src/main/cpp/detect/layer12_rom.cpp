#include "layer12_rom.h"
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

bool detectCustomROM() {
    char buf[4096];
    // Check build properties for custom ROM indicators
    if (read_file("/system/build.prop", buf, sizeof(buf))) {
        const char* custom[] = {
            "ro.custom.build", "ro.mod.version",
            "ro.lineage", "ro.pixelexperience",
            "ro.crdroid", "ro.havoc", "ro.evolution",
            "ro.aospa", "ro.cyanogen", "ro.omni",
            "ro.resurrection", "ro.aosip", "ro.bootleggers",
            "ro.dotui", "ro.stag", "ro.msmxtended",
            "ro.candyrom", "ro.bliss", "ro.xenonhd"
        };
        for (auto s : custom) {
            if (strstr(buf, s)) return true;
        }
    }
    return false;
}

bool detectSELinuxStatus() {
    char buf[16];
    if (!read_file("/sys/fs/selinux/enforce", buf, sizeof(buf))) return false;
    // 0 = permissive, 1 = enforcing
    return buf[0] == '0';
}

bool detectSystemPartitionIntegrity() {
    // Check if system partition is mounted read-only (as expected)
    char buf[16384];
    if (!read_file("/proc/mounts", buf, sizeof(buf))) return false;

    // Check for rw mounts on system partitions
    const char* sys_parts[] = {"/system", "/vendor", "/product", "/odm"};
    char* line = buf;
    char* end = buf + strlen(buf);
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';
        for (auto part : sys_parts) {
            if (strstr(line, part) && strstr(line, " rw,")) {
                return true; // System partition writable = tampered
            }
        }
        line = nl + 1;
    }
    return false;
}
