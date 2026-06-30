#include "anti_hiding.h"
#include "../common/syscall.h"
#include <cstring>

// ─── Raw syscall I/O helpers ───────────────────────────────

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

static int64_t check_access(const char* path) {
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"(path), "i"(F_OK) : "x0", "x1", "x8");
    return ret;
}

// ═══════════════════════════════════════════════════════════
//  Shamiko Detection (root 级，无需内核态)
// ═══════════════════════════════════════════════════════════

bool detectShamiko() {
    // Check for Shamiko module directory
    if (check_access("/data/adb/modules/shamiko") == 0) return true;
    if (check_access("/data/adb/modules/Shamiko") == 0) return true;
    if (check_access("/data/adb/shamiko") == 0) return true;
    if (check_access("/data/adb/modules/shamiko/sepolicy.override") == 0) return true;

    // Check for Shamiko whitelist
    if (check_access("/data/adb/shamiko/whitelist") == 0) return true;
    if (check_access("/data/adb/modules/shamiko/whitelist") == 0) return true;

    // 扩充：Shamiko 新版路径
    if (check_access("/data/adb/shamiko/empty") == 0) return true;
    if (check_access("/data/adb/modules/shamiko/empty") == 0) return true;
    if (check_access("/data/adb/shamiko/version") == 0) return true;
    if (check_access("/data/adb/modules/shamiko/version") == 0) return true;
    // Shamiko 的 dex 文件
    if (check_access("/data/adb/modules/shamiko/system/framework/shamiko.jar") == 0) return true;

    // Check in memory
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "shamiko")) return true;
    }

    return false;
}

bool detectShamikoSELinuxTrick() {
    // Shamiko uses sepolicy.override to modify SELinux policy at runtime
    char buf[4096];
    if (read_file("/sys/fs/selinux/access", buf, sizeof(buf))) {
        // If we can read selinuxfs access, policy has been modified
        return true;
    }

    // Check for sepolicy.override file
    if (read_file("/data/adb/modules/shamiko/sepolicy.override", buf, sizeof(buf))) {
        return true;
    }

    // Check current SELinux context - Shamiko may restore it to expected value
    char ctx[256];
    if (read_file("/proc/self/attr/current", ctx, sizeof(ctx))) {
        // Shamiko sometimes sets context to kernel or init temporarily
        if (strstr(ctx, "kernel") || strstr(ctx, ":init:")) return true;
    }

    return false;
}

bool detectShamikoWhiteList() {
    // Shamiko whitelist mode: only hides root from whitelisted apps
    char buf[4096];
    if (read_file("/data/adb/shamiko/whitelist", buf, sizeof(buf))) {
        // Check if this package is in the whitelist
        if (strstr(buf, "com.apex.root")) return true;
        if (buf[0] != '\0') return true; // whitelist exists and is not empty
    }
    if (read_file("/data/adb/modules/shamiko/whitelist", buf, sizeof(buf))) {
        if (buf[0] != '\0') return true;
    }
    return false;
}

int shamikoFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectShamiko()) {
        append("[Shamiko] Module detected\n"); findings++;
    }
    if (detectShamikoSELinuxTrick()) {
        append("[Shamiko] SELinux override active\n"); findings++;
    }
    if (detectShamikoWhiteList()) {
        append("[Shamiko] Whitelist mode active\n"); findings++;
    }

    // Check for Shamiko's unique syscall hook pattern
    char maps[65536];
    if (read_file("/proc/self/maps", maps, sizeof(maps))) {
        // Shamiko creates anonymous executable mappings
        int anon_rx = 0;
        char* line = maps;
        char* end = maps + strlen(maps);
        while (line < end) {
            char* nl = line;
            while (nl < end && *nl != '\n') nl++;
            *nl = '\0';
            // Look for r-xp anonymous (no path at end)
            const char* perms_loc = line;
            while (*perms_loc && *perms_loc != ' ') perms_loc++;
            if (*perms_loc) perms_loc++;
            if (strncmp(perms_loc, "r-xp", 4) == 0) {
                // Find the path part (5th column)
                int spaces = 0;
                const char* p = perms_loc;
                while (*p) {
                    if (*p == ' ') spaces++;
                    if (spaces == 3) { p++; break; }
                    p++;
                }
                while (*p == ' ') p++;
                // If path is empty or [anon:, it's anonymous executable
                if (*p == '\0' || *p == '\n' || strncmp(p, "[anon:", 6) == 0) {
                    anon_rx++;
                }
            }
            line = nl + 1;
        }
        if (anon_rx > 5) {
            append("[Shamiko] Anon executable pages: ");
            char num[16];
            int ni = 0;
            if (anon_rx == 0) num[ni++] = '0';
            else {
                int t = anon_rx;
                char rev[16]; int ri = 0;
                while (t > 0) { rev[ri++] = '0' + (t % 10); t /= 10; }
                while (ri > 0) num[ni++] = rev[--ri];
            }
            num[ni] = '\0';
            append(num);
            append(" (suspicious)\n");
            findings++;
        }
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

// ═══════════════════════════════════════════════════════════
//  ZygiskNext Detection (root 级)
// ═══════════════════════════════════════════════════════════

bool detectZygiskNext() {
    if (check_access("/data/adb/modules/zygisknext") == 0) return true;
    if (check_access("/data/adb/modules/ZygiskNext") == 0) return true;
    if (check_access("/data/adb/zygisknext") == 0) return true;

    // 扩充：ZygiskNext 新版路径
    if (check_access("/data/adb/modules/zygisknext/bin") == 0) return true;
    if (check_access("/data/adb/modules/zygisknext/zygisknext") == 0) return true;
    if (check_access("/data/adb/zygisknext/zygisknext.so") == 0) return true;

    // ReZygisk (Rust 实现的 ZygiskNext 替代品)
    if (check_access("/data/adb/modules/rezygisk") == 0) return true;
    if (check_access("/data/adb/modules/ReZygisk") == 0) return true;
    if (check_access("/data/adb/rezygisk") == 0) return true;
    if (check_access("/data/adb/rezygisk/zygiskd") == 0) return true;

    // Check in memory
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "zygisknext")) return true;
        if (strstr(buf, "ZygiskNext")) return true;
        if (strstr(buf, "rezygisk")) return true;
        if (strstr(buf, "ReZygisk")) return true;
    }
    return false;
}

