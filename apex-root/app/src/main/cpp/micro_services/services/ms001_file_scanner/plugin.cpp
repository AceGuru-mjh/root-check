#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

static bool initialized = false;

static const char* SUSPECT_PATHS[] = {
    "/data/local/tmp/", "/data/adb/", "/data/adb/magisk/",
    "/data/adb/ksu/", "/data/adb/ap/", "/data/adb/modules/",
    "/system/xbin/su", "/system/bin/su", "/sbin/su",
    "/system/bin/.ext/", "/system/xbin/daemonsu",
    "/data/local/xbin/su", "/data/local/bin/su",
    "/data/local/su", "/su", "/su/bin/"
};

static const char* SUSPECT_FILES[] = {
    "su", "magisk", "magiskinit", "magiskpolicy",
    "ksud", "ksu", "apd", "apatch", "busybox",
    "frida-server", "fridaserver", "frida",
    "zygisk", "zygiskd", "lsposed", "edxposed"
};

extern "C" bool init() {
    initialized = true;
    return true;
}

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 1;
    result.service_name = "File Scanner";
    result.success = true;
    result.confidence = 1.0f;
    result.description = "Scanning for root binaries and hidden paths";

    int found = 0;
    for (size_t i = 0; i < sizeof(SUSPECT_PATHS) / sizeof(SUSPECT_PATHS[0]); i++) {
        int fd = static_cast<int>(bs_openat(-100, SUSPECT_PATHS[i], 0x10000, 0));
        if (fd >= 0) {
            bs_close(fd);
            found++;
        }
    }

    for (size_t i = 0; i < sizeof(SUSPECT_FILES) / sizeof(SUSPECT_FILES[0]); i++) {
        char path[256];
        path[0] = '/'; path[1] = '\0';
        // Concatenate / + SUSPECT_FILES[i]
        int pos = 0;
        path[pos++] = '/';
        for (const char* f = SUSPECT_FILES[i]; *f; f++) path[pos++] = *f;
        path[pos] = '\0';

        int fd = static_cast<int>(bs_openat(-100, path, 0, 0));
        if (fd >= 0) {
            bs_close(fd);
            found++;
        }
    }

    if (found > 0) {
        result.success = false;
        result.confidence = 0.9f;
        result.description = "Suspicious files detected";
    }

    return result;
}

extern "C" void cleanup() {
    initialized = false;
}

// Plugin registration
static apex::engine::ServicePlugin plugin = {
    1, "File Scanner", "1.0.0",
    init, execute, cleanup
};

__attribute__((constructor)) void register_plugin() {
    apex::engine::service_engine::register_service(plugin);
}
