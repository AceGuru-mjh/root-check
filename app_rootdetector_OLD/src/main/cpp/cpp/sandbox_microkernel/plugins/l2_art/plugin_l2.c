#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

/* scan /proc/self/maps for ART anomalies */
static int scan_maps_for_art(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char maps[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)maps, sizeof(maps) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    maps[n] = '\0';

    int score = 0;

    /* Zygisk / LSPosed / Riru injection signatures */
    const char *inj[] = {
        "liblspd.so", "libriru.so", "libxposed",
        "libzygisk.so", "libmagisk.so", "libshamiko.so",
        "libkernelsu.so", "libapatch.so",
        NULL
    };
    for (const char **s = inj; *s; s++) {
        const char *p = maps;
        while ((p = strstr(p, *s)) != NULL) {
            int cl = 0;
            while (p[cl] && p[cl] != '\n' && cl < 120) cl++;
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L2-ART: injection %.*s\n", cl, p);
            score += 15;
            p++;
        }
    }

    /* dex2oat / /data/dalvik-cache anomalies */
    const char *dex_sig[] = {"dex2oat", "/data/dalvik-cache", NULL};
    for (const char **s = dex_sig; *s; s++) {
        if (strstr(maps, *s)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L2-ART: dex anomaly %s\n", *s);
            score += 5;
        }
    }

    /* /data/local/tmp mapped as executable (dex injection) */
    if (strstr(maps, "/data/local/tmp") && strstr(maps, "r-xp")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L2-ART: /data/local/tmp exec mapping\n");
        score += 20;
    }

    /* Non-standard ART vs AOSP */
    /* AOSP ART maps to /system/framework/arm64/boot* */
    int has_aosp_art = (strstr(maps, "/system/framework/") != NULL);
    int has_vendor_art = (strstr(maps, "/vendor/framework/") != NULL);
    if (!has_aosp_art && !has_vendor_art) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L2-ART: no standard framework mapping\n");
        score += 3;
    }

    /* /memfd: (anonymous executable memory) */
    if (strstr(maps, "r-xp") && strstr(maps, "/memfd:")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L2-ART: anonymous exec memfd\n");
        score += 15;
    }

    /* detect inline hook by counting suspicious RWX regions */
    int rwx_count = 0;
    char *line = maps;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strstr(line, "rwxp") || strstr(line, "rwx ")) {
            rwx_count++;
        }
        line = nl ? (nl + 1) : NULL;
    }
    if (rwx_count > 3) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L2-ART: %d RWX regions (possible hook)\n", rwx_count);
        score += rwx_count * 2;
    }

    return score;
}

/* detect Frida by scanning pipe names and /proc/self/fd */
static int detect_frida_specific(char *detail, size_t ds) {
    int score = 0;
    char fd_path[64];
    for (int i = 0; i < 256; i++) {
        snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", i);
        char link[256];
        long ret = bare_readlinkat(BARE_AT_FDCWD, fd_path, link, sizeof(link));
        if (ret > 0) {
            link[ret] = '\0';
            if (strstr(link, "linjector") || strstr(link, "frida") ||
                strstr(link, "gum-js-loop")) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L2-ART: frida fd %s\n", link);
                score += 25;
            }
        }
    }
    return score;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;

    score += scan_maps_for_art(out->detail, sizeof(out->detail));
    score += detect_frida_specific(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 60 ? 95U : score > 30 ? 80U : score > 10 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_ART, "l2_art", "2.0.0",
                "L2: ART injection detection (Zygisk/LSPosed/Riru) + Frida + RWX regions",
                init, detect, cleanup)