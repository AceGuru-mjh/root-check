#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 14;
    result.service_name = "SELinux Check";
    result.success = true;
    result.confidence = 1.0f;

    // 1. Check enforce status
    int fd = static_cast<int>(bs_openat(-100, "/sys/fs/selinux/enforce", 0, 0));
    if (fd >= 0) {
        char val;
        if (bs_read(fd, &val, 1) == 1) {
            bs_close(fd);
            if (val == '0') {
                result.success = false;
                result.confidence = 0.95f;
                result.description = "SELinux is disabled (permissive)";
                return result;
            }
        } else {
            bs_close(fd);
        }
    } else {
        // Cannot access selinux status
        result.description = "Cannot access SELinux status";
        return result;
    }

    // 2. Check selinuxfs for integrity
    fd = static_cast<int>(bs_openat(-100, "/sys/fs/selinux/policy", 0, 0));
    if (fd < 0) {
        result.success = false;
        result.confidence = 0.8f;
        result.description = "SELinux policy inaccessible";
        return result;
    }
    bs_close(fd);

    // 3. Check context of our process
    fd = static_cast<int>(bs_openat(-100, "/proc/self/attr/current", 0, 0));
    if (fd >= 0) {
        char ctx[256] = {0};
        int64_t n = bs_read(fd, ctx, sizeof(ctx) - 1);
        bs_close(fd);
        if (n > 0) {
            ctx[n] = '\0';
            result.description = "SELinux enforcing, context: ";
            result.description += ctx;
            if (strstr(ctx, "untrusted_app")) {
                // Expected for normal apps
            }
        }
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    14, "SELinux Check", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
