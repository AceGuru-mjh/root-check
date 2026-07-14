#ifndef APEX_ROOT_SANDBOX_ISOLATOR_H
#define APEX_ROOT_SANDBOX_ISOLATOR_H

#include <cstdint>
#include <cstddef>

namespace apex {
namespace sandbox {

// ─── Isolation levels for three-replica consensus ────────

enum class IsolationLevel : uint32_t {
    NONE = 0,           // Replica A: bait, no isolation
    LIGHT = 1,          // Replica B: mount+pid ns, no user ns
    FULL = 2            // Replica C: user+mount+pid+net ns + seccomp + landlock
};

// ─── Seccomp action constants ────────────────────────────

static constexpr uint32_t SECCOMP_RET_KILL_PROCESS = 0x80000000U;
static constexpr uint32_t SECCOMP_RET_ALLOW        = 0x7fff0000U;
static constexpr uint32_t SECCOMP_SET_MODE_FILTER  = 2;

// BPF instruction
struct sock_filter {
    uint16_t code;
    uint8_t  jt;
    uint8_t  jf;
    uint32_t k;
};

// ─── Landlock constants ─────────────────────────────────

static constexpr uint32_t LANDLOCK_CREATE_RULESET_VERSION = 1;
static constexpr int LANDLOCK_ACCESS_FS_READ_FILE          = 1ULL << 0;
static constexpr int LANDLOCK_ACCESS_FS_WRITE_FILE         = 1ULL << 1;
static constexpr int LANDLOCK_ACCESS_FS_EXECUTE            = 1ULL << 2;
static constexpr int LANDLOCK_ACCESS_FS_READ_DIR           = 1ULL << 3;
static constexpr int LANDLOCK_ACCESS_FS_REMOVE_DIR         = 1ULL << 4;
static constexpr int LANDLOCK_ACCESS_FS_REMOVE_FILE        = 1ULL << 5;
static constexpr int LANDLOCK_ACCESS_FS_MAKE_DIR           = 1ULL << 6;
static constexpr int LANDLOCK_ACCESS_FS_MAKE_REG           = 1ULL << 7;
static constexpr int LANDLOCK_ACCESS_FS_MAKE_SYM           = 1ULL << 8;
static constexpr int LANDLOCK_RULE_PATH_BENEATH             = 1;

struct landlock_ruleset_attr {
    uint64_t handled_access_fs;
};

struct landlock_path_beneath_attr {
    uint64_t allowed_access;
    int      parent_fd;
};

// ─── Perf event constants ───────────────────────────────

struct perf_event_attr {
    uint32_t type;
    uint32_t size;
    uint64_t config;
    uint64_t sample_period;
    // (simplified — only fields needed for cycle/branch counting)
    uint64_t read_format;
    uint32_t pinned;
    uint32_t exclusive;
    uint32_t exclude_kernel;
    uint32_t exclude_hv;
    uint32_t exclude_idle;
};

static constexpr uint32_t PERF_TYPE_HARDWARE        = 0;
static constexpr uint64_t PERF_COUNT_HW_CPU_CYCLES  = 0;
static constexpr uint64_t PERF_COUNT_HW_INSTRUCTIONS = 1;
static constexpr uint64_t PERF_COUNT_HW_CACHE_MISSES = 3;
static constexpr uint64_t PERF_COUNT_HW_BRANCH_MISSES = 5;

// ─── Sandbox config ─────────────────────────────────────

struct SandboxConfig {
    IsolationLevel   level;
    uint64_t         timeout_ms;
    const char*      rootfs_path;     // tmpfs mount point
    const char*      target_path;     // executable to run
    const char* const* argv;
    int              argc;
};

// ─── Sandbox result ──────────────────────────────────────

struct SandboxResult {
    int      exit_code;
    uint64_t duration_ns;
    bool     seccomp_violation;
    bool     monitor_killed;
    uint64_t pmu_cycles;
    uint64_t pmu_instructions;
    uint64_t pmu_cache_misses;
    uint64_t pmu_branch_misses;
    int      tracer_pid;             // non-zero if debugger attached
};

// ─── Monitor event (sent via eventfd by child) ──────────

enum class MonitorEvent : uint64_t {
    CHILD_RUNNING = 0,
    CHILD_EXITED  = 1,
    CHILD_SECCOMP = 2,
    CHILD_SIGNAL  = 3,
    HEARTBEAT     = 4
};

// ─── Core API ───────────────────────────────────────────

// Create an isolated sandbox and execute the target.
// Blocks until completion and returns result + PMU telemetry.
SandboxResult run_isolated(const SandboxConfig& config);

// ─── Three-replica helpers ──────────────────────────────

// These set the isolation level and call run_isolated internally.
SandboxResult launch_replica_a(const SandboxConfig& base);
SandboxResult launch_replica_b(const SandboxConfig& base);
SandboxResult launch_replica_c(const SandboxConfig& base);

// ─── Low-level primitives (exposed for unit testing) ────

int  ns_unshare_all();          // unshare all non-user namespaces
int  ns_try_userns();           // try CLONE_NEWUSER, returns 0 on success
int  ns_write_uid_map(int uid); // write uid/gid maps after NEWUSER
int  ns_clone_pid();            // clone(CLONE_NEWPID|SIGCHLD)
int  ns_mount_minimal(const char* rootfs); // tmpfs + /proc + /dev + /sys
int  ns_pivot(const char* rootfs);         // pivot_root + chdir
int  ns_detach_old();                     // umount old root

int  seccomp_install();         // apply BPF whitelist, SIGKILL on violation
int  landlock_lockdown();       // restrict FS to read-only

int  priv_set_no_new();         // prctl(NO_NEW_PRIVS)
int  priv_set_dumpable();       // prctl(PR_SET_DUMPABLE, 0)
int  priv_drop_caps();          // drop all bounding+ambient caps

int  pmu_open(uint64_t config); // perf_event_open for a counter
uint64_t pmu_read_fd(int fd);   // read a perf_event fd
uint64_t pmu_cycle_count();     // user-space cntvct_el0

// Monitor loop: returns child result plus PMU snapshot
SandboxResult monitor_child(int child_pid, int event_fd,
                            uint64_t timeout_ms);



// ─── Legacy handle API (for backward compatibility) ────

struct SandboxHandle {
    int  pid;
    int  ipc_fd;
    bool active;
};

SandboxHandle create_sandbox(const SandboxConfig& config);
bool         run_in_sandbox(SandboxHandle& h, void (*fn)());
void         destroy_sandbox(SandboxHandle& h);

} // namespace sandbox
} // namespace apex
#endif