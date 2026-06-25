#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 13;
    result.service_name = "Hook Detector";
    result.success = true;
    result.confidence = 1.0f;

    std::string findings;

    // 1. Check for Xposed/Frida in /proc/self/maps
    int fd = static_cast<int>(bs_openat(-100, "/proc/self/maps", 0, 0));
    if (fd >= 0) {
        char buf[8192];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            const char* SUSPECT_MODULES[] = {
                "frida", "xposed", "posed", "edxp", "lsposed",
                "substrate", "cydia", "hook", "inline_hook",
                "gadget", "libfrida", "libsrloader"
            };
            for (size_t i = 0; i < sizeof(SUSPECT_MODULES) / sizeof(SUSPECT_MODULES[0]); i++) {
                if (strstr(buf, SUSPECT_MODULES[i])) {
                    if (!findings.empty()) findings += "; ";
                    findings += SUSPECT_MODULES[i];
                }
            }
        }
    }

    // 2. Check for ptrace attachment
    fd = static_cast<int>(bs_openat(-100, "/proc/self/status", 0, 0));
    if (fd >= 0) {
        char buf[1024];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            char* tracer = strstr(buf, "TracerPid:");
            if (tracer) {
                int pid = 0;
                for (int i = 10; tracer[i] >= '0' && tracer[i] <= '9'; i++) {
                    pid = pid * 10 + (tracer[i] - '0');
                }
                if (pid != 0) {
                    if (!findings.empty()) findings += "; ";
                    findings += "ptrace detected (pid " + std::to_string(pid) + ")";
                }
            }
        }
    }

    if (!findings.empty()) {
        result.success = false;
        result.confidence = 0.9f;
        result.description = findings;
    } else {
        result.description = "No hooks detected";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    13, "Hook Detector", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
