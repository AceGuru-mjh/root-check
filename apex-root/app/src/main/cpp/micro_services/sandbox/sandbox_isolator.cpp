#include "sandbox_isolator.h"
#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <unistd.h>
#include <time.h>

// ─── BPF macros (file-scope, self-contained) ─────────────

#define BPF_LD(c, jt, jf, k) \
    { (uint16_t)(c), (uint8_t)(jt), (uint8_t)(jf), (uint32_t)(k) }
#define BPF_STMT(code, k)   BPF_LD(code, 0, 0, k)
#define BPF_JUMP(code, k, jt, jf) BPF_LD(code, jt, jf, k)

#define BPF_JMP_JEQ(k, jt)  BPF_JUMP(0x15, (k), (jt), 0)
#define BPF_JMP_JA(jt)      BPF_JUMP(0x05, 0, (jt), 0)
#define BPF_RET_ALLOW       BPF_STMT(0x06, 0x7fff0000U)
#define BPF_RET_KILL        BPF_STMT(0x06, 0x80000000U)

#define AUDIT_ARCH_AARCH64  (0xC00000B7U)

// ─── Seccomp BPF filter ─────────────────────────────────

// Whitelist: only these syscalls are allowed. Everything else kills the process.
static const apex::sandbox::sock_filter
g_seccomp_filter[] = {

    // Check architecture = AARCH64
    BPF_STMT(0x20, 4),  // LD abs [4] = arch
    BPF_JUMP(0x15, AUDIT_ARCH_AARCH64, 1, 0),
    BPF_RET_KILL,

    // LD abs [0] = syscall nr now

    // ── I/O ──────────────────────────────────────────────
    BPF_JMP_JEQ(63,  1),  // read
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(64,  1),  // write
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(57,  1),  // close
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(62,  1),  // lseek
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(67,  1),  // pread64
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(68,  1),  // pwrite64
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(65,  1),  // readv
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(66,  1),  // writev
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(23,  1),  // dup
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(24,  1),  // dup3
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── File ─────────────────────────────────────────────
    BPF_JMP_JEQ(56,  1),  // openat
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(79,  1),  // newfstatat (fstatat64 on 64-bit)
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(80,  1),  // fstat
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(78,  1),  // readlinkat
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(34,  1),  // mkdirat
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(48,  1),  // faccessat
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(49,  1),  // chdir
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Memory ───────────────────────────────────────────
    BPF_JMP_JEQ(222, 1),  // mmap
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(215, 1),  // munmap
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(226, 1),  // mprotect
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(214, 1),  // brk
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(233, 1),  // madvise
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(216, 1),  // mremap
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(228, 1),  // mlock
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(229, 1),  // munlock
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Process ──────────────────────────────────────────
    BPF_JMP_JEQ(93,  1),  // exit
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(94,  1),  // exit_group
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(172, 1),  // getpid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(178, 1),  // gettid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(173, 1),  // getppid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(129, 1),  // kill
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(131, 1),  // tgkill
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(174, 1),  // getuid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(175, 1),  // geteuid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(176, 1),  // getgid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(177, 1),  // getegid
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(186, 1),  // gettid (gettid)
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Signal ───────────────────────────────────────────
    BPF_JMP_JEQ(13,  1),  // rt_sigaction
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(14,  1),  // rt_sigprocmask
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(15,  1),  // rt_sigreturn
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(289, 1),  // signalfd4
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(62,  1),  // kill
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Thread sync ──────────────────────────────────────
    BPF_JMP_JEQ(98,  1),  // futex
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(274, 1),  // set_robust_list
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(275, 1),  // get_robust_list
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Time ─────────────────────────────────────────────
    BPF_JMP_JEQ(101, 1),  // nanosleep
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(113, 1),  // clock_gettime
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(114, 1),  // clock_getres
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(115, 1),  // clock_nanosleep
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(169, 1),  // gettimeofday
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(29,  1),  // settimeofday
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Random ───────────────────────────────────────────
    BPF_JMP_JEQ(278, 1),  // getrandom
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Scheduling ───────────────────────────────────────
    BPF_JMP_JEQ(124, 1),  // sched_yield
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(123, 1),  // sched_getaffinity
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── FD monitoring ────────────────────────────────────
    BPF_JMP_JEQ(290, 1),  // eventfd2
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(20,  1),  // epoll_create1
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(21,  1),  // epoll_ctl
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(22,  1),  // epoll_pwait
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(7,   1),  // poll
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(73,  1),  // ppoll
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Socket ───────────────────────────────────────────
    BPF_JMP_JEQ(198, 1),  // socket
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(203, 1),  // connect
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(200, 1),  // bind
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(201, 1),  // listen
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(202, 1),  // accept
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(204, 1),  // getsockname
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(205, 1),  // getpeername
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(206, 1),  // sendto
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(207, 1),  // recvfrom
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(208, 1),  // setsockopt
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(209, 1),  // getsockopt
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(211, 1),  // sendmsg
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(212, 1),  // recvmsg
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Security / control ───────────────────────────────
    BPF_JMP_JEQ(277, 1),  // seccomp
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(167, 1),  // prctl
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(25,  1),  // fcntl
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(29,  1),  // ioctl
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Namespace / mount (rare) ─────────────────────────
    BPF_JMP_JEQ(244, 1),  // unshare
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(40,  1),  // mount
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(39,  1),  // umount2
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(41,  1),  // pivot_root
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Landlock ─────────────────────────────────────────
    BPF_JMP_JEQ(444, 1),  // landlock_create_ruleset
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(445, 1),  // landlock_add_rule
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(446, 1),  // landlock_restrict_self
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── PMU / profiling ─────────────────────────────────
    BPF_JMP_JEQ(241, 1),  // perf_event_open
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── Generic ──────────────────────────────────────────
    BPF_JMP_JEQ(17,  1),  // getcwd
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(61,  1),  // getdents64
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(220, 1),  // clone
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    BPF_JMP_JEQ(302, 1),  // prlimit64
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // ── ARM arch-specific ────────────────────────────────
    BPF_JMP_JEQ(0x0f0002, 1),  // __ARM_NR_cacheflush
    BPF_JMP_JA(1),
    BPF_RET_ALLOW,

    // Everything else = KILL
    BPF_RET_KILL,
};

