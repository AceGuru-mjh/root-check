#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 19;
    result.service_name = "Hardware Probe";
    result.success = true;
    result.confidence = 1.0f;

    // Probe CPU info
    int fd = static_cast<int>(bs_openat(-100, "/proc/cpuinfo", 0, 0));
    if (fd >= 0) {
        char buf[2048];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            // Check for emulator CPU indicators
            if (strstr(buf, "qemu") || strstr(buf, "Virtual") || strstr(buf, "KVM")) {
                result.success = false;
                result.confidence = 0.9f;
                result.description = "Virtual CPU detected";
                return result;
            }
        }
    }

    // Check online CPUs
    fd = static_cast<int>(bs_openat(-100, "/sys/devices/system/cpu/online", 0, 0));
    if (fd >= 0) {
        char buf[64] = {0};
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            result.description = "Online CPUs: ";
            result.description += buf;
        }
    } else {
        result.description = "Hardware probe completed";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    19, "Hardware Probe", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
