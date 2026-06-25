#include "self_protection.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <link.h>

namespace {

static bool is_debugged() {
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) return true;

    int fd = (int)bs_openat(-100, "/proc/self/status", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[1024] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    const char *needle = "TracerPid:";
    const char *p = strstr(buf, needle);
    if (!p) return false;
    p += strlen(needle);
    while (*p == ' ' || *p == '\t') p++;
    return *p != '0';
}

static bool is_injected() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    const char *suspicious[] = {
        "/data/local/", "/data/data/", "/sdcard/",
        "frida", "xposed", "libinject", "libriru", "liblspd"
    };
    for (auto &s : suspicious) {
        if (strstr(buf, s)) return true;
    }
    return false;
}

static bool has_rwx_segments() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;
    return strstr(buf, "rwxp") != nullptr;
}

static bool is_hooked() {
    void *libc = dlopen("libc.so", RTLD_NOW);
    if (!libc) return true;
    void *open_addr = dlsym(libc, "open");
    void *read_addr = dlsym(libc, "read");
    if (!open_addr || !read_addr) { dlclose(libc); return true; }
    Dl_info info;
    if (dladdr(open_addr, &info) == 0) { dlclose(libc); return true; }
    dlclose(libc);
    return false;
}

static bool has_breakpoints() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (strstr(line, "r-xp") && strstr(line, "apex")) {
            char *dash = strchr(line, '-');
            char *space = strchr(line, ' ');
            if (dash && space) {
                *dash = 0;
                *space = 0;
                unsigned long start = strtoul(line, nullptr, 16);
                unsigned long end = strtoul(dash + 1, nullptr, 16);
                for (unsigned long a = start; a < start + 16 && a < end; a++) {
                    volatile unsigned char c = *(volatile unsigned char*)a;
                    if (c == 0xCC) return true;
                }
            }
        }
        line = nl ? nl + 1 : nullptr;
    }
    return false;
}

} // anonymous namespace

namespace apex {
namespace protection {

void init_protection() {
    if (is_debugged()) {
        const char msg[] = "apex: debugger detected, exiting\n";
        write(2, msg, sizeof(msg) - 1);
        _exit(1);
    }
    if (is_injected()) {
        const char msg[] = "apex: code injection detected\n";
        write(2, msg, sizeof(msg) - 1);
    }
}

bool check_debugger() { return is_debugged(); }
bool check_injection() { return is_injected(); }
bool check_code_integrity() { return has_rwx_segments(); }
bool check_hook() { return is_hooked(); }
bool check_breakpoint() { return has_breakpoints(); }

bool verify_integrity() {
    if (is_debugged()) return false;
    if (is_injected()) return false;
    if (has_rwx_segments()) return false;
    return true;
}

} // namespace protection
} // namespace apex