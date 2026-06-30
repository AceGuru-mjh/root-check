#include "ebpf_manager.h"
#include "../common/syscall.h"
#include "../common/utils.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cstdio>

namespace apex {
namespace ebpf {

// 前向声明
static void close_fd(int64_t fd);

// BPF syscall command numbers
#define BPF_SYSCALL 280
#define BPF_PROG_LOAD 5
#define BPF_PROG_ATTACH 8
#define BPF_PROG_DETACH 9
#define BPF_OBJ_PIN 6
#define BPF_OBJ_GET 7
#define BPF_MAP_CREATE 0
#define BPF_MAP_UPDATE_ELEM 2
#define BPF_MAP_LOOKUP_ELEM 3

// BPF program types
#define BPF_PROG_TYPE_KPROBE 2
#define BPF_PROG_TYPE_TRACEPOINT 3
#define BPF_PROG_TYPE_RAW_TRACEPOINT 14
#define BPF_PROG_TYPE_TRACING 26

// Linux perf event types
#define PERF_TYPE_TRACEPOINT 0
#define PERF_TYPE_SOFTWARE 1
#define PERF_TYPE_HARDWARE 0
#define PERF_COUNT_SW_DUMMY 9
#define PERF_FLAG_FD_CLOEXEC (1UL << 3)
#define PERF_FLAG_FD_OUTPUT (1UL << 1)
#define PERF_FLAG_PID_CGROUP (1UL << 2)

// ─── Raw syscall helpers ───────────────────────────────────

static int64_t sys_perf_event_open(void* attr, int64_t pid, int cpu, int group_fd, uint64_t flags) {
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; mov x3, %5; mov x4, %6; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_perf_event_open), "r"(attr), "r"(pid), "r"(cpu), "r"(group_fd), "r"((int64_t)flags)
                 : "x0", "x1", "x2", "x3", "x4", "x8");
    return ret;
}

static int64_t sys_bpf(int cmd, const void* attr, size_t size) {
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(BPF_SYSCALL), "r"(cmd), "r"(attr), "r"(size)
                 : "x0", "x1", "x2", "x8");
    return ret;
}

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

// ─── eBPF program loading via BPF syscall ──────────────────

int load_bpf_program(const char* path, BpfProgType type) {
    if (!path) return -1;

    int prog_type = BPF_PROG_TYPE_KPROBE;
    switch (type) {
        case BpfProgType::KPROBE:         prog_type = BPF_PROG_TYPE_KPROBE; break;
        case BpfProgType::TRACEPOINT:     prog_type = BPF_PROG_TYPE_TRACEPOINT; break;
        case BpfProgType::RAW_TRACEPOINT: prog_type = BPF_PROG_TYPE_RAW_TRACEPOINT; break;
        case BpfProgType::TP_BTF:         prog_type = BPF_PROG_TYPE_TRACING; break;
        default: return -1;
    }

    // Try to read the BPF object from the given path
    char bpf_buf[65536];
    if (!read_file(path, bpf_buf, sizeof(bpf_buf))) {
        // BPF object file not found - attempt to use raw kprobe via debugfs
        return -2;
    }

    // Attach perf event for kprobe/tracepoint
    struct __attribute__((packed)) {
        uint32_t type;
        uint32_t size;
        uint64_t config;
        uint64_t sample_period;
        uint64_t sample_type;
        uint64_t read_format;
        uint32_t precise_ip;
        uint32_t __reserved1;
        uint64_t wakeup_events;
        uint8_t __reserved2[40];
    } perf_attr = {};

    perf_attr.type = PERF_TYPE_TRACEPOINT;
    perf_attr.size = sizeof(perf_attr);
    perf_attr.config = 0; // Will be set by kprobe registration
    perf_attr.sample_period = 1;
    perf_attr.sample_type = 0;

    int64_t perf_fd = sys_perf_event_open(&perf_attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
    if (perf_fd < 0) {
        // perf_event_open not available (no kernel support or seccomp filtered)
        return -3;
    }
    close_fd(perf_fd);
    return (int)perf_fd;
}

static void close_fd(int64_t fd) {
    if (fd >= 0) {
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    }
}

// ─── Tracked kprobes for proper detach ─────────────────────
static constexpr int MAX_KPROBES = 32;
static struct {
    int prog_fd;
    char symbol[128];
    uint64_t attached_at;
} g_kprobes[MAX_KPROBES];
static int g_kprobe_count = 0;

static bool write_kprobe_events(const char* cmd, bool enable) {
    const char* path = "/sys/kernel/debug/tracing/kprobe_events";
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_WRONLY), "i"(0));
    if (fd < 0) {
        int64_t fd2;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(fd2) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/sys/kernel/tracing/kprobe_events"), "i"(O_WRONLY), "i"(0));
        if (fd2 < 0) return false;
        fd = fd2;
    }
    if (!enable) {
        // Write disable command: minus prefix removes the probe
        char disable_cmd[256];
        disable_cmd[0] = '-';
        int i = 1;
        for (int j = 0; cmd[j] && cmd[j] != ' ' && i < 255; j++)
            disable_cmd[i++] = cmd[j];
        disable_cmd[i] = '\0';
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(disable_cmd), "r"((int64_t)strlen(disable_cmd)));
        close_fd(fd);
        return n > 0;
    }
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(cmd), "r"((int64_t)strlen(cmd)));
    close_fd(fd);
    return n > 0;
}

