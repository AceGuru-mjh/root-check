#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 2;
    result.service_name = "Process Scanner";
    result.success = true;
    result.confidence = 1.0f;

    // Open /proc
    int fd = static_cast<int>(bs_openat(-100, "/proc", 0x10000, 0));
    if (fd < 0) {
        result.success = false;
        result.confidence = 0.5f;
        result.description = "Cannot access /proc";
        return result;
    }

    char buf[4096];
    int64_t n = bs_getdents64(fd, buf, sizeof(buf));
    bs_close(fd);

    int proc_count = 0;
    size_t pos = 0;
    while (pos < static_cast<size_t>(n)) {
        auto* dirent = reinterpret_cast<struct apex_dirent64*>(buf + pos);
        if (dirent->d_type == 4) {
            bool is_digit = true;
            for (int i = 0; dirent->d_name[i]; i++) {
                if (dirent->d_name[i] < '0' || dirent->d_name[i] > '9') {
                    is_digit = false;
                    break;
                }
            }
            if (is_digit) proc_count++;
        }
        pos += dirent->d_reclen;
    }

    result.description = "Processes found: " + std::to_string(proc_count);
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    2, "Process Scanner", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
