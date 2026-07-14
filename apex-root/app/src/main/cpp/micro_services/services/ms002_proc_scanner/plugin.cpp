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
    while (pos + sizeof(struct apex_dirent64) <= static_cast<size_t>(n)) {
        auto* dirent = reinterpret_cast<struct apex_dirent64*>(buf + pos);
        // P0-1 修复: 校验 d_reclen 防止无限循环和越界读取
        if (dirent->d_reclen == 0 || dirent->d_reclen > static_cast<size_t>(n) - pos) {
            break;
        }
        if (dirent->d_type == 4) {
            bool is_digit = true;
            const char* name_end = (const char*)dirent + dirent->d_reclen;
            for (const char* c = dirent->d_name; c < name_end && *c; c++) {
                if (*c < '0' || *c > '9') {
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
