#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 3;
    result.service_name = "Memory Scanner";
    result.success = true;
    result.confidence = 1.0f;

    // Read /proc/self/maps
    int fd = static_cast<int>(bs_openat(-100, "/proc/self/maps", 0, 0));
    if (fd < 0) {
        result.description = "Cannot access memory maps";
        return result;
    }

    char buf[8192];
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) {
        result.description = "Empty memory maps";
        return result;
    }
    buf[n] = '\0';

    // Check for suspicious memory regions
    int anon_exec = 0;
    int suspicious = 0;

    char* line = buf;
    char* end = buf + n;
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';

        // Look for "rwx" or anonymous executable mappings
        char* perms = line;
        while (*perms && *perms != ' ') perms++;
        if (*perms == ' ') perms++;
        if (perms[0] == 'r' && perms[1] == 'w' && perms[2] == 'x') {
            anon_exec++;
        }

        // Check for memfd or JIT regions
        if (strstr(line, "memfd") || strstr(line, "/jit") || strstr(line, "[vdso]")) {
            suspicious++;
        }

        line = nl + 1;
    }

    if (anon_exec > 3 || suspicious > 2) {
        result.success = false;
        result.confidence = 0.85f;
        result.description = "Suspicious memory regions detected";
    } else {
        result.description = "Memory regions: " + std::to_string(anon_exec) + " exec";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    3, "Memory Scanner", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
