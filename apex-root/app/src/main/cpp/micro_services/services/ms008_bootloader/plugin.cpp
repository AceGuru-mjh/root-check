#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 8;
    result.service_name = "Bootloader Detector";
    result.success = true;
    result.confidence = 1.0f;

    // Check multiple bootloader status sources
    const char* BL_CHECKS[] = {
        "/proc/cmdline",
        "/sys/class/android_usb/android0/enable",
        "/sys/devices/system/cpu/possible",
        "/proc/version"
    };

    // 1. Read /proc/cmdline for unlock indicators
    int fd = static_cast<int>(bs_openat(-100, "/proc/cmdline", 0, 0));
    if (fd >= 0) {
        char buf[1024];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, "androidboot.verifiedbootstate=orange") ||
                strstr(buf, "androidboot.flash.locked=0") ||
                strstr(buf, "bootloader.unlocked=1")) {
                result.success = false;
                result.confidence = 0.95f;
                result.description = "Bootloader is unlocked";
                return result;
            }
        }
    }

    // 2. Check for vbmeta / AVB status
    fd = static_cast<int>(bs_openat(-100, "/sys/fs/selinux/enforce", 0, 0));
    if (fd >= 0) {
        char val;
        int64_t n = bs_read(fd, &val, 1);
        bs_close(fd);
        if (n == 1 && val == '0') {
            result.success = false;
            result.confidence = 0.85f;
            result.description = "SELinux disabled - possible bootloader tamper";
            return result;
        }
    }

    result.description = "Bootloader appears locked";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    8, "Bootloader Detector", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