bool detectZygiskNextMemfd() {
    // ZygiskNext uses memfd_create for private memory-backed file descriptors
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        // /memfd: is the signature of memfd_create'd files
        if (strstr(buf, "/memfd:")) {
            // ZygiskNext-specific: named memfd
            if (strstr(buf, "/memfd:ZygiskNext")) return true;
            if (strstr(buf, "/memfd:dex")) return true;
            if (strstr(buf, "/memfd:jit")) return true;
            // ReZygisk 也用 memfd
            if (strstr(buf, "/memfd:rezygisk")) return true;

            // Count memfd entries for heuristics
            int count = 0;
            const char* p = buf;
            while ((p = strstr(p, "/memfd:")) != nullptr) {
                count++;
                p++;
            }
            // If we have many memfd entries, likely ZygiskNext
            return count > 2;
        }
    }
    return false;
}

bool detectZygiskNextIsolation() {
    // ZygiskNext provides stronger isolation by placing modules in separate processes
    // Check for unusual process relationships
    char buf[4096];

    // Check /proc/self/status for unusual PPID or NSpid
    if (read_file("/proc/self/status", buf, sizeof(buf))) {
        // NSpid shows the PID in different namespaces
        if (strstr(buf, "NSpid:")) {
            const char* ns = strstr(buf, "NSpid:");
            int spaces = 0;
            while (*ns) {
                if (*ns == ' ') spaces++;
                if (*ns == '\n') break;
                ns++;
            }
            // Multiple PIDs in NSpid = in a different PID namespace
            if (spaces > 2) return true; // more than "NSpid:\t<pid>"
        }
    }
    return false;
}

int zygiskNextFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectZygiskNext()) {
        append("[ZygiskNext] Module detected\n"); findings++;
    }
    if (detectZygiskNextMemfd()) {
        append("[ZygiskNext] Memfd isolation detected\n"); findings++;
    }
    if (detectZygiskNextIsolation()) {
        append("[ZygiskNext] PID namespace isolation detected\n"); findings++;
    }
    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

// ═══════════════════════════════════════════════════════════
//  Generic Anti-Hiding Probes (root 级)
// ----------------------------------------------------------------
//  原 detectSyscallTableHook() 已移除——它依赖 /proc/kallsyms
//  内核符号表扫描，属于 Ring0 检测。在新版中，syscall hook
//  的检测统一由 layer5_sidechannel.cpp 的侧信道时延分析覆盖。
// ═══════════════════════════════════════════════════════════

