#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

static int detect_apatch_ftruncate(char *detail, size_t ds) {
    bare_bind_cpu(0);
    int fd = bare_open("/dev/null", 2, 0);
    if (fd < 0) return 0;
    for (int i = 0; i < 500; i++) {
        ftruncate(fd, 0xffff); ftruncate(fd, 0x10000);
    }
    int iters = 20000;
    long long s = now_ns();
    for (int i = 0; i < iters; i++) ftruncate(fd, 0xffff);
    long long ts = (now_ns() - s) / iters;
    s = now_ns();
    for (int i = 0; i < iters; i++) ftruncate(fd, 0x10000);
    long long tl = (now_ns() - s) / iters;
    bare_close(fd);
    long long diff = tl - ts;
    if (diff < 0) diff = -diff;
    snprintf(detail + strlen(detail), ds - strlen(detail),
             "APATCH: ftruncate diff=%lldns (sm=%lld lg=%lld)\n", diff, ts, tl);
    if (diff > 800) return 15;
    if (diff > 400) return 8;
    return 0;
}

static int detect_apatch_files(char *detail, size_t ds) {
    const char *paths[] = {
        "/data/adb/apatch", "/data/adb/apatch.db",
        "/data/adb/modules/apatch", "/dev/apatch",
        "/dev/socket/apatched", "/data/data/com.dergoogler.mmrl",
        NULL
    };
    int score = 0;
    for (const char **p = paths; *p; p++) {
        if (bare_exists(*p)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "APATCH: %s\n", *p);
            score += 12;
        }
    }
    return score;
}

static int detect_apatch_kernel(char *detail, size_t ds) {
    int fd = bare_open("/proc/kallsyms", 0, 0);
    if (fd < 0) return 0;
    char buf[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    int score = 0;
    const char *sigs[] = {"apatch", "kp_", "kpatch", "kprobe", "kp_hook", NULL};
    for (const char **s = sigs; *s; s++) {
        if (strstr(buf, *s)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "APATCH: sym %s\n", *s);
            score += 8;
        }
    }
    /* kallsyms masking detection */
    int all_masked = 1;
    char first[16] = {0};
    char *line = buf;
    for (int i = 0; line && *line && i < 30; i++) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strlen(line) > 18) {
            if (first[0] == 0) { strncpy(first, line, 8); first[8] = '\0'; }
            else {
                char cur[16]; strncpy(cur, line, 8); cur[8] = '\0';
                if (strcmp(cur, first) != 0) { all_masked = 0; break; }
            }
        }
        line = nl ? (nl + 1) : NULL;
    }
    if (all_masked) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "APATCH: kallsyms masked\n");
        score += 10;
    }
    return score;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;
    score += detect_apatch_ftruncate(out->detail, sizeof(out->detail));
    score += detect_apatch_files(out->detail, sizeof(out->detail));
    score += detect_apatch_kernel(out->detail, sizeof(out->detail));
    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 25 ? 98U : score > 10 ? 85U : score > 3 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(10, "layer10_apatch", "1.0.0",
                "L10: APatch side-channel + file + kernel + kallsyms masking",
                init, detect, cleanup)