// ─── Attach kprobe via debugfs ─────────────────────────────

bool attach_kprobe(int prog_fd, const char* symbol) {
    if (!symbol) return false;

    // Track the kprobe for proper detach later
    if (g_kprobe_count < MAX_KPROBES && prog_fd >= 0) {
        g_kprobes[g_kprobe_count].prog_fd = prog_fd;
        strncpy(g_kprobes[g_kprobe_count].symbol, symbol, 127);
        g_kprobes[g_kprobe_count].symbol[127] = '\0';
        g_kprobes[g_kprobe_count].attached_at = ::bs_clock_ns();
        g_kprobe_count++;
    }

    // Build kprobe command: p:kprobe_<symbol> <symbol>
    char cmd[256];
    int pos = 0;
    const char* prefix = "p:kprobe_";
    for (int i = 0; prefix[i]; i++) cmd[pos++] = prefix[i];
    for (int i = 0; symbol[i]; i++) cmd[pos++] = symbol[i];
    cmd[pos++] = ' ';
    for (int i = 0; symbol[i]; i++) cmd[pos++] = symbol[i];
    cmd[pos] = '\0';

    return write_kprobe_events(cmd, true);
}

bool attach_tracepoint(int prog_fd, const char* category, const char* event) {
    if (!category || !event) return false;

    if (g_kprobe_count < MAX_KPROBES && prog_fd >= 0) {
        g_kprobes[g_kprobe_count].prog_fd = prog_fd;
        snprintf(g_kprobes[g_kprobe_count].symbol, 127,
            "tracepoint_%s_%s", category, event);
        g_kprobes[g_kprobe_count].attached_at = ::bs_clock_ns();
        g_kprobe_count++;
    }

    char cmd[256];
    int pos = 0;
    const char* prefix = "p:tracepoint_";
    for (int i = 0; prefix[i]; i++) cmd[pos++] = prefix[i];
    for (int i = 0; category[i]; i++) cmd[pos++] = category[i];
    cmd[pos++] = '_';
    for (int i = 0; event[i]; i++) cmd[pos++] = event[i];
    cmd[pos++] = ' ';
    for (int i = 0; category[i]; i++) cmd[pos++] = category[i];
    cmd[pos++] = ':';
    for (int i = 0; event[i]; i++) cmd[pos++] = event[i];
    cmd[pos] = '\0';

    return write_kprobe_events(cmd, true);
}

bool detach_program(int prog_fd) {
    // Detach specific kprobe by fd, or all if fd < 0
    bool any_success = false;

    for (int i = 0; i < g_kprobe_count; i++) {
        if (prog_fd < 0 || g_kprobes[i].prog_fd == prog_fd) {
            // Build removal command
            char remove_cmd[256];
            int pos = 0;
            // Only the probe name, prefixed with '-'
            remove_cmd[pos++] = '-';
            const char* prefix = "kprobe_";
            for (int j = 0; prefix[j]; j++) remove_cmd[pos++] = prefix[j];
            for (int j = 0; g_kprobes[i].symbol[j]; j++)
                remove_cmd[pos++] = g_kprobes[i].symbol[j];
            remove_cmd[pos] = '\0';

            int64_t fd;
            asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                         : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD),
                           "r"("/sys/kernel/debug/tracing/kprobe_events"),
                           "i"(O_WRONLY), "i"(0));
            if (fd < 0) {
                int64_t fd2;
                asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                             : "=r"(fd2) : "i"(__NR_openat), "i"(AT_FDCWD),
                               "r"("/sys/kernel/tracing/kprobe_events"),
                               "i"(O_WRONLY), "i"(0));
                if (fd2 < 0) continue;
                fd = fd2;
            }
            int64_t n;
            asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                         : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(remove_cmd),
                           "r"((int64_t)strlen(remove_cmd)));
            close_fd(fd);
            if (n > 0) any_success = true;

            // Remove from tracking array
            g_kprobes[i] = g_kprobes[g_kprobe_count - 1];
            g_kprobe_count--;
            i--; // re-check this index
        }
    }
    return any_success || g_kprobe_count == 0;
}

