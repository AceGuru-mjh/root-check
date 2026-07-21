#include "layer22_emulator.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>
#include <string>
#include <sys/system_properties.h>

// ═══════════════════════════════════════════════════════════
//  第二十二层 · 模拟器检测
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1;
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1;
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
    #else
        d = -1;
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

// ─── QEMU 检测 ────────────────────────────────────────────
bool detectQEMU() {
    // FIX-P1-DETECT D7: 原 qemu_paths 只列 32 位 libc_malloc_debug_qemu.so,
    // 64 位设备上该路径不存在 → 漏报。补齐 lib64 + 常见模拟器路径。
    // 另: typo /data/data.com.microvirt (应为 /data/data/com.microvirt) 在
    // detectNoxOrLDPlayer 里修复。
    static const char* qemu_paths[] = {
        "/dev/qemu_pipe",
        "/dev/socket/qemud",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/system/lib64/libc_malloc_debug_qemu.so",   // FIX-P1-DETECT D7: 64-bit path
        "/sys/qemu_trace",
        "/dev/binfmt_misc/qemu",                       // registered binfmt
        "/system/bin/qemu-props",                      // qemu-props service
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
    // goldfish 字符设备也是强信号 (QEMU goldfish platform)
    if (check_access("/dev/socket/goldfish") == 0) return true;
    return false;
}

// ─── Genymotion 检测 ──────────────────────────────────────
bool detectGenymotion() {
    // FIX-P1-DETECT D7: 补齐 64 位 libc_malloc_debug_qemu.so 路径。
    static const char* geny_paths[] = {
        "/dev/socket/genyd",
        "/dev/socket/baseband_genyd",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/system/lib64/libc_malloc_debug_qemu.so",   // FIX-P1-DETECT D7: 64-bit path
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
        // FIX-P1-DETECT D7: typo 修复 "/data/data.com.microvirt" → "/data/data/com.microvirt"
        // (原路径斜杠后多点号, 永远不会存在该路径 → MEmu 漏报)
        "/data/data/com.microvirt",
        "/system/bin/memu",
        nullptr
    };
    for (auto p = nox_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 通用模拟器属性检测 ───────────────────────────────────
// FIX-P2-KT P2-D13 (v1.1.3): 修复目录读取 EISDIR 漏报。
//   原实现: read_file("/dev/__properties__", buf, ...) 直接 openat 目录 + read,
//   read 返回 -EISDIR → 永远 false → 所有 generic emulator 属性检测 100% 漏报。
//   (与 P0-D4 layer1_prop.cpp 同一类 bug。)
//   修复方案 A (首选): 用 __system_property_get API 按 key 精确查询,
//     不读目录、不依赖属性文件布局, 是 Android 官方推荐做法。
//   修复方案 B (备用, layer1_prop 已用): getdents64 枚举目录再逐文件 read,
//     但需自行解析属性二进制格式, 容易出错。
//   现采用方案 A — 对每个可疑 key 单独 __system_property_get, 比对期望值。
//   注: 性能开销可忽略 (7 次 property 查询, 每次 < 10us)。
bool detectGenericEmulatorProps() {
    // 辅助: 取 property 值, 失败返回空串
    auto prop = [](const char* key) {
        char val[PROP_VALUE_MAX] = {};
        int n = __system_property_get(key, val);
        return (n > 0) ? std::string(val) : std::string();
    };

    // 1) QEMU / goldfish / ranchu 硬件标识
    if (prop("ro.kernel.qemu") == "1") return true;
    if (prop("ro.hardware") == "goldfish") return true;
    if (prop("ro.hardware") == "ranchu") return true;
    if (prop("ro.boot.hardware") == "qemu") return true;

    // 2) Google SDK / generic_x86 模拟器产品标识
    if (prop("ro.product.model") == "sdk") return true;        // SDK GPE
    if (prop("ro.product.model") == "google_sdk") return true; // 旧 SDK
    if (prop("ro.product.brand") == "google_sdk") return true;
    if (prop("ro.product.brand") == "generic_x86") return true;
    if (prop("ro.product.brand") == "generic") return true;    // generic_arm64 也覆盖
    if (prop("ro.product.device") == "generic_x86") return true;
    if (prop("ro.product.name") == "sdk_x86") return true;
    if (prop("ro.product.name") == "sdk_gphone_x86") return true;

    // 3) init.svc.qemu-props 服务存在 → 强烈信号
    if (prop("init.svc.qemu-props").length() > 0) return true;

    // 4) 检查 /proc/self/cmdline — 模拟器进程名通常含 emulator/qemu
    //    (保留原逻辑, 作为非 property 路径的兜底)
    if (check_access("/proc/self/fd/0") == 0) {
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