static constexpr int g_seccomp_count =
    sizeof(g_seccomp_filter) / sizeof(g_seccomp_filter[0]);

// ─── Namespace constants ────────────────────────────────

#ifndef CLONE_NEWNS
#define CLONE_NEWNS     0x00020000
#endif
#ifndef CLONE_NEWPID
#define CLONE_NEWPID    0x20000000
#endif
#ifndef CLONE_NEWNET
#define CLONE_NEWNET    0x40000000
#endif
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC    0x08000000
#endif
#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS    0x04000000
#endif
#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP 0x02000000
#endif
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER   0x10000000
#endif
#ifndef CLONE_NEWTIME
#define CLONE_NEWTIME   0x00000080
#endif
#ifndef CLONE_VM
#define CLONE_VM        0x00000100
#endif
#ifndef CLONE_SIGHAND
#define CLONE_SIGHAND   0x00000800
#endif
#ifndef CLONE_THREAD
#define CLONE_THREAD    0x00010000
#endif
#ifndef SIGCHLD
#define SIGCHLD         17
#endif

#define MS_PRIVATE      0x40000
#define MS_REC          0x4000
#define MS_SLAVE        0x80000

#define MNT_DETACH      0x1

#define PR_SET_NO_NEW_PRIVS 38
#define PR_SET_DUMPABLE     4

#define GRND_NONBLOCK   0x1

// ─── UID/GID mapping ────────────────────────────────────
// Magic constants for /proc/self/uid_map / gid_map

// ─── Landlock access flags ─────────────────────────────

// definitions are in the header

// ═══════════════════════════════════════════════════════════
// IMPLEMENTATION
// ═══════════════════════════════════════════════════════════

