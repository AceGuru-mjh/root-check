#include "layer22_emulator.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

// ═══════════════════════════════════════════════════════════
//  第二十二层 · 模拟器检测
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1;
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    #else
        n = -1;
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    #else
        d = -1;
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

// ─── QEMU 检测 ────────────────────────────────────────────
bool detectQEMU() {
    static const char* qemu_paths[] = {
        "/dev/qemu_pipe",
        "/dev/socket/qemud",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/sys/qemu_trace",
        nullptr
    };
    for (auto p = qemu_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    // 检查 /proc/cpuinfo 中的 QEMU 标记
    char buf[4096];
    if (read_file("/proc/cpuinfo", buf, sizeof(buf))) {
        if (strstr(buf, "QEMU") || strstr(buf, "qemu")) return true;
        if (strstr(buf, "goldfish") || strstr(buf, "ranchu")) return true;
    }
    return false;
}

// ─── Genymotion 检测 ──────────────────────────────────────
bool detectGenymotion() {
    static const char* geny_paths[] = {
        "/dev/socket/genyd",
        "/dev/socket/baseband_genyd",
        "/system/lib/libc_malloc_debug_qemu.so",
        nullptr
    };
    for (auto p = geny_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    // Genymotion 使用 vbox (VirtualBox)
    char buf[4096];
    if (read_file("/proc/cpuinfo", buf, sizeof(buf))) {
        if (strstr(buf, "vbox") || strstr(buf, "VBOX")) return true;
    }
    return false;
}

// ─── BlueStacks 检测 ──────────────────────────────────────
bool detectBluestacks() {
    static const char* bs_paths[] = {
        "/data/data/com.bluestacks",
        "/data/data/com.bluestacks.home",
        "/sdcard/windows/BstSharedFolder",
        "/system/bin/bstfolder",
        nullptr
    };
    for (auto p = bs_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Nox / LDPlayer / MEmu 检测 ───────────────────────────
bool detectNoxOrLDPlayer() {
    static const char* nox_paths[] = {
        // Nox
        "/data/data/com.bignox.app",
        "/system/bin/nox",
        "/proc/nox",
        // LDPlayer
        "/data/data/com.ldmnq", 
        "/system/bin/ldnemu",
        // MEmu
        "/data/data.com.microvirt",
        "/system/bin/memu",
        nullptr
    };
    for (auto p = nox_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 通用模拟器属性检测 ───────────────────────────────────
bool detectGenericEmulatorProps() {
    char buf[8192];
    // 读 /dev/__properties__ 检查模拟器特有属性
    if (read_file("/dev/__properties__", buf, sizeof(buf))) {
        if (strstr(buf, "ro.kernel.qemu=1")) return true;
        if (strstr(buf, "ro.hardware=goldfish")) return true;
        if (strstr(buf, "ro.hardware=ranchu")) return true;
        if (strstr(buf, "ro.product.model=SDK")) return true;
        if (strstr(buf, "ro.product.brand=google_sdk")) return true;
        if (strstr(buf, "ro.product.brand=generic_x86")) return true;
        if (strstr(buf, "ro.boot.hardware=qemu")) return true;
    }
    // 检查 init.svc.qemu-props
    if (check_access("/proc/self/fd/0") == 0) {
        // 读 /proc/self/cmdline — 模拟器进程名通常含 emulator/qemu
        char cmd[256];
        if (read_file("/proc/self/cmdline", cmd, sizeof(cmd))) {
            if (strstr(cmd, "emulator")) return true;
        }
    }
    return false;
}

// ─── 主检测入口 ───────────────────────────────────────────
bool detectEmulator() {
    return detectQEMU() ||
           detectGenymotion() ||
           detectBluestacks() ||
           detectNoxOrLDPlayer() ||
           detectGenericEmulatorProps();
}

int emulatorFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectQEMU()) { append("[L22] QEMU emulator detected\n"); findings++; }
    if (detectGenymotion()) { append("[L22] Genymotion detected\n"); findings++; }
    if (detectBluestacks()) { append("[L22] BlueStacks detected\n"); findings++; }
    if (detectNoxOrLDPlayer()) { append("[L22] Nox/LDPlayer/MEmu detected\n"); findings++; }
    if (detectGenericEmulatorProps()) { append("[L22] Generic emulator properties detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