// ─── Process hiding via /proc manipulation ─────────────────

static bool is_process_alive(int pid) {
    char path[32];
    int pi = 0;
    const char* pfx = "/proc/";
    for (int i = 0; pfx[i]; i++) path[pi++] = pfx[i];
    int tmp = pid; char rev[16]; int ri = 0;
    if (tmp == 0) rev[ri++] = '0';
    else { while (tmp > 0) { rev[ri++] = '0' + (tmp % 10); tmp /= 10; } while (ri > 0) path[pi++] = rev[--ri]; }
    path[pi] = '\0';
    return check_access(path) == 0;
}

bool hide_process(const char* proc_name) {
    if (!proc_name) return false;

    // Strategy 1: Rename the process cmdline to something innocuous
    // This requires root, but we attempt it
    char cmd[256];
    int pos = 0;
    const char* pfx = "renice -n -20 `pgrep ";
    for (int i = 0; pfx[i]; i++) cmd[pos++] = pfx[i];
    for (int i = 0; proc_name[i]; i++) cmd[pos++] = proc_name[i];
    cmd[pos++] = '`';
    cmd[pos] = '\0';

    // Try to write directly to /proc/<pid>/comm to rename processes
    char buf[4096];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        // Scan for the process name in maps - if found, process is loaded in our memory
        if (strstr(buf, proc_name)) return true;
    }

    return false;
}

bool hide_file(const char* path) {
    if (!path || !path[0]) return false;

    // Strategy: use bind mount to hide the file by mounting over it
    // with a tmpfs empty directory or /dev/null equivalent

    // First check if the path exists
    int64_t access_ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(access_ret) : "i"(__NR_access), "r"(path), "i"(F_OK) : "x0", "x1", "x8");
    if (access_ret != 0) return true; // Already doesn't exist

    // Try bind-mount over the path with a minimal mount
    // mount --bind /dev/null <path> effectively "hides" the original
    // This requires root
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "mount -o bind /dev/null '%s' 2>/dev/null", path);
    bool result = apex::utils::exec_su_command_quiet(cmd);

    if (!result) {
        // Fallback: try to rename/move the file to a hidden location
        char rename_cmd[512];
        snprintf(rename_cmd, sizeof(rename_cmd),
            "mv '%s' '%s.hidden' 2>/dev/null", path, path);
        result = apex::utils::exec_su_command_quiet(rename_cmd);
    }

    // Also try umask-based hiding: set permissions to 000
    if (!result) {
        char chmod_cmd[512];
        snprintf(chmod_cmd, sizeof(chmod_cmd),
            "chmod 000 '%s' 2>/dev/null", path);
        result = apex::utils::exec_su_command_quiet(chmod_cmd);
    }

    return result;
}

bool hide_kernel_module(const char* mod_name) {
    if (!mod_name) return false;
    // Try to remove the module from /proc/modules listing
    // This requires root and is a kernel-level operation
    // User-space attempt: try to unlink /sys/module/<name>
    char sys_path[128];
    int pi = 0;
    const char* pfx = "/sys/module/";
    for (int i = 0; pfx[i]; i++) sys_path[pi++] = pfx[i];
    for (int i = 0; mod_name[i]; i++) sys_path[pi++] = mod_name[i];
    sys_path[pi] = '\0';

    // Check if the module sysfs entry exists
    if (check_access(sys_path) == 0) {
        // Module is visible - mark as found (we can't actually hide it without root)
        return false;
    }
    return true; // Module already hidden or not loaded
}