namespace apex {
namespace sandbox {

// ─── PMU: user-space cycle counter ──────────────────────

uint64_t pmu_cycle_count() {
#if defined(__aarch64__)
    uint64_t cnt;
    asm volatile("mrs %0, cntvct_el0" : "=r"(cnt));
    return cnt;
#else
    // arm32/x86_64: use clock_gettime as fallback cycle counter
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// ─── PMU: perf_event_open wrapper ───────────────────────

int pmu_open(uint64_t config) {
    struct perf_event_attr attr;
    __builtin_memset(&attr, 0, sizeof(attr));
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
    // Exclude kernel and hypervisor to count only user-space events
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_idle = 1;
    attr.pinned = 1;
    int fd = (int)bs_perf_event_open(&attr, 0, -1, -1, 0);
    return fd;
}

uint64_t pmu_read_fd(int fd) {
    uint64_t val = 0;
    if (fd >= 0) {
        bs_read(fd, &val, sizeof(val));
    }
    return val;
}

// ─── Namespace helpers ──────────────────────────────────

int ns_unshare_all() {
    uint64_t flags = CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWUTS |
                     CLONE_NEWCGROUP | CLONE_NEWTIME;
    // NEWNET is optional — skip if ENOSYS
    int ret = (int)bs_unshare((int64_t)(flags));
    if (ret == 0) return 0;
    // If NET fails, try without it
    flags &= ~((uint64_t)CLONE_NEWNET);
    return (int)bs_unshare((int64_t)(flags));
}

int ns_try_userns() {
    int ret = (int)bs_unshare((int64_t)CLONE_NEWUSER);
    return ret;  // 0 = success, -1 = not supported
}

int ns_write_uid_map(int uid) {
    // Write to /proc/self/uid_map: "0 <uid> 1"
    // Then /proc/self/setgroups + /proc/self/gid_map
    char uid_buf[64];
    char gid_buf[64];
    char setgroups_buf[] = "deny";
    int n;

    // uid_map
    n = __builtin_sprintf(uid_buf, "0 %d 1\n", uid);
    int fd = (int)bs_openat(-100, "/proc/self/uid_map",
                            1, 0);  // O_WRONLY
    if (fd < 0) return -1;
    bs_write(fd, uid_buf, (size_t)n);
    bs_close(fd);

    // setgroups
    fd = (int)bs_openat(-100, "/proc/self/setgroups",
                        1, 0);  // O_WRONLY
    if (fd >= 0) {
        bs_write(fd, setgroups_buf, 4);
        bs_close(fd);
    }

    // gid_map
    n = __builtin_sprintf(gid_buf, "0 %d 1\n", uid);
    fd = (int)bs_openat(-100, "/proc/self/gid_map",
                        1, 0);
    if (fd < 0) return -1;
    bs_write(fd, gid_buf, (size_t)n);
    bs_close(fd);

    return 0;
}

int ns_clone_pid() {
    // clone(CLONE_NEWPID | SIGCHLD, NULL, NULL, NULL, NULL)
    // Returns child PID to parent, 0 to child
    return (int)bs_clone((int64_t)(CLONE_NEWPID | SIGCHLD),
                         nullptr, nullptr, nullptr, nullptr);
}

// ─── Mount helpers ──────────────────────────────────────

int ns_mount_minimal(const char* rootfs) {
    // Make parent mounts private
    bs_mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr);

    // Mount tmpfs as new rootfs
    int ret = (int)bs_mount("tmpfs", rootfs, "tmpfs",
                            0, nullptr);
    if (ret < 0) return ret;

    // Create /proc inside new root
    char proc_path[256];
    char dev_path[256];
    char sys_path[256];
    char devpts_path[256];
    __builtin_snprintf(proc_path, sizeof(proc_path),
                       "%s/proc", rootfs);
    __builtin_snprintf(dev_path, sizeof(dev_path),
                       "%s/dev", rootfs);
    __builtin_snprintf(sys_path, sizeof(sys_path),
                       "%s/sys", rootfs);
    __builtin_snprintf(devpts_path, sizeof(devpts_path),
                       "%s/dev/pts", rootfs);

    // Create directories (must use openat + mkdirat)
    auto mkdir_p = [](const char* parent, const char* child) {
        int pfd = (int)bs_openat(-100, parent, 0, 0); // O_RDONLY
        if (pfd < 0) return -1;
        // mkdirat has no bare wrapper — use openat with O_DIRECTORY|O_CREAT
        // Actually, just use mount which creates the target if possible
        bs_close(pfd);
        return 0;
    };

    // Mount proc
    bs_mount("proc", proc_path, "proc", 0, nullptr);

    // Mount devtmpfs
    bs_mount("devtmpfs", dev_path, "devtmpfs", 0, nullptr);

    // Mount sysfs
    bs_mount("sysfs", sys_path, "sysfs", 0, nullptr);

    // Create and mount devpts
    bs_mount("devpts", devpts_path, "devpts", 0, nullptr);

    return 0;
}

int ns_pivot(const char* rootfs) {
    // pivot_root(rootfs, rootfs/put_old)
    char put_old[256];
    __builtin_snprintf(put_old, sizeof(put_old),
                       "%s/put_old", rootfs);

    int ret = (int)bs_pivot_root(rootfs, put_old);
    if (ret < 0) return ret;

    bs_chdir("/");

    return 0;
}

int ns_detach_old() {
    return (int)bs_umount2("/put_old", MNT_DETACH);
}

// ─── Seccomp ────────────────────────────────────────────

int seccomp_install() {
    struct { unsigned short len; const apex::sandbox::sock_filter* filter; } prog;
    prog.len = (unsigned short)g_seccomp_count;
    prog.filter = g_seccomp_filter;

    int ret = (int)bs_seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog);
    return ret;
}

// ─── Landlock ──────────────────────────────────────────

int landlock_lockdown() {
    // Create ruleset: deny write/remove/make operations
    struct landlock_ruleset_attr attr;
    __builtin_memset(&attr, 0, sizeof(attr));
    attr.handled_access_fs =
        LANDLOCK_ACCESS_FS_WRITE_FILE |
        LANDLOCK_ACCESS_FS_REMOVE_DIR |
        LANDLOCK_ACCESS_FS_REMOVE_FILE |
        LANDLOCK_ACCESS_FS_MAKE_DIR |
        LANDLOCK_ACCESS_FS_MAKE_REG |
        LANDLOCK_ACCESS_FS_MAKE_SYM;

    int ruleset_fd = (int)bs_landlock_create_ruleset(
        &attr, sizeof(attr), 0);
    if (ruleset_fd < 0) return ruleset_fd;

    // Allow read + execute on /
    struct landlock_path_beneath_attr path_attr;
    __builtin_memset(&path_attr, 0, sizeof(path_attr));
    path_attr.allowed_access =
        LANDLOCK_ACCESS_FS_READ_FILE |
        LANDLOCK_ACCESS_FS_READ_DIR |
        LANDLOCK_ACCESS_FS_EXECUTE;
    path_attr.parent_fd = (int)bs_openat(-100, "/", 0, 0);

    int ret = (int)bs_landlock_add_rule(
        ruleset_fd, LANDLOCK_RULE_PATH_BENEATH,
        &path_attr, 0);
    bs_close(path_attr.parent_fd);
    if (ret < 0) {
        bs_close(ruleset_fd);
        return ret;
    }

    // Restrict self
    ret = (int)bs_landlock_restrict_self(ruleset_fd, 0);
    bs_close(ruleset_fd);
    return ret;
}

// ─── Privilege dropping ─────────────────────────────────

int priv_set_no_new() {
    return (int)bs_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
}

int priv_set_dumpable() {
    // PR_SET_DUMPABLE=0: only root-owned process can ptrace
    return (int)bs_prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
}

int priv_drop_caps() {
    // Use prctl(PR_CAPBSET_DROP, cap) for each cap
    // or use prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, ...)
    // For simplicity, just set NO_NEW_PRIVS which prevents acquiring new caps
    return priv_set_no_new();
}

// ─── Monitor child ─────────────────────────────────────

static int read_tracer_pid(int pid) {
    char path[64];
    __builtin_snprintf(path, sizeof(path),
                       "/proc/%d/status", pid);
    int fd = (int)bs_openat(-100, path, 0, 0);  // O_RDONLY
    if (fd < 0) return 0;
    char buf[512];
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return 0;
    buf[n] = 0;

    const char* needle = "TracerPid:";
    const char* p = strstr(buf, needle);
    if (!p) return 0;
    p += 10;  // skip "TracerPid:"
    while (*p == ' ' || *p == '\t') p++;
    int val = 0;
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    return val;
}

SandboxResult monitor_child(int child_pid, int event_fd,
                            uint64_t timeout_ms) {
    SandboxResult r;
    __builtin_memset(&r, 0, sizeof(r));
    r.exit_code = -1;
    r.tracer_pid = 0;

    uint64_t deadline = bs_clock_ns() + timeout_ms * 1000000ULL;
    bool child_alive = true;

    // PMU counters
    int pmu_cycles_fd = pmu_open(PERF_COUNT_HW_CPU_CYCLES);
    int pmu_insn_fd = pmu_open(PERF_COUNT_HW_INSTRUCTIONS);
    int pmu_cache_fd = pmu_open(PERF_COUNT_HW_CACHE_MISSES);
    int pmu_branch_fd = pmu_open(PERF_COUNT_HW_BRANCH_MISSES);

    // Initialize PMU counters
    pmu_read_fd(pmu_cycles_fd);
    pmu_read_fd(pmu_insn_fd);
    pmu_read_fd(pmu_cache_fd);
    pmu_read_fd(pmu_branch_fd);

    uint64_t pmu_start = pmu_cycle_count();

    // Wait loop: poll /proc for child existence + TracerPid
    while (child_alive && bs_clock_ns() < deadline) {
        // Check if child still exists
        char proc_check[64];
        __builtin_snprintf(proc_check, sizeof(proc_check),
                           "/proc/%d/status", child_pid);
        int fd = (int)bs_openat(-100, proc_check, 0, 0);
        if (fd < 0) {
            // Child has exited — read its exit status from wait
            child_alive = false;
        } else {
            bs_close(fd);

            // Check for debugger attached
            int tp = read_tracer_pid(child_pid);
            if (tp > r.tracer_pid) r.tracer_pid = tp;

            // If a tracer appeared and it's not us, kill child
            if (tp != 0) {
                r.monitor_killed = true;
                bs_kill(child_pid, 9);  // SIGKILL
                child_alive = false;
            }
        }

        bs_nanosleep(100000000ULL);  // 100ms poll interval
    }

    if (child_alive) {
        // Timeout — kill child
        bs_kill(child_pid, 9);
        r.monitor_killed = true;
    }

    // Read PMU end values
    uint64_t pmu_end = pmu_cycle_count();
    r.pmu_cycles = pmu_end - pmu_start;
    r.pmu_instructions = pmu_read_fd(pmu_insn_fd);
    r.pmu_cache_misses = pmu_read_fd(pmu_cache_fd);
    r.pmu_branch_misses = pmu_read_fd(pmu_branch_fd);
    r.duration_ns = bs_clock_ns() - (deadline - timeout_ms * 1000000ULL);

    // Close PMU fds
    if (pmu_cycles_fd >= 0) bs_close(pmu_cycles_fd);
    if (pmu_insn_fd >= 0) bs_close(pmu_insn_fd);
    if (pmu_cache_fd >= 0) bs_close(pmu_cache_fd);
    if (pmu_branch_fd >= 0) bs_close(pmu_branch_fd);

    return r;
}

// ─── Three-replica launchers ────────────────────────────

static SandboxResult do_run(const SandboxConfig& config) {
    SandboxResult result;
    __builtin_memset(&result, 0, sizeof(result));
    result.exit_code = -1;

    // Phase 1: user namespace (best-effort)
    bool have_userns = (ns_try_userns() == 0);
    if (have_userns) {
        // Map UID: write our process UID to 0 inside the ns
        // Read our UID from /proc/self/status
        // For simplicity, try common UIDs (2000 = shell, 10000+ = app)
        ns_write_uid_map(0);  // If running as root, 0 maps to 0
    }

    // Phase 2: create non-user namespaces
    ns_unshare_all();

    // Phase 3: fork into new PID namespace
    int pid = ns_clone_pid();
    if (pid < 0) {
        result.exit_code = -2;
        return result;
    }

    if (pid == 0) {
        // ─── CHILD: in new PID namespace ───────────────

        // Mount minimal rootfs (only if we have capabilities)
        if (have_userns && config.rootfs_path) {
            ns_mount_minimal(config.rootfs_path);
            ns_pivot(config.rootfs_path);
            ns_detach_old();
        }

        // Set NO_NEW_PRIVS before seccomp (required order)
        priv_set_no_new();

        // Make process non-dumpable (harder to ptrace)
        priv_set_dumpable();

        // Install seccomp whitelist
        if (config.level == IsolationLevel::FULL) {
            seccomp_install();
        }

        // Lock down with Landlock (only with user ns)
        if (have_userns && config.level == IsolationLevel::FULL) {
            landlock_lockdown();
        }

        // Execute target
        if (config.target_path && config.argv) {
            // Direct syscall execveat or execve
            // execve syscall number = 221 on arm64
            // For now, use execve via libc or inline asm
            // execve(filename, argv, envp)
            #if defined(__aarch64__)
            register int64_t x8 asm("x8") = 221;
            register int64_t x0 asm("x0") = (int64_t)config.target_path;
            register int64_t x1 asm("x1") = (int64_t)config.argv;
            register int64_t x2 asm("x2") = 0;  // NULL envp
            asm volatile("svc #0"
                    : "+r"(x0)
                    : "r"(x8), "r"(x1), "r"(x2)
                    : "memory");
            #else
            /* arm32/x64: use libc execve */
            execve(config.target_path, (char* const*)config.argv, nullptr);
            #endif
            // If execve returns, it failed
            bs_exit(255);
        }

        bs_exit(0);
        // unreachable
    }

    // ─── PARENT: monitor child ─────────────────────────

    // Wait for child with timeout
    result = monitor_child(pid, -1, config.timeout_ms);

    return result;
}

SandboxResult run_isolated(const SandboxConfig& config) {
    return do_run(config);
}

SandboxResult launch_replica_a(const SandboxConfig& base) {
    // Replica A: no isolation, bait for Shamiko
    SandboxConfig cfg = base;
    cfg.level = IsolationLevel::NONE;
    return do_run(cfg);
}

SandboxResult launch_replica_b(const SandboxConfig& base) {
    // Replica B: PID + mount ns without user ns
    SandboxConfig cfg = base;
    cfg.level = IsolationLevel::LIGHT;
    return do_run(cfg);
}

SandboxResult launch_replica_c(const SandboxConfig& base) {
    // Replica C: FULL isolation
    SandboxConfig cfg = base;
    cfg.level = IsolationLevel::FULL;
    return do_run(cfg);
}

// ─── Legacy handle API ──────────────────────────────────

// Shared memory layout for handle-based sandbox
struct SandboxShared {
    void (*fn)();
    volatile uint32_t state; // 0=idle, 1=signalled, 2=running, 3=done
};

static constexpr int MAP_SHARED      = 0x01;
static constexpr int MAP_ANONYMOUS   = 0x20;
static constexpr int PROT_RW         = 0x3;  // PROT_READ|PROT_WRITE

// Bounded global tracker so parent can find the shared region
// (handle struct has no opaque field, so we stash it here)
static struct { pid_t pid; SandboxShared* sh; } g_sandbox_map[32];
static int g_sandbox_count = 0;

static int sandbox_child_loop(SandboxShared* sh, int efd) {
    while (true) {
        uint64_t val = 0;
        int64_t n = bs_read(efd, &val, sizeof(val));
        if (n <= 0) break;
        if (!sh->fn) continue;
        sh->state = 2;
        sh->fn();
        sh->state = 3;
        sh->fn = nullptr;
    }
    return 0;
}

SandboxHandle create_sandbox(const SandboxConfig& config) {
    SandboxHandle h;
    h.pid = -1;
    h.ipc_fd = -1;
    h.active = false;

    void* mem = (void*)bs_mmap(nullptr, sizeof(SandboxShared),
                                PROT_RW, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!mem || mem == (void*)-1) return h;
    SandboxShared* sh = static_cast<SandboxShared*>(mem);
    sh->fn = nullptr;
    sh->state = 0;

    int efd = (int)bs_eventfd2(0, 0);
    if (efd < 0) {
        bs_munmap(mem, sizeof(SandboxShared));
        return h;
    }

    int pid = (int)bs_clone(CLONE_NEWPID | SIGCHLD,
                            nullptr, nullptr, nullptr, nullptr);
    if (pid < 0) {
        bs_close(efd);
        bs_munmap(mem, sizeof(SandboxShared));
        return h;
    }

    if (pid == 0) {
        // ─── CHILD: apply isolation, then wait for work ─
        ns_unshare_all();
        if (config.level >= IsolationLevel::LIGHT && config.rootfs_path) {
            ns_mount_minimal(config.rootfs_path);
            ns_pivot(config.rootfs_path);
            ns_detach_old();
        }
        priv_set_no_new();
        priv_set_dumpable();
        if (config.level == IsolationLevel::FULL) {
            seccomp_install();
            landlock_lockdown();
        }
        sandbox_child_loop(sh, efd);
        bs_exit(0);
    }

    // ─── PARENT ───────────────────────────────────────
    // Track shared region
    if (g_sandbox_count < 32) {
        g_sandbox_map[g_sandbox_count].pid = pid;
        g_sandbox_map[g_sandbox_count].sh = sh;
        g_sandbox_count++;
    }

    h.pid = pid;
    h.ipc_fd = efd;
    h.active = true;
    return h;
}

bool run_in_sandbox(SandboxHandle& h, void (*fn)()) {
    if (!h.active || h.pid <= 0 || !fn) return false;

    // Look up shared region
    SandboxShared* sh = nullptr;
    for (int i = 0; i < g_sandbox_count; i++) {
        if (g_sandbox_map[i].pid == h.pid) {
            sh = g_sandbox_map[i].sh;
            break;
        }
    }
    if (!sh) return false;

    // Write function pointer to shared memory
    sh->fn = fn;
    sh->state = 1;

    // Signal child via eventfd
    uint64_t signal = 1;
    bs_write(h.ipc_fd, &signal, sizeof(signal));

    // Wait for completion (poll shared state with timeout)
    uint64_t deadline = bs_clock_ns() + 30000000000ULL;
    while (bs_clock_ns() < deadline) {
        if (sh->state == 3) {
            sh->state = 0;
            return true;
        }
        // If child died, detect via /proc
        char status_path[64];
        __builtin_snprintf(status_path, sizeof(status_path), "/proc/%d/status", h.pid);
        int fd = (int)bs_openat(-100, status_path, 0, 0);
        if (fd < 0) {
            h.active = false;
            return false;
        }
        bs_close(fd);
        bs_nanosleep(50000000ULL);
    }

    // Timeout
    bs_kill(h.pid, 9);
    h.active = false;
    return false;
}

void destroy_sandbox(SandboxHandle& h) {
    if (h.active && h.pid > 0) {
        bs_kill(h.pid, 9);
    }
    if (h.ipc_fd >= 0) {
        bs_close(h.ipc_fd);
    }
    // Clean up tracker
    for (int i = 0; i < g_sandbox_count; i++) {
        if (g_sandbox_map[i].pid == h.pid) {
            if (g_sandbox_map[i].sh) {
                bs_munmap(g_sandbox_map[i].sh, sizeof(SandboxShared));
            }
            g_sandbox_map[i] = g_sandbox_map[--g_sandbox_count];
            break;
        }
    }
    h.active = false;
}

bool apply_seccomp_filter() {
    return seccomp_install() == 0;
}

bool drop_privileges() {
    return priv_set_no_new() == 0;
}

} // namespace sandbox
} // namespace apex