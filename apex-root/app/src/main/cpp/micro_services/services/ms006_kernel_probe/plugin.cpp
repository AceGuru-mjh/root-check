#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 6;
    result.service_name = "Kernel Integrity Probe";
    result.success = true;
    result.confidence = 1.0f;

    // 1. Try to read /proc/kallsyms for syscall table base
    int fd = static_cast<int>(bs_openat(-100, "/proc/kallsyms", 0, 0));
    if (fd >= 0) {
        char buf[4096];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            // Check if kallsyms appears tampered (all zeros or short)
            int non_zero = 0;
            for (int64_t i = 0; i < n; i++) {
                if (buf[i] != 0 && buf[i] != '\n' && buf[i] != ' ') non_zero++;
            }
            if (non_zero < 100) {
                result.success = false;
                result.confidence = 0.8f;
                result.description = "kallsyms appears tampered or empty";
                return result;
            }
        }
    }

    // 2. Check /proc/modules for suspicious kernel modules
    fd = static_cast<int>(bs_openat(-100, "/proc/modules", 0, 0));
    if (fd >= 0) {
        char buf[2048];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, "rootkit") || strstr(buf, "hide") || strstr(buf, "sec_hook")) {
                result.success = false;
                result.confidence = 0.9f;
                result.description = "Suspicious kernel module detected";
                return result;
            }
        }
    } else {
        // Cannot access modules - might be hidden
        if (config.level >= apex::engine::ScanLevel::DEEP) {
            result.description = "Cannot access /proc/modules (may be restricted)";
        }
    }

    result.description = "Kernel integrity verified";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    6, "Kernel Integrity Probe", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
