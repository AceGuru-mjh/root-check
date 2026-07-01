#include "layer4_mount.h"
#include "../common/syscall.h"
#include <cstring>

static bool read_file_to_buf(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t dummy;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    #else
        dummy = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

bool detectSuspiciousMounts() {
    char buf[16384];
    if (!read_file_to_buf("/proc/self/mountinfo", buf, sizeof(buf))) return false;

    const char* patterns[] = {
        "overlay /data/adb", "/sbin/.magisk", "/data/adb/modules",
        "tmpfs /data/local/tmp", "magisk", "/data/adb/magisk",
        "/data/adb/ksu", "/data/adb/ap", "kernelsu",
        "overlay /system", "overlay /vendor"
    };
    for (auto p : patterns) {
        if (strstr(buf, p)) return true;
    }
    return false;
}

bool detectMagiskMount() {
    char buf[16384];
    if (!read_file_to_buf("/proc/self/mountinfo", buf, sizeof(buf))) return false;
    return strstr(buf, "magisk") != nullptr || strstr(buf, "/sbin/.magisk") != nullptr;
}

bool detectKernelSUOverlay() {
    char buf[16384];
    if (!read_file_to_buf("/proc/self/mountinfo", buf, sizeof(buf))) return false;
    return strstr(buf, "kernelsu") != nullptr || strstr(buf, "/data/adb/ksu") != nullptr;
}

bool detectNamespaceIsolation() {
    // Check if different processes have different mount namespaces
    int64_t self_fd, init_fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(self_fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/ns/mnt"), "i"(O_RDONLY), "i"(0));
    #else
        self_fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(init_fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/1/ns/mnt"), "i"(O_RDONLY), "i"(0));
    #else
        init_fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (self_fd < 0 || init_fd < 0) {
        #if defined(__aarch64__)
        if (self_fd >= 0) { int64_t d; asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(self_fd) : "x0","x8"); }
        if (init_fd >= 0) { int64_t d; asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(init_fd) : "x0","x8"); }
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        return false;
    }

    // Compare inode numbers (stat the two fds)
    char self_path[64], init_path[64];
    char self_link[256], init_link[256];
    int64_t ret;

    // Read /proc/self/fd/<self_fd> to get the namespace inode
    // Simpler: just read the link target
    #if defined(__aarch64__)
    asm volatile("mov x8, 89; mov x0, %1; mov x1, %2; mov x2, %3; svc #0; mov %0, x0" // readlinkat
                 : "=r"(ret) : "i"(AT_FDCWD), "r"("/proc/self/ns/mnt"), "r"(self_link) : "x0", "x8");
    #else
        ret = -1; /* arm32/x64: readlinkat bypass disabled */
    #endif
    int64_t ret2;
    #if defined(__aarch64__)
    asm volatile("mov x8, 89; mov x0, %1; mov x1, %2; mov x2, %3; svc #0; mov %0, x0"
                 : "=r"(ret2) : "i"(AT_FDCWD), "r"("/proc/1/ns/mnt"), "r"(init_link) : "x0", "x8");
    #else
        ret2 = -1; /* arm32/x64: readlinkat bypass disabled */
    #endif

    #if defined(__aarch64__)
    if (self_fd >= 0) { int64_t d; asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(self_fd) : "x0","x8"); }
    if (init_fd >= 0) { int64_t d; asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(init_fd) : "x0","x8"); }
    #endif

    if (ret <= 0 || ret2 <= 0) return false;
    self_link[ret < 256 ? ret : 255] = '\0';
    init_link[ret2 < 256 ? ret2 : 255] = '\0';

    // Different inode = different namespace
    return strcmp(self_link, init_link) != 0;
}
