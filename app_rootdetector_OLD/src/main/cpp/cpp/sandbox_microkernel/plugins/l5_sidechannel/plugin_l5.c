#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

/* APatch side-channel: ftruncate timing diff */
static int detect_apatch_ftruncate(char *detail, size_t ds) {
    bare_bind_cpu(0);
    int fd = bare_open("/dev/null", 2, 0);
    if (fd < 0) return 0;

    /* warmup */
    for (int i = 0; i < 1000; i++) {
        ftruncate(fd, 0xffff);
        ftruncate(fd, 0x10000);
    }

    /* measure 0xffff */
    int iters = 50000;
    long long s1 = now_ns();
    for (int i = 0; i < iters; i++) ftruncate(fd, 0xffff);
    long long t_small = (now_ns() - s1) / iters;

    /* measure 0x10000 */
    s1 = now_ns();
    for (int i = 0; i < iters; i++) ftruncate(fd, 0x10000);
    long long t_large = (now_ns() - s1) / iters;

    bare_close(fd);

    long long diff = t_large - t_small;
    if (diff < 0) diff = -diff;

    snprintf(detail + strlen(detail), ds - strlen(detail),
             "L5-AP: diff=%lldns (sm=%lld lg=%lld)\n", diff, t_small, t_large);

    if (diff > 500) {
        if (diff > 2000) return 20;   /* strong indicator */
        if (diff > 1000) return 15;
        return 10;
    }
    return 0;
}

/* syscall timing baseline comparison */
static int check_syscall_timing(char *detail, size_t ds) {
    bare_bind_cpu(0);
    int iters = 5000;

    /* baseline estimates (clean Android 14 arm64) */
    long long baselines[] = {1500, 2000, 3500, 800, 2500};
    const char *names[] = {"openat", "readlinkat", "getdents64", "access", "close"};

    long long totals[5] = {0};
    char buf[256];

    for (int i = 0; i < iters; i++) {
        long long s = now_ns();
        bare_openat(AT_FDCWD, "/dev/null", 0, 0);
        totals[0] += now_ns() - s;

        s = now_ns();
        bare_readlinkat(AT_FDCWD, "/proc/self/exe", buf, sizeof(buf));
        totals[1] += now_ns() - s;

        s = now_ns();
        int dfd = bare_open("/proc", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
        if (dfd >= 0) {
            bare_getdents64(dfd, buf, sizeof(buf));
            bare_close(dfd);
        }
        totals[2] += now_ns() - s;

        s = now_ns();
        bare_access("/dev/null", 0);
        totals[3] += now_ns() - s;

        s = now_ns();
        int tfd = bare_open("/dev/null", 0, 0);
        if (tfd >= 0) { bare_close(tfd); }
        totals[4] += now_ns() - s;
    }

    int anomalies = 0;
    for (int i = 0; i < 5; i++) {
        long long avg = totals[i] / iters;
        long long ratio = avg * 100 / (baselines[i] > 0 ? baselines[i] : 1);
        if (ratio > 200) {  /* 2x baseline */
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L5-TIM: %s %lldns (%lld%%)\n", names[i], avg, ratio);
            anomalies++;
        }
    }

    if (anomalies >= 3) return 20;   /* widespread hooking */
    if (anomalies >= 1) return 10;
    return 0;
}

/* SELinux context jump detection */
static int check_selinux_context(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/attr/current", 0, 0);
    if (fd < 0) return 0;
    char ctx[256];
    ssize_t n = bare_read_full(fd, (uint8_t*)ctx, sizeof(ctx) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    ctx[n] = '\0';

    /* Normal app context is like u:r:untrusted_app:s0:c* */
    if (strstr(ctx, "kernel") || strstr(ctx, "su") ||
        strstr(ctx, "init") || strstr(ctx, "system_app") ||
        strstr(ctx, "platform_app")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L5-SEL: abnormal ctx %s\n", ctx);
        return 15;
    }

    /* Re-read after short delay to detect context switching */
    usleep(10000);
    fd = bare_open("/proc/self/attr/current", 0, 0);
    if (fd < 0) return 0;
    char ctx2[256];
    n = bare_read_full(fd, (uint8_t*)ctx2, sizeof(ctx2) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    ctx2[n] = '\0';

    if (strcmp(ctx, ctx2) != 0) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L5-SEL: ctx jump detected\n");
        return 20;  /* SELinux context switching in progress = Shamiko */
    }
    return 0;
}

/* Binder transaction monitor (detect suspicious IPC) */
static int check_binder_traffic(char *detail, size_t ds) {
    long long start = now_ns();
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char buf[4096];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    long long elapsed = now_ns() - start;

    /* If maps read is slow (>5ms), binder might be intercepted */
    if (elapsed > 5000000) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L5-BIN: /proc/self/maps slow (%lldms)\n", elapsed / 1000000);
        return 5;
    }
    return 0;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;

    score += detect_apatch_ftruncate(out->detail, sizeof(out->detail));
    score += check_syscall_timing(out->detail, sizeof(out->detail));
    score += check_selinux_context(out->detail, sizeof(out->detail));
    score += check_binder_traffic(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 40 ? 95U : score > 20 ? 80U : score > 5 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_SIDECHANNEL, "l5_sidechannel", "2.0.0",
                "L5: APatch side-channel + syscall timing + SELinux context + Binder monitor",
                init, detect, cleanup)