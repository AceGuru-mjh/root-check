#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static int detect_shamiko_selinux(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/attr/current", 0, 0);
    if (fd < 0) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "SHAMIKO: no selinux attr\n");
        return 5;
    }
    char ctx[256];
    ssize_t n = bare_read_full(fd, (uint8_t*)ctx, sizeof(ctx) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    ctx[n] = '\0';

    int score = 0;
    /* Shamiko puts app into permissive or switches context */
    if (strstr(ctx, "kernel") || strstr(ctx, "init") ||
        strstr(ctx, "su") || strstr(ctx, "system")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "SHAMIKO: selinux %s\n", ctx);
        score += 12;
    }
    /* Check selinux enforcing state */
    fd = bare_open("/sys/fs/selinux/enforce", 0, 0);
    if (fd >= 0) {
        char val[8];
        ssize_t vn = bare_read_full(fd, (uint8_t*)val, sizeof(val) - 1);
        bare_close(fd);
        if (vn > 0) {
            val[vn] = '\0';
            if (val[0] == '0') {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "SHAMIKO: selinux permissive\n");
                score += 15;
            }
        }
    }
    /* Context jump detection (Shamiko switches ctx dynamically) */
    usleep(20000);
    fd = bare_open("/proc/self/attr/current", 0, 0);
    if (fd >= 0) {
        char ctx2[256];
        n = bare_read_full(fd, (uint8_t*)ctx2, sizeof(ctx2) - 1);
        bare_close(fd);
        if (n > 0) {
            ctx2[n] = '\0';
            if (strcmp(ctx, ctx2) != 0) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "SHAMIKO: ctx jump\n");
                score += 20;
            }
        }
    }
    return score;
}

static int detect_shamiko_hide(char *detail, size_t ds) {
    int score = 0;
    /* Shamiko hides files by mount namespace manipulation */
    int fd = bare_open("/data/adb", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (fd >= 0) {
        char buf[4096];
        long n = bare_getdents64(fd, buf, sizeof(buf));
        bare_close(fd);
        if (n > 0) {
            int has_modules = 0;
            long pos = 0;
            while (pos < n) {
                struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
                if (strcmp(d->d_name, "modules") == 0) has_modules = 1;
                pos += d->d_reclen;
            }
            if (!has_modules) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "SHAMIKO: /data/adb hidden\n");
                score += 8;
            }
        }
    }
    /* Check if /data/adb/modules exists on disk but not in our NS */
    if (bare_exists("/data/adb/modules")) {
        /* Can see it directly - Shamiko may not be active */
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "SHAMIKO: /data/adb/modules visible\n");
        score += 2;
    }
    return score;
}

static int detect_shamiko_process_scan(char *detail, size_t ds) {
    int fd = bare_open("/proc", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (fd < 0) return 0;
    char buf[16384];
    long n = bare_getdents64(fd, buf, sizeof(buf));
    bare_close(fd);
    if (n <= 0) return 0;
    int score = 0;
    long pos = 0;
    while (pos < n) {
        struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
        if (d->d_type == 10) {
            long pid = atol(d->d_name);
            if (pid > 0 && pid != getpid()) {
                char ppath[64];
                snprintf(ppath, sizeof(ppath), "/proc/%ld/cmdline", pid);
                int cf = bare_open(ppath, 0, 0);
                if (cf >= 0) {
                    char cmd[256];
                    ssize_t cn = bare_read_full(cf, (uint8_t*)cmd, sizeof(cmd) - 1);
                    bare_close(cf);
                    if (cn > 0) {
                        cmd[cn] = '\0';
                        if (strcmp(cmd, "shamiko") == 0 ||
                            strstr(cmd, "shamiko")) {
                            snprintf(detail + strlen(detail), ds - strlen(detail),
                                     "SHAMIKO: process pid %ld\n", pid);
                            score += 25;
                        }
                    }
                }
            }
        }
        pos += d->d_reclen;
    }
    return score;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;
    score += detect_shamiko_selinux(out->detail, sizeof(out->detail));
    score += detect_shamiko_hide(out->detail, sizeof(out->detail));
    score += detect_shamiko_process_scan(out->detail, sizeof(out->detail));
    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 35 ? 98U : score > 15 ? 85U : score > 5 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(12, "layer12_shamiko", "1.0.0",
                "L12: Shamiko SELinux context + hide detection + process scan",
                init, detect, cleanup)