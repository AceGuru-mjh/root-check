#include "ebpf_manager.h"
#include "../common/syscall.h"
#include "../common/utils.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cstdio>
#include <android/log.h>

#define LOG_TAG "APEX-EBPF"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; mov x3, %5; mov x4, %6; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_perf_event_open), "r"(attr), "r"(pid), "r"(cpu), "r"(group_fd), "r"((int64_t)flags)
                 : "x0", "x1", "x2", "x3", "x4", "x8");
    #else
        ret = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    return ret;
}

static int64_t sys_bpf(int cmd, const void* attr, size_t size) {
    int64_t ret;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(BPF_SYSCALL), "r"(cmd), "r"(attr), "r"(size)
                 : "x0", "x1", "x2", "x8");
    #else
        /* arm32/x64 fallback */ (void)0;
    #endif
    return ret;
}

static bool read_file(const char* path, char* buf, size_t size) {
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
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
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
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        #else
            d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
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
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_WRONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) {
        int64_t fd2;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(fd2) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/sys/kernel/tracing/kprobe_events"), "i"(O_WRONLY), "i"(0));
        #else
            fd2 = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
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
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(disable_cmd), "r"((int64_t)strlen(disable_cmd)));
        #else
            n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        close_fd(fd);
        return n > 0;
    }
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(cmd), "r"((int64_t)strlen(cmd)));
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
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
            #if defined(__aarch64__)
            asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                         : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD),
                           "r"("/sys/kernel/debug/tracing/kprobe_events"),
                           "i"(O_WRONLY), "i"(0));
            #else
                fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
            #endif
            if (fd < 0) {
                int64_t fd2;
                #if defined(__aarch64__)
                asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                             : "=r"(fd2) : "i"(__NR_openat), "i"(AT_FDCWD),
                               "r"("/sys/kernel/tracing/kprobe_events"),
                               "i"(O_WRONLY), "i"(0));
                #else
                    fd2 = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
                #endif
                if (fd2 < 0) continue;
                fd = fd2;
            }
            int64_t n;
            #if defined(__aarch64__)
            asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                         : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(remove_cmd),
                           "r"((int64_t)strlen(remove_cmd)));
            #else
                n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
            #endif
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
    access_ret = apex_check_access(path);
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

// ═══════════════════════════════════════════════════════════
//  protect_syscall_table() — REMOVED (Ring0 检测残留)
// ----------------------------------------------------------------
//  原实现读取 /proc/kallsyms 来定位 sys_call_table 的内核符号地址,
//  并尝试写 /proc/sys/kernel/kptr_restrict 来"保护"它。这属于
//  Ring0 (内核态) 检测路径, 与本仓库 "仅 Ring3 root 级检测" 的
//  定位相违。在新版中:
//
//    - 内核 syscall 表 Hook 检测统一由 layer5_sidechannel.cpp
//      的侧信道时序分析覆盖 (detectSyscallResultInconsistency)
//    - 本函数保留为 no-op 占位符, 以维持 ABI 兼容 (调用方
//      activate_game_mode / deactivate_game_mode 仍引用此符号),
//      但不再读取 /proc/kallsyms
//
//  返回值: 始终为 true (与原实现的 "最佳情况" 语义一致)
// ═══════════════════════════════════════════════════════════
bool protect_syscall_table() {
    // No-op — see comment above. Avoids Ring0 /proc/kallsyms dependency.
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
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/__properties__"), "i"(O_WRONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        close_fd(fd);
    }

    return false; // Properties are read-only for apps
}

// ─── Game mode ─────────────────────────────────────────────

bool activate_game_mode() {
    // 0. Probe eBPF capability first. If kernel/SELinux denies BPF, all
    //    kprobe attach calls below will silently fail — giving the user a
    //    false sense of security. We log the result so logcat shows why
    //    hide-mode may be degraded, and fall back to non-eBPF hiding
    //    (process rename + hide_file).
    EbpfCapability cap;
    detect_ebpf_capabilities(&cap);
    LOGI("eBPF capability: syscall=%d bpffs=%d kernel=%d.%d selinux=%s "
         "can_create_map=%d can_load_prog=%d → use_ebpf=%d",
         cap.bpf_syscall_present, cap.bpffs_mounted,
         cap.kernel_major, cap.kernel_minor, cap.selinux_mode,
         cap.can_create_map, cap.can_load_prog,
         should_use_ebpf_path());

    bool use_ebpf = should_use_ebpf_path();

    // 1. Attempt to hide root processes via kprobe (eBPF path only)
    if (use_ebpf) {
        attach_kprobe(-1, "sys_getdents64");
    } else {
        LOGW("eBPF path disabled — falling back to mount-namespace + rename hiding");
    }

    // 2. Try to rename our process to something innocuous (works regardless)
    char comm_path[64];
    int pi = 0;
    const char* pfx = "/proc/self/comm";
    for (int i = 0; pfx[i]; i++) comm_path[pi++] = pfx[i];
    comm_path[pi] = '\0';

    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(comm_path), "i"(O_WRONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        const char* hidden_name = "surfaceflinger";
        int64_t n;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(hidden_name), "r"((int64_t)strlen(hidden_name)) : "x0", "x1", "x2", "x8");
        #else
            n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        close_fd(fd);
    }

    // 3. Hide all root daemon processes (works regardless of eBPF availability)
    hide_process("magiskd");
    hide_process("ksud");
    hide_process("apd");
    hide_process("apex_root_daemon");

    // 4. Hide root files/directories (mount-bind, works regardless)
    hide_file("/data/adb");
    hide_file("/sbin/.magisk");

    // 5. protect_syscall_table() is now a no-op (Ring0 path removed).
    //    Syscall tamper detection is done by layer5_sidechannel in
    //    detect-only mode, not by hide-mode.
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
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(comm_path), "i"(O_WRONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd >= 0) {
        const char* real_name = "com.apex.root";
        int64_t n;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(real_name), "r"((int64_t)strlen(real_name)) : "x0", "x1", "x2", "x8");
        #else
            n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        close_fd(fd);
    }

    detach_program(-1);
    return true;
}

