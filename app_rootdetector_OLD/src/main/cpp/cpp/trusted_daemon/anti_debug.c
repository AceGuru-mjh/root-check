#include "anti_debug.h"
#include "../bare_syscall/bare_syscall.h"
#include <string.h>

/* SYS_getppid */
#define __NR_getppid 110

static int g_ptrace_attached = 0;

void anti_debug_init(void) {
    if (bare_syscall(101, 0, 1, 0) == -1) {
        g_ptrace_attached = 1;
    }
    bare_syscall(141, 0, 0, -20);
}

int anti_debug_check_tracer(void) {
    char buf[512];
    int fd = bare_open("/proc/self/status", 0, 0);
    if (fd < 0) return 0;
    ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    buf[len] = '\0';

    const char *t = strstr(buf, "TracerPid:");
    if (t) {
        int pid = 0;
        const char *p = t + 10;
        while (*p == ' ' || *p == '\t') p++;
        while (*p >= '0' && *p <= '9') {
            pid = pid * 10 + (*p - '0');
            p++;
        }
        if (pid != 0) return 1;
    }

    const char *sec = strstr(buf, "Seccomp:");
    if (sec) {
        int mode = 0;
        const char *p = sec + 8;
        while (*p == ' ' || *p == '\t') p++;
        while (*p >= '0' && *p <= '9') {
            mode = mode * 10 + (*p - '0');
            p++;
        }
        if (mode != 2) return -1;
    }

    return 0;
}

int anti_debug_check_port(void) {
    int fd = bare_open("/proc/net/tcp", 0, 0);
    if (fd < 0) return 0;
    char buf[4096];
    ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    buf[len] = '\0';

    const char *danger_ports[] = {"1A89", "1F40", "0D3A", "1F41", NULL};
    for (const char **port = danger_ports; *port; port++) {
        if (strstr(buf, *port)) return 1;
    }
    return 0;
}

int anti_debug_integrity(void) {
    return anti_debug_check_tracer() || anti_debug_check_port();
}
