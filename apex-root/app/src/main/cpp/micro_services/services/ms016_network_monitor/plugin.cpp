#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 16;
    result.service_name = "Network Monitor";
    result.success = true;
    result.confidence = 1.0f;

    // Read /proc/net/tcp and /proc/net/tcp6 for active connections
    int fd = static_cast<int>(bs_openat(-100, "/proc/net/tcp", 0, 0));
    if (fd >= 0) {
        char buf[4096];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            int connections = 0;
            char* line = buf;
            char* end = buf + n;
            while (line < end) {
                if (*line == '\n') connections++;
                line++;
            }
            result.description = "Active TCP connections: " + std::to_string(connections);
        }
    } else {
        result.description = "Cannot access /proc/net";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    16, "Network Monitor", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
