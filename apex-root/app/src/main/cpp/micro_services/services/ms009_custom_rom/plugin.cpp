#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 9;
    result.service_name = "Custom ROM Detector";
    result.success = true;
    result.confidence = 1.0f;

    // Read build properties directly from /proc/version and system properties
    int fd = static_cast<int>(bs_openat(-100, "/proc/version", 0, 0));
    if (fd >= 0) {
        char buf[1024];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            // Check for custom ROM indicators in kernel version
            if (strstr(buf, "LineageOS") || strstr(buf, "crDroid") ||
                strstr(buf, "PixelExperience") || strstr(buf, "EvolutionX") ||
                strstr(buf, "custom") || strstr(buf, "unofficial") ||
                strstr(buf, "userdebug") || strstr(buf, "eng.")) {
                result.success = false;
                result.confidence = 0.8f;
                result.description = "Custom ROM indicators detected";
                return result;
            }
        }
    }

    // Check for custom build fingerprint in system properties area
    // by reading /dev/__properties__ directly (bypasses getprop)
    fd = static_cast<int>(bs_openat(-100, "/dev/__properties__", 0, 0));
    if (fd >= 0) {
        char buf[4096];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, "test-keys") || strstr(buf, "userdebug")) {
                result.success = false;
                result.confidence = 0.75f;
                result.description = "Custom build keys detected";
                return result;
            }
        }
    }

    result.description = "Stock ROM detected";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    9, "Custom ROM Detector", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
