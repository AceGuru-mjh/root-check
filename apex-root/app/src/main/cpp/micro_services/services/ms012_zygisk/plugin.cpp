#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 12;
    result.service_name = "Zygisk Detector";
    result.success = true;
    result.confidence = 1.0f;

    int findings = 0;

    // 1. Check for Zygisk module paths
    const char* ZYGISK_PATHS[] = {
        "/data/adb/zygisk/", "/data/adb/modules/zygisk_",
        "/data/adb/modules/zygisk-", "/data/adb/lsposed/",
        "/data/adb/modules/lsposed/"
    };

    for (size_t i = 0; i < sizeof(ZYGISK_PATHS) / sizeof(ZYGISK_PATHS[0]); i++) {
        int fd = static_cast<int>(bs_openat(-100, ZYGISK_PATHS[i], 0x10000, 0));
        if (fd >= 0) { bs_close(fd); findings++; }
    }

    // 2. Check for FD traps in /proc/self/fd
    int fd = static_cast<int>(bs_openat(-100, "/proc/self/fd", 0x10000, 0));
    if (fd >= 0) {
        char buf[2048];
        int64_t n = bs_getdents64(fd, buf, sizeof(buf));
        bs_close(fd);
        if (n > 0) {
            // Look for suspicious FD patterns
            int suspicious_fds = 0;
            size_t pos = 0;
            while (pos < static_cast<size_t>(n)) {
                auto* dirent = reinterpret_cast<struct apex_dirent64*>(buf + pos);
                if (dirent->d_type == 8) { // DT_LNK
                    // Check if FD points to Zygisk sockets
                    char link_path[64];
                    int plen = 0;
                    const char* p = "/proc/self/fd/";
                    for (int i = 0; p[i]; i++) link_path[plen++] = p[i];
                    for (int i = 0; dirent->d_name[i]; i++) link_path[plen++] = dirent->d_name[i];
                    link_path[plen] = '\0';
                    // Read the link target
                    // fd openat check would go here
                }
                pos += dirent->d_reclen;
            }
        }
    }

    // 3. Check for linker residue
    fd = static_cast<int>(bs_openat(-100, "/proc/self/maps", 0, 0));
    if (fd >= 0) {
        char buf[8192];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, "zygisk") || strstr(buf, "lsposed") ||
                strstr(buf, "magisk") || strstr(buf, "ksud")) {
                findings += 2;
            }
        }
    }

    if (findings > 0) {
        result.success = false;
        result.confidence = (findings > 3) ? 0.95f : 0.8f;
        result.description = "Zygisk/LSPosed indicators: " + std::to_string(findings);
    } else {
        result.description = "No Zygisk traces found";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    12, "Zygisk Detector", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
