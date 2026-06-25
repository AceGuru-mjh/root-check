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
//  Shamiko Detection
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
//  ZygiskNext Detection
// ═══════════════════════════════════════════════════════════

bool detectZygiskNext() {
    if (check_access("/data/adb/modules/zygisknext") == 0) return true;
    if (check_access("/data/adb/modules/ZygiskNext") == 0) return true;
    if (check_access("/data/adb/zygisknext") == 0) return true;

    // Check in memory
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "zygisknext")) return true;
        if (strstr(buf, "ZygiskNext")) return true;
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
//  Generic Anti-Hiding Probes
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

bool detectSyscallTableHook() {
    char buf[16384];

    // If we can read /proc/kallsyms, check for syscall table hooking
    if (!read_file("/proc/kallsyms", buf, sizeof(buf))) {
        // kallsyms not readable - this is normal on production kernels
        return false;
    }

    // Check if sys_call_table address seems reasonable
    const char* sct = strstr(buf, " sys_call_table");
    if (!sct) {
        // sys_call_table symbol hidden - possible APatch/KernelSU hiding
        // But on some kernels this is normal
        return false;
    }

    // Read the address
    char addr_str[17];
    for (int i = 0; i < 16; i++) {
        char c = *(sct - 16 + i);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
            addr_str[i] = c;
        else
            addr_str[i] = '0';
    }
    addr_str[16] = '\0';

    // If address is 0x0000000000000000, kallsyms has been wiped
    bool all_zero = true;
    for (int i = 0; i < 16; i++) {
        if (addr_str[i] != '0') { all_zero = false; break; }
    }
    if (all_zero) return true;

    return false;
}
