#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static int scan_memory_maps(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char maps[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)maps, sizeof(maps) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    maps[n] = '\0';

    int score = 0;

    /* root-tool shared libraries in memory */
    const char *root_libs[] = {
        "libmagisk.so", "libzygisk.so", "liblspd.so", "libshamiko.so",
        "libkernelsu.so", "libapatch.so", "libriru.so", "libxposed.so",
        "libfrida-gadget.so", "libfrida-core.so", "libgum.so",
        "libinjector.so", "libwhale.so", "libdobby.so",
        NULL
    };
    for (const char **l = root_libs; *l; l++) {
        if (strstr(maps, *l)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L3-MEM: %s\n", *l);
            score += 15;
        }
    }

    /* anonymous executable mappings (memfd, heap spray) */
    int anon_exec = 0;
    char *line = maps;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strstr(line, "r-xp")) {
            if (strstr(line, "/memfd:")) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L3-MEM: exec memfd\n");
                anon_exec++; score += 10;
            }
            if (strstr(line, "/dev/ashmem")) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L3-MEM: exec ashmem\n");
                anon_exec++; score += 8;
            }
            if (line[0] == '7' && !strstr(line, "/")) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L3-MEM: anon exec\n");
                anon_exec++; score += 5;
            }
        }
        line = nl ? (nl + 1) : NULL;
    }

    /* rwxp count (potential injected mapping) */
    int rwxp = 0;
    line = maps;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strstr(line, "rwxp") || strstr(line, "rwxs")) rwxp++;
        line = nl ? (nl + 1) : NULL;
    }
    if (rwxp > 2) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L3-MEM: %d RWX mappings\n", rwxp);
        score += rwxp * 3;
    }

    /* check if native bridge is loaded (for x86 emulation, often used by hook tools) */
    if (strstr(maps, "libhoudini") || strstr(maps, "libndk_translation")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L3-MEM: native bridge loaded\n");
        score += 2;
    }

    return score;
}

/* scan /proc/self/smaps for dirty shared pages (injected code indicator) */
static int scan_smaps_anomaly(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/smaps", 0, 0);
    if (fd < 0) return 0;
    char buf[65536];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    int huge_pss = 0;
    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strstr(line, "Pss:")) {
            unsigned long val = strtoul(line + 5, NULL, 10);
            if (val > 50000) huge_pss++;  /* >50MB region is suspicious */
        }
        line = nl ? (nl + 1) : NULL;
    }
    if (huge_pss > 2) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L3-MEM: %d regions >50MB PSS\n", huge_pss);
        return huge_pss * 2;
    }
    return 0;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;

    score += scan_memory_maps(out->detail, sizeof(out->detail));
    score += scan_smaps_anomaly(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 60 ? 95U : score > 30 ? 80U : score > 10 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_MEMORY, "l3_memory", "2.0.0",
                "L3: memory fingerprint (root libs) + RWX/RWXP mapping + smaps analysis",
                init, detect, cleanup)