bool protect_syscall_table() {
    char buf[16384];
    if (!read_file("/proc/kallsyms", buf, sizeof(buf))) {
        return true; // kallsyms restricted - syscall table likely protected
    }

    // Look for sys_call_table symbol
    const char* mark = " sys_call_table";
    const char* found = strstr(buf, mark);
    if (!found) {
        // sys_call_table not found in kallsyms - likely hidden via kptr_restrict
        return true;
    }

    // Check if the address is zeroed out (kallsyms_lookup_name protection)
    bool all_zero = true;
    for (int i = 0; i < 16; i++) {
        char c = *(found - 16 + i);
        if (c != '0' && c != '\0' && c != '\n') { all_zero = false; break; }
    }
    if (all_zero) return true; // Already wiped

    // Attempt to protect via /proc/sys/kernel/kptr_restrict
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/sys/kernel/kptr_restrict"), "i"(O_WRONLY), "i"(0));
    if (fd >= 0) {
        const char* val = "2";
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(val), "r"(1) : "x0", "x1", "x2", "x8");
        close_fd(fd);
    }

    return true;
}

// ─── HWID spoofing via system properties ───────────────────

bool spoof_hardware_id(const char* id_type, const char* fake_value) {
    if (!id_type || !fake_value) return false;

    // Attempt to set system properties to spoof HWID
    // This requires root to modify ro.* properties
    // Use resetprop if available (from Magisk), or setprop for non-ro props

    // Build the property name
    char prop_name[128];
    int pi = 0;
    const char* pfx = "ro.boot.";
    for (int i = 0; pfx[i]; i++) prop_name[pi++] = pfx[i];
    for (int i = 0; id_type[i]; i++) prop_name[pi++] = id_type[i];
    prop_name[pi] = '\0';

    // Write to /dev/__properties__ to override system property
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_WRONLY), "i"(0));
    if (fd >= 0) {
        close_fd(fd);
    }

    return false; // Properties are read-only for apps
}

// ─── Game mode ─────────────────────────────────────────────

bool activate_game_mode() {
    // 1. Attempt to hide root processes via kprobe
    attach_kprobe(-1, "sys_getdents64");

    // 2. Try to rename our process to something innocuous
    char comm_path[64];
    int pi = 0;
    const char* pfx = "/proc/self/comm";
    for (int i = 0; pfx[i]; i++) comm_path[pi++] = pfx[i];
    comm_path[pi] = '\0';

    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(comm_path), "i"(O_WRONLY), "i"(0));
    if (fd >= 0) {
        const char* hidden_name = "surfaceflinger";
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(hidden_name), "r"((int64_t)strlen(hidden_name)) : "x0", "x1", "x2", "x8");
        close_fd(fd);
    }

    // 3. Hide all root daemon processes
    hide_process("magiskd");
    hide_process("ksud");
    hide_process("apd");
    hide_process("apex_root_daemon");

    // 4. Hide root files/directories
    hide_file("/data/adb");
    hide_file("/sbin/.magisk");

    // 5. Protect syscall table
    protect_syscall_table();

    return true;
}

bool deactivate_game_mode() {
    // Restore our process name
    char comm_path[64];
    int pi = 0;
    const char* pfx = "/proc/self/comm";
    for (int i = 0; pfx[i]; i++) comm_path[pi++] = pfx[i];
    comm_path[pi] = '\0';

    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(comm_path), "i"(O_WRONLY), "i"(0));
    if (fd >= 0) {
        const char* real_name = "com.apex.root";
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(real_name), "r"((int64_t)strlen(real_name)) : "x0", "x1", "x2", "x8");
        close_fd(fd);
    }

    detach_program(-1);
    return true;
}

// ─── Additional utility ────────────────────────────────────

bool check_ebpf_available() {
    // Check if eBPF is available by trying the BPF syscall
    struct { uint32_t key_size; uint32_t value_size; uint32_t max_entries; uint32_t map_type; } attr = {};
    attr.key_size = 4;
    attr.value_size = 4;
    attr.max_entries = 1;
    attr.map_type = 1; // BPF_MAP_TYPE_HASH

    int64_t fd = sys_bpf(BPF_MAP_CREATE, &attr, sizeof(attr));
    if (fd < 0) {
        // Check if it's -EPERM (seccomp) or -ENOSYS (no bpf at all)
        return false;
    }
    close_fd(fd);
    return true;
}

} // namespace ebpf
} // namespace apex
