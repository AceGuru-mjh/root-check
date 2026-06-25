#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 11;
    result.service_name = "Play Integrity Fix Detector";
    result.success = true;
    result.confidence = 1.0f;

    // Check for Play Integrity Fix / PIF module indicators
    const char* PIF_INDICATORS[] = {
        "/data/adb/pif.json", "/data/adb/modules/playintegrityfix/",
        "/data/adb/zygisk/playintegrityfix/",
        "/data/local/tmp/pif.json"
    };

    for (size_t i = 0; i < sizeof(PIF_INDICATORS) / sizeof(PIF_INDICATORS[0]); i++) {
        int fd = static_cast<int>(bs_openat(-100, PIF_INDICATORS[i], 0, 0));
        if (fd >= 0) {
            bs_close(fd);
            result.success = false;
            result.confidence = 0.9f;
            result.description = "Play Integrity Fix module detected";
            return result;
        }
    }

    // Check for spoofed build properties in /proc/cmdline
    int fd = static_cast<int>(bs_openat(-100, "/proc/cmdline", 0, 0));
    if (fd >= 0) {
        char buf[1024];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            // Check for official Google fingerprints
            if (strstr(buf, "google") && !strstr(buf, "androidboot.vbmeta.device_state=locked")) {
                // Suspicious if Google fingerprint but unlocked
                result.success = false;
                result.confidence = 0.7f;
                result.description = "Possible build fingerprint spoofing";
                return result;
            }
        }
    }

    result.description = "No PIF indicators found";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    11, "Play Integrity Fix Detector", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
