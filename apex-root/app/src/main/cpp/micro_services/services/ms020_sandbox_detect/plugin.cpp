#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 20;
    result.service_name = "Environment Detection";
    result.success = true;
    result.confidence = 1.0f;

    std::string flags;

    // 1. Check for common emulator files
    const char* EMULATOR_FILES[] = {
        "/system/lib/libndktranslation.so",
        "/system/lib64/libndktranslation.so",
        "/system/bin/qemu-props",
        "/system/bin/qemu-trace",
        "/dev/socket/qemud",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/system/bin/androVM-prop",
        "/data/androVM/"
    };

    for (size_t i = 0; i < sizeof(EMULATOR_FILES) / sizeof(EMULATOR_FILES[0]); i++) {
        int fd = static_cast<int>(bs_openat(-100, EMULATOR_FILES[i], 0, 0));
        if (fd >= 0) {
            bs_close(fd);
            if (!flags.empty()) flags += "; ";
            flags += "emu_file";
            result.success = false;
            result.confidence = 0.9f;
        }
    }

    // 2. Check for QEMU-specific /proc entries
    int fd = static_cast<int>(bs_openat(-100, "/proc/tty/drivers", 0, 0));
    if (fd >= 0) {
        char buf[1024];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, "goldfish") || strstr(buf, "qemu")) {
                if (!flags.empty()) flags += "; ";
                flags += "qemu_tty";
                result.success = false;
                result.confidence = 0.95f;
            }
        }
    }

    if (!result.success) {
        result.description = "Virtualization/emulator evidence: " + flags;
    } else {
        result.description = "Physical device environment confirmed";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    20, "Environment Detection", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