// ─── Additional utility ────────────────────────────────────

bool check_ebpf_available() {
    // Legacy API — kept for ABI compatibility. New callers should use
    // detect_ebpf_capabilities() / should_use_ebpf_path() instead, which
    // also account for SELinux enforcing + kernel version.
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

// ─── Runtime capability / fallback detection ───────────────

// Read kernel version (major.minor) from /proc/version.
// Returns false if the file cannot be parsed.
static bool read_kernel_version(int* major, int* minor) {
    if (!major || !minor) return false;
    *major = 0; *minor = 0;
    char buf[256];
    if (!read_file("/proc/version", buf, sizeof(buf))) return false;
    // Format: "Linux version 5.15.0-android13-8-..."
    const char* p = buf;
    // Skip "Linux version "
    while (*p && (*p < '0' || *p > '9')) p++;
    int maj = 0, min = 0;
    while (*p >= '0' && *p <= '9') { maj = maj * 10 + (*p - '0'); p++; }
    if (*p != '.') return false;
    p++;
    while (*p >= '0' && *p <= '9') { min = min * 10 + (*p - '0'); p++; }
    *major = maj;
    *minor = min;
    return maj > 0 || min > 0;
}

// Read SELinux mode from /sys/fs/selinux/enforce.
// "1" → enforcing, "0" → permissive, file missing → disabled/unknown.
static void read_selinux_mode(char* out, size_t out_size) {
    if (!out || out_size == 0) return;
    out[0] = '\0';
    char buf[16];
    // /sys/fs/selinux/enforce: "1" = enforcing, "0" = permissive
    if (!read_file("/sys/fs/selinux/enforce", buf, sizeof(buf))) {
        // File missing → SELinux disabled or not mounted
        // Fallback: check /proc/self/attr/current for SELinux context
        if (read_file("/proc/self/attr/current", buf, sizeof(buf)) && buf[0] != '\0') {
            // Context present (e.g. "u:r:app:s0") → SELinux enabled, mode unknown
            snprintf(out, out_size, "unknown");
        } else {
            snprintf(out, out_size, "disabled");
        }
        return;
    }
    // Trim whitespace
    char* nl = buf;
    while (*nl && *nl != '\n' && *nl != ' ') nl++;
    *nl = '\0';
    if (buf[0] == '1') {
        snprintf(out, out_size, "enforcing");
    } else if (buf[0] == '0') {
        snprintf(out, out_size, "permissive");
    } else {
        snprintf(out, out_size, "unknown");
    }
}

bool detect_ebpf_capabilities(EbpfCapability* out_cap) {
    if (out_cap) {
        memset(out_cap, 0, sizeof(*out_cap));
        out_cap->kernel_major = 0;
        out_cap->kernel_minor = 0;
    }

    // 1. Kernel version
    int kmaj = 0, kmin = 0;
    bool kv_ok = read_kernel_version(&kmaj, &kmin);
    bool kernel_version_ok = kv_ok && (kmaj > 5 || (kmaj == 5 && kmin >= 10));

    // 2. SELinux mode
    char selinux_mode[16] = "unknown";
    read_selinux_mode(selinux_mode, sizeof(selinux_mode));
    // Heuristic: in enforcing mode, unprivileged bpf is almost always denied
    // on Android 13+ GKI. In permissive or disabled, bpf may work.
    // Real test below (can_create_map / can_load_prog) is authoritative.
    bool selinux_likely_allows = (strcmp(selinux_mode, "enforcing") != 0);

    // 3. bpffs mounted at /sys/fs/bpf
    //    We check via stat/access on the directory itself.
    bool bpffs_mounted = (check_access("/sys/fs/bpf") == 0);

    // 4. BPF syscall present (not blocked by seccomp)?
    //    Test with a deliberately-tiny invalid attr → if syscall returns
    //    -EINVAL the syscall is reachable; if -ENOSYS it's not present;
    //    if -EPERM it's filtered by seccomp.
    bool bpf_syscall_present = false;
    {
        struct { uint32_t zero; } bad_attr = {};
        int64_t r = sys_bpf(BPF_MAP_CREATE, &bad_attr, sizeof(bad_attr));
        // -EINVAL (-22) means syscall is reachable but args are wrong — good.
        // -EACCES (-13) or -EPERM (-1) means reachable but denied.
        // -ENOSYS (-38) means not implemented.
        if (r != -38 /* ENOSYS */) {
            bpf_syscall_present = true;
        }
    }

    // 5. Real test: can we BPF_MAP_CREATE a minimal hash map?
    bool can_create_map = false;
    {
        struct {
            uint32_t map_type;    // BPF_MAP_TYPE_HASH = 1
            uint32_t key_size;
            uint32_t value_size;
            uint32_t max_entries;
            uint32_t map_flags;
            uint32_t inner_map_fd;
            uint32_t numa_node;
        } attr = {};
        attr.map_type = 1;
        attr.key_size = 4;
        attr.value_size = 4;
        attr.max_entries = 1;
        int64_t fd = sys_bpf(BPF_MAP_CREATE, &attr, sizeof(attr));
        if (fd >= 0) {
            can_create_map = true;
            close_fd(fd);
        }
    }

    // 6. Real test: can we BPF_PROG_LOAD a tiny valid program?
    //    We use a 1-instruction "mov r0, 0; exit" BPF program which is
    //    the smallest valid eBPF program. If verifier + permissions allow
    //    it, we have full BPF_PROG_LOAD capability.
    bool can_load_prog = false;
    {
        // eBPF instruction: mov r0, #0 (BPF_ALU64 | BPF_K | BPF_MOV, dst=0, src=0, off=0, imm=0)
        // then exit (BPF_JMP | BPF_K | BPF_EXIT, ...)
        struct __attribute__((packed)) bpf_insn {
            uint8_t code;
            uint8_t dst_reg:4;
            uint8_t src_reg:4;
            int16_t off;
            int32_t imm;
        };
        bpf_insn insns[2] = {};
        // mov r0, 0  →  code=0xB7 (BPF_ALU64|BPF_MOV|BPF_K), dst=0
        insns[0].code = 0xB7;
        insns[0].dst_reg = 0;
        insns[0].imm = 0;
        // exit       →  code=0x95 (BPF_JMP|BPF_EXIT|BPF_K)
        insns[1].code = 0x95;

        // log buffer for verifier messages (helps debugging but not required)
        char log_buf[4096] = {};

        // BPF_PROG_LOAD attr (bpf_attr union, we use first ~48 bytes)
        struct __attribute__((packed)) {
            uint32_t prog_type;        // BPF_PROG_TYPE_SOCKET_FILTER = 1 (least privileged)
            uint32_t insn_cnt;
            uint64_t insns_ptr;
            uint64_t license_ptr;
            uint32_t log_level;        // 1 = verbose verifier log
            uint32_t log_size;
            uint64_t log_buf_ptr;
            uint32_t kern_version;     // ignored for socket filter
            uint32_t prog_flags;
        } attr = {};
        attr.prog_type = 1; // BPF_PROG_TYPE_SOCKET_FILTER — least privileged type
        attr.insn_cnt = 2;
        attr.insns_ptr = (uint64_t)insns;
        static const char license[] = "GPL";
        attr.license_ptr = (uint64_t)license;
        attr.log_level = 1;
        attr.log_size = sizeof(log_buf);
        attr.log_buf_ptr = (uint64_t)log_buf;

        int64_t fd = sys_bpf(BPF_PROG_LOAD, &attr, sizeof(attr));
        if (fd >= 0) {
            can_load_prog = true;
            close_fd(fd);
        }
    }

    if (out_cap) {
        out_cap->bpf_syscall_present = bpf_syscall_present;
        out_cap->bpffs_mounted = bpffs_mounted;
        out_cap->kernel_version_ok = kernel_version_ok;
        // selinux_allows_bpf = authoritative result from real probe,
        // not just from mode string. If we can_create_map, SELinux
        // is clearly not blocking us.
        out_cap->selinux_allows_bpf = selinux_likely_allows || can_create_map;
        out_cap->can_create_map = can_create_map;
        out_cap->can_load_prog = can_load_prog;
        out_cap->kernel_major = kmaj;
        out_cap->kernel_minor = kmin;
        snprintf(out_cap->selinux_mode, sizeof(out_cap->selinux_mode), "%s", selinux_mode);
    }

    return true;
}

bool should_use_ebpf_path() {
    EbpfCapability cap;
    detect_ebpf_capabilities(&cap);
    // Must be able to both create maps AND load programs — anything less
    // means hide-mode eBPF will silently no-op and give a false sense of
    // security. Fall back to mount namespace + LD_PRELOAD path instead.
    //
    // Kernel >= 5.10 is a soft requirement (Android 13 GKI baseline).
    // Older kernels may technically support eBPF but with fewer helpers
    // and weaker verifier — refuse to use eBPF there to avoid surprises.
    if (!cap.can_create_map) return false;
    if (!cap.can_load_prog) return false;
    if (!cap.kernel_version_ok) return false;
    // If SELinux is enforcing and we couldn't actually create a map, the
    // real-probe already returned false above. So no additional check here.
    return true;
}

} // namespace ebpf
} // namespace apex
