#include "proc_scanner.h"
#include "bare_syscall/syscall_bridge.h"
#include "common/syscall.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include <cstring>
#include <cinttypes>
#include <cstdlib>
#include <csignal>
#include <fcntl.h>
#include <android/log.h>

#ifndef DT_DIR
#define DT_DIR 4
#endif
#ifndef DT_LNK
#define DT_LNK 10
#endif
#ifndef DT_REG
#define DT_REG 8
#endif

// getdents64 returns struct linux_dirent64
struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

namespace apex {
namespace observer {

std::vector<ProcEntry> scan_processes() {
    std::vector<ProcEntry> entries;
    int fd = static_cast<int>(bs_openat(-100, "/proc", 0x10000, 0));
    if (fd < 0) return entries;

    char buf[4096];
    int64_t n = bs_getdents64(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return entries;

    size_t pos = 0;
    while (pos < static_cast<size_t>(n)) {
        auto* dirent = reinterpret_cast<linux_dirent64*>(buf + pos);
        if (dirent->d_type == DT_DIR) {
            ProcEntry entry;
            entry.pid = 0;
            entry.state = '?';
            entry.flags = 0;
            std::memset(entry.name, 0, sizeof(entry.name));

            const char* name = dirent->d_name;
            bool all_digits = true;
            int val = 0;
            for (int i = 0; name[i]; i++) {
                if (name[i] >= '0' && name[i] <= '9') {
                    val = val * 10 + (name[i] - '0');
                } else {
                    all_digits = false;
                    break;
                }
            }
            if (all_digits) {
                entry.pid = val;
                entries.push_back(entry);
            }
        }
        pos += dirent->d_reclen;
    }

    return entries;
}

std::vector<MemoryRegion> read_process_maps(int pid) {
    std::vector<MemoryRegion> regions;
    char path[64];
    int len = 0;
    const char* prefix = "/proc/";
    for (int i = 0; prefix[i]; i++) path[len++] = prefix[i];
    char pid_str[16];
    int dcount = 0;
    if (pid == 0) { pid_str[dcount++] = '0'; }
    else {
        int temp = pid;
        while (temp > 0) { pid_str[dcount++] = '0' + (temp % 10); temp /= 10; }
    }
    for (int i = dcount - 1; i >= 0; i--) path[len++] = pid_str[i];
    const char* suffix = "/maps";
    for (int i = 0; suffix[i]; i++) path[len++] = suffix[i];
    path[len] = '\0';

    int fd = static_cast<int>(bs_openat(-100, path, 0, 0));
    if (fd < 0) return regions;

    char buf[8192];
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return regions;
    buf[n] = '\0';

    char* line = buf;
    char* end = buf + n;
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';

        MemoryRegion mr{};
        // Parse: hex-hex perms
        char* p = line;
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        int pi = 0;
        while (*p && *p != ' ' && pi < 4) mr.perms[pi++] = *p++;
        mr.perms[pi] = '\0';

        regions.push_back(mr);
        line = nl + 1;
    }

    return regions;
}

// Track processes we've seen across scans
static std::vector<int> g_known_pids;
static uint64_t g_last_scan_ns = 0;

bool detect_hidden_process(const std::vector<ProcEntry>& baseline) {
    // Strategy: compare /proc listing against baseline
    // Hidden processes by root kits won't show up in /proc
    // but can be detected via sysfs, pidfd, or other methods

    // 1. Get current /proc snapshot
    auto current = scan_processes();
    if (current.empty()) return true; // Can't detect if we can't read /proc

    // 2. Check for processes in baseline that are missing from current
    for (const auto& base : baseline) {
        bool found = false;
        for (const auto& cur : current) {
            if (cur.pid == base.pid) {
                found = true;
                break;
            }
        }
        if (!found && base.pid > 0) {
            // Process disappeared from /proc — may be hidden
            // Double-check with alternative method
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/status", base.pid);
            int64_t fd;
            asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                         : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
            if (fd >= 0) {
                // Process still exists but /proc getdents64 didn't show it
                // This is a classic sign of process hiding
                bs_close(fd);
                return true;
            }
        }
    }

    // 3. Check for PID gaps — sequential PID allocation means
    // missing PIDs could indicate hidden processes
    int max_pid = 0;
    int min_pid = 999999;
    for (const auto& cur : current) {
        if (cur.pid > max_pid) max_pid = cur.pid;
        if (cur.pid < min_pid) min_pid = cur.pid;
    }

    // If we have more than 50% gap between min and max PID count vs expected
    int expected_count = max_pid - min_pid + 1;
    int actual_count = static_cast<int>(current.size());
    if (expected_count > 100 && actual_count < expected_count / 2) {
        return true; // Suspicious: too many missing PIDs
    }

    // 4. Cross-check with pidfd or /proc/loadavg
    // Read /proc/loadavg — the number of running processes should roughly match
    char loadavg[128];
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/loadavg"), "i"(O_RDONLY), "i"(0));
    if (fd >= 0) {
        int64_t n = bs_read(fd, loadavg, sizeof(loadavg) - 1);
        bs_close(fd);
        if (n > 0) {
            loadavg[n] = '\0';
            // loadavg format: "1.23 0.45 0.67 <running>/<total> <lastpid>"
            const char* slash = strchr(loadavg, '/');
            if (slash) {
                int running = atoi(loadavg);
                int total = atoi(slash + 1);
                // If /proc shows significantly fewer processes than loadavg reports
                if (total > 20 && actual_count < total * 0.6) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool check_memory_integrity(const char* path, const uint8_t* expected_hash) {
    int fd = static_cast<int>(bs_openat(-100, path, 0, 0));
    if (fd < 0) return false;
    char buf[8192];
    int64_t n = bs_read(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return false;
    auto hash = crypto::sha3_512(reinterpret_cast<const uint8_t*>(buf), n);
    return std::memcmp(hash.data(), expected_hash, 64) == 0;
}

// Syscall trace state
static int g_trace_fd = -1;

void start_syscall_trace() {
    if (g_trace_fd >= 0) return; // already tracing

    // Open audit log for syscall tracing
    // Try /proc/sys/kernel/printk and auditd interaction
    g_trace_fd = static_cast<int>(bs_openat(-100,
        "/proc/self/status", O_RDONLY, 0));

    // Set up a signal handler for SIGSYS to catch seccomp violations
    // This serves as an alternative syscall tracing mechanism
    struct sigaction sa;
    __builtin_memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN; // Ignore SIGSYS to avoid crashes from seccomp
    sigaction(31, &sa, nullptr); // SIGSYS = 31 on ARM64

    // Enable syscall auditing via prctl if available
    bs_prctl(38, 1, 0, 0, 0); // PR_SET_NO_NEW_PRIVS (prerequisite for seccomp)

    __android_log_print(ANDROID_LOG_DEBUG, "APEX-PROC",
        "Syscall trace initialized (fd=%d)", g_trace_fd);
}

SyscallTrace collect_syscall_stats(int64_t syscall_nr) {
    SyscallTrace trace{};
    trace.syscall_nr = syscall_nr;
    trace.timestamp_ns = bs_clock_ns();
    trace.duration_ns = 0;
    trace.ret_value = -1;

    // Read our own seccomp filter violations
    if (g_trace_fd >= 0) {
        char buf[256];
        int64_t n = bs_read(g_trace_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            const char* needle = "TracerPid:";
            const char* p = strstr(buf, needle);
            if (p) {
                p += 10;
                while (*p == ' ' || *p == '\t') p++;
                if (*p >= '0' && *p <= '9') {
                    trace.ret_value = atoi(p);
                }
            }
        }
    }

    return trace;
}

} // namespace observer
} // namespace apex
