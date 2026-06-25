#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static int detect_ksu_overlay(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/mountinfo", 0, 0);
    if (fd < 0) return 0;
    char buf[65536];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    int score = 0;
    if (strstr(buf, "overlay") && strstr(buf, "/system")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "KSU: overlay on /system\n");
        score += 10;
    }
    if (strstr(buf, "overlay") && strstr(buf, "/vendor")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "KSU: overlay on /vendor\n");
        score += 8;
    }
    if (strstr(buf, "overlay") && strstr(buf, "/product")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "KSU: overlay on /product\n");
        score += 6;
    }
    /* KernelSU creates tmpfs on /data/adb */
    if (strstr(buf, "tmpfs") && strstr(buf, "/data/adb")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "KSU: tmpfs on /data/adb\n");
        score += 8;
    }
    return score;
}

static int detect_ksu_files(char *detail, size_t ds) {
    const char *paths[] = {
        "/data/adb/ksu", "/data/adb/ksu.db",
        "/dev/ksu", "/dev/socket/ksud",
        "/dev/ksud", "/sys/fs/selinux/ksu",
        "/proc/ksu", "/data/data/me.weishu.kernelsu",
        NULL
    };
    int score = 0;
    for (const char **p = paths; *p; p++) {
        if (bare_exists(*p)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "KSU: file %s\n", *p);
            score += 12;
        }
    }
    return score;
}

static int detect_ksu_kernel(char *detail, size_t ds) {
    int fd = bare_open("/proc/kallsyms", 0, 0);
    if (fd < 0) return 0;
    char buf[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    int score = 0;
    const char *sigs[] = {"ksu", "kernelsu", "kernelsu_user",
                          "kernelsu_system", "kernelsu_kern", NULL};
    for (const char **s = sigs; *s; s++) {
        if (strstr(buf, *s)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "KSU: kernel sym %s\n", *s);
            score += 10;
        }
    }
    return score;
}

static int detect_ksu_module(char *detail, size_t ds) {
    int fd = bare_open("/proc/modules", 0, 0);
    if (fd < 0) return 0;
    char buf[32768];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    int score = 0;
    if (strstr(buf, "kernelsu") || strstr(buf, "ksu")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "KSU: kernel module\n");
        score += 15;
    }
    return score;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;
    score += detect_ksu_overlay(out->detail, sizeof(out->detail));
    score += detect_ksu_files(out->detail, sizeof(out->detail));
    score += detect_ksu_kernel(out->detail, sizeof(out->detail));
    score += detect_ksu_module(out->detail, sizeof(out->detail));
    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 30 ? 98U : score > 15 ? 85U : score > 5 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(9, "layer9_ksu", "1.0.0",
                "L9: KernelSU overlay + file + kernel symbol + module detection",
                init, detect, cleanup)