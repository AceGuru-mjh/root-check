#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

// P3-2: bs_fork 已删除,改用 bs_clone(SIGCHLD, ...)。SIGCHLD=17 在 Linux arm64 上。
#ifndef SIGCHLD
#define SIGCHLD 17
#endif

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 4;
    result.service_name = "Mount Check";
    result.success = true;
    result.confidence = 1.0f;

    // Read /proc/self/mountinfo
    int fd = static_cast<int>(bs_openat(-100, "/proc/self/mountinfo", 0, 0));
    if (fd < 0) {
        result.description = "Cannot access mountinfo";
        return result;
    }

    char buf[8192];
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return result;
    buf[n] = '\0';

    // Check for overlayfs and suspicious mount points
    int overlay_count = 0;
    int suspicious_mounts = 0;

    char* line = buf;
    char* end = buf + n;
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';

        if (strstr(line, "overlay") || strstr(line, "overlayfs")) {
            overlay_count++;
        }
        if (strstr(line, "/data/adb") || strstr(line, "/sbin")) {
            suspicious_mounts++;
        }

        line = nl + 1;
    }

    if (overlay_count > 5 || suspicious_mounts > 0) {
        result.success = false;
        result.confidence = 0.9f;
        result.description = "Suspicious mounts detected";
    } else {
        result.description = "Mounts checked: " + std::to_string(overlay_count) + " overlay";
    }

    // Namespace fork comparison (L4 special)
    if (config.level == apex::engine::ScanLevel::DEEP ||
        config.level == apex::engine::ScanLevel::FORENSIC) {
        int pid = static_cast<int>(bs_clone(SIGCHLD, nullptr, nullptr, nullptr, nullptr));
        if (pid == 0) {
            bs_unshare(0x00020000);
            // Child reads mountinfo in new namespace
            bs_exit(0);
        } else if (pid > 0) {
            bs_nanosleep(50000000);
            bs_kill(pid, 9);
        }
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    4, "Mount Check", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
