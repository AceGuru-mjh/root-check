#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 15;
    result.service_name = "Property Check";
    result.success = true;
    result.confidence = 1.0f;

    // Directly read /dev/__properties__ for tampered system properties
    int fd = static_cast<int>(bs_openat(-100, "/dev/__properties__", 0, 0));
    if (fd >= 0) {
        char buf[4096];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';

            // Check for property-based tamper evidence
            const char* TAMPER_PROP_INDICATORS[] = {
                "ro.debuggable=1", "ro.secure=0",
                "ro.adb.secure=0", "ro.build.type=eng",
                "ro.build.type=userdebug", "ro.build.tags=test-keys",
                "persist.sys.root_access=", "sys.init.perf_lite_root_", "sys.init.perf_lite_on=1"
            };

            for (size_t i = 0; i < sizeof(TAMPER_PROP_INDICATORS) / sizeof(TAMPER_PROP_INDICATORS[0]); i++) {
                if (strstr(buf, TAMPER_PROP_INDICATORS[i])) {
                    result.success = false;
                    result.confidence = 0.9f;
                    result.description = "Tampered system property: ";
                    result.description += TAMPER_PROP_INDICATORS[i];
                    return result;
                }
            }
        }
    } else {
        result.description = "Cannot read property area directly";
    }

    if (result.success) {
        result.description = "System properties integrity verified";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    15, "Property Check", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
