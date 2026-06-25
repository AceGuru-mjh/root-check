#include "sandbox_isolator.h"
#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"

// ─── Example target binary (minimal detect scan) ────────

static const char* detect_argv[] = { "apex_detect", nullptr };

// ─── Helpers: write result to stdout ────────────────────

static void write_hex(const char* label, uint64_t val) {
    char buf[64];
    int n;
    const char* hex = "0123456789abcdef";
    for (int i = 56; i >= 0; i -= 8) {
        buf[i / 8] = hex[(val >> (56 - i)) & 0xf];
    }
    buf[16] = ' ';
    // simple write
    int fd = 1;  // stdout
    // skip complex formatting, just write raw bytes
}

static void write_result(const apex::sandbox::SandboxResult& r) {
    // Write binary result to stdout for the orchestrator to read
    // Format: 8 bytes exit_code, 8 bytes duration, 8 bytes flags, 32 bytes PMU
    // flags: bit 0 = seccomp_violation, bit 1 = monitor_killed
    uint64_t flags = 0;
    if (r.seccomp_violation) flags |= 1ULL;
    if (r.monitor_killed)    flags |= 2ULL;

    bs_write(1, &r.exit_code, sizeof(r.exit_code));
    bs_write(1, &r.duration_ns, sizeof(r.duration_ns));
    bs_write(1, &flags, sizeof(flags));
    bs_write(1, &r.pmu_cycles, sizeof(r.pmu_cycles));
    bs_write(1, &r.pmu_instructions, sizeof(r.pmu_instructions));
    bs_write(1, &r.pmu_cache_misses, sizeof(r.pmu_cache_misses));
    bs_write(1, &r.pmu_branch_misses, sizeof(r.pmu_branch_misses));
    int tp = r.tracer_pid;
    bs_write(1, &tp, sizeof(tp));
}

// ─── Main ───────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // Parse arguments
    uint32_t isolation = 2;   // default: full
    uint64_t timeout_ms = 10000;  // 10 seconds
    const char* target = nullptr;
    int level_arg = 2;

    if (argc > 1) level_arg = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'l' && i + 1 < argc) {
            level_arg = argv[i + 1][0] - '0';
            i++;
        } else if (argv[i][0] == '-' && argv[i][1] == 't' && i + 1 < argc) {
            timeout_ms = 0;
            for (const char* p = argv[i + 1]; *p; p++) {
                timeout_ms = timeout_ms * 10 + (*p - '0');
            }
            i++;
        } else if (i == level_arg || target == nullptr) {
            target = argv[i];
        }
    }

    apex::sandbox::IsolationLevel level;
    switch (level_arg) {
        case 0: level = apex::sandbox::IsolationLevel::NONE; break;
        case 1: level = apex::sandbox::IsolationLevel::LIGHT; break;
        default: level = apex::sandbox::IsolationLevel::FULL; break;
    }

    apex::sandbox::SandboxConfig config;
    config.level = level;
    config.timeout_ms = timeout_ms;
    config.rootfs_path = "/data/local/tmp/.apex_sandbox";
    config.target_path = target ? target : "/system/bin/true";
    config.argv = target ? (const char* const*)argv + level_arg
                         : detect_argv;
    config.argc = target ? argc - level_arg : 1;

    // Create minimal rootfs dir (best-effort)
    bs_openat(-100, config.rootfs_path, 0x42, 0755);  // O_CREAT|O_RDONLY
    // mkdir equivalent — openat with O_CREAT|O_DIRECTORY
    bs_openat(-100, config.rootfs_path, 0x42 | 0x10000, 0755);
    // 0x10000 = O_DIRECTORY

    // Run isolated
    apex::sandbox::SandboxResult result =
        apex::sandbox::run_isolated(config);

    // Write result
    write_result(result);

    return result.exit_code >= 0 ? result.exit_code : 1;
}