bool detectProcessHiding() {
    char buf[16384];

    // Compare /proc listing via raw getdents64 vs libc opendir
    // If libc shows different results, process hiding is active

    // Read /proc/1/status - if it fails or shows unexpected values, hiding is active
    if (!read_file("/proc/1/status", buf, sizeof(buf))) {
        // Cannot read init process - unusual
        return true;
    }

    // Check if we can find our own PID in /proc via raw syscall
    int64_t my_pid;
    asm volatile("mov x8, %1; svc #0; mov %0, x0" : "=r"(my_pid) : "i"(__NR_getpid) : "x0", "x8");

    // Try to read our own /proc/<pid>/status via direct path
    char self_path[64];
    int pi = 0;
    const char* pfx = "/proc/";
    for (int i = 0; pfx[i]; i++) self_path[pi++] = pfx[i];
    int64_t tmp = my_pid;
    char pid_str[16]; int psi = 0;
    if (tmp == 0) pid_str[psi++] = '0';
    else {
        char rev[16]; int ri = 0;
        while (tmp > 0) { rev[ri++] = '0' + (tmp % 10); tmp /= 10; }
        while (ri > 0) pid_str[psi++] = rev[--ri];
    }
    pid_str[psi] = '\0';
    for (int i = 0; pid_str[i]; i++) self_path[pi++] = pid_str[i];
    const char* sfx = "/status";
    for (int i = 0; sfx[i]; i++) self_path[pi++] = sfx[i];
    self_path[pi] = '\0';

    char our_buf[256];
    if (!read_file(self_path, our_buf, sizeof(our_buf))) {
        // Cannot read our own /proc entry - hidden!
        return true;
    }

    // 扩充：检测 Magisk Hide / Magisk DenyList
    if (check_access("/data/adb/magisk/denylist") == 0) return true;
    if (check_access("/data/adb/magisk/hide") == 0) return true;
    if (check_access("/data/adb/modules/.hide") == 0) return true;

    // 扩充：检测 HideMyApplist
    if (check_access("/data/adb/modules/hidemyapplist") == 0) return true;
    if (check_access("/data/adb/modules/HideMyApplist") == 0) return true;

    // 扩充：检测 StorageIsolation
    if (check_access("/data/adb/modules/storage_isolation") == 0) return true;
    if (check_access("/data/adb/modules/storageisolation") == 0) return true;

    return false;
}

bool detectMountNamespaceHiding() {
    char buf[16384];

    // Magisk Hide uses mount namespace to hide root
    // Read our mountinfo
    if (!read_file("/proc/self/mountinfo", buf, sizeof(buf))) return false;

    // Compare with /proc/1/mountinfo
    char init_buf[16384];
    if (!read_file("/proc/1/mountinfo", init_buf, sizeof(init_buf))) {
        // Cannot read init's mountinfo - likely hidden
        return true;
    }

    // If they differ significantly, namespace isolation is active
    // Check specific markers
    bool self_has_adb = strstr(buf, "/data/adb") != nullptr;
    bool init_has_adb = strstr(init_buf, "/data/adb") != nullptr;
    if (init_has_adb && !self_has_adb) return true; // /data/adb hidden from us

    bool self_has_sbin = strstr(buf, "/sbin") != nullptr;
    bool init_has_sbin = strstr(init_buf, "/sbin") != nullptr;
    if (init_has_sbin && !self_has_sbin) return true;

    return false;
}

// ═══════════════════════════════════════════════════════════
//  新增：检测额外的 Root 隐藏框架 (root 级)
// ═══════════════════════════════════════════════════════════

bool detectHideMyApplist() {
    // HideMyApplist (Rikka) 是常用的应用列表隐藏 Xposed 模块
    if (check_access("/data/adb/modules/hidemyapplist") == 0) return true;
    if (check_access("/data/adb/modules/HideMyApplist") == 0) return true;
    if (check_access("/data/adb/modules/hma") == 0) return true;
    if (check_access("/data/data/moe.haruishijima.hidemyapplist") == 0) return true;
    return false;
}

bool detectStorageIsolation() {
    // Storage Isolation (Rikka) - 强制沙箱
    if (check_access("/data/adb/modules/storage_isolation") == 0) return true;
    if (check_access("/data/adb/modules/storageisolation") == 0) return true;
    if (check_access("/data/data/moe.shizuku.privileged.api") == 0) return true;
    return false;
}

bool detectMagiskHideLegacy() {
    // 老式 MagiskHide (已废弃但仍可能存在)
    if (check_access("/data/adb/magisk/.core/magiskhide") == 0) return true;
    if (check_access("/data/adb/magisk/core/magiskhide") == 0) return true;
    if (check_access("/data/adb/magiskimg") == 0) return true; // 老式 image 模式
    return false;
}

// detectMagiskDenyList() is implemented in layer16_magisk_extensions.cpp
// to avoid duplicate-symbol linker errors. The function is declared in
// anti_hiding.h and layer16_magisk_extensions.h; only the layer16 .cpp
// provides the definition.

// 已移除：detectSyscallTableHook()
// 原函数依赖 /proc/kallsyms 内核符号表扫描，属 Ring0 检测。
// 如需检测 syscall hook，请使用 layer5_sidechannel.cpp 的时延分析。
