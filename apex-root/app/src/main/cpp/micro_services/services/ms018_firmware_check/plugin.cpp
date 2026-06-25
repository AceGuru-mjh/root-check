#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 18;
    result.service_name = "Firmware Check";
    result.success = true;
    result.confidence = 1.0f;

    // Check various firmware-related system paths
    const char* FIRMWARE_PATHS[] = {
        "/sys/firmware/devicetree/base/model",
        "/proc/device-tree/model",
        "/sys/class/dmi/id/product_name",
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
    };

    int accessible = 0;
    for (size_t i = 0; i < sizeof(FIRMWARE_PATHS) / sizeof(FIRMWARE_PATHS[0]); i++) {
        int fd = static_cast<int>(bs_openat(-100, FIRMWARE_PATHS[i], 0, 0));
        if (fd >= 0) {
            accessible++;
            bs_close(fd);
        }
    }

    // Check for device model
    int fd = static_cast<int>(bs_openat(-100, "/sys/firmware/devicetree/base/model", 0, 0));
    if (fd >= 0) {
        char buf[256] = {0};
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            result.description = "Device: ";
            result.description += buf;
            // Check for emulator indicators
            if (strstr(buf, "Android Emulator") || strstr(buf, "qemu") ||
                strstr(buf, "VirtualBox") || strstr(buf, "VMware")) {
                result.success = false;
                result.confidence = 0.95f;
                result.description = "Virtual environment detected: ";
                result.description += buf;
            }
            return result;
        }
    }

    result.description = "Firmware verified (" + std::to_string(accessible) + " nodes)";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    18, "Firmware Check", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
