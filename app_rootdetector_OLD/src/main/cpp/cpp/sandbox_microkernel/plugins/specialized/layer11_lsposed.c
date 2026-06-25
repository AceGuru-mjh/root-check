#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static int detect_lsposed_memory(char *detail, size_t ds) {
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char maps[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)maps, sizeof(maps) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    maps[n] = '\0';
    int score = 0;
    const char *sigs[] = {
        "liblspd.so", "libxposed", "xposed",
        "lspd", "LSPosed", "de.robv.android.xposed",
        NULL
    };
    for (const char **s = sigs; *s; s++) {
        const char *p = maps;
        while ((p = strstr(p, *s)) != NULL) {
            int cl = 0;
            while (p[cl] && p[cl] != '\n' && cl < 120) cl++;
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "LSPOSED: memory %.*s\n", cl, p);
            score += 15;
            p++;
        }
    }
    /* non-standard dex2oat (LSPosed modifies compilation) */
    if (strstr(maps, "dex2oat") && strstr(maps, "/data/data/")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "LSPOSED: dex2oat in data\n");
        score += 8;
    }
    return score;
}

static int detect_lsposed_files(char *detail, size_t ds) {
    const char *paths[] = {
        "/data/adb/lspd", "/data/local/tmp/lspd",
        "/data/adb/modules/lsposed",
        "/dev/socket/lspd", "/data/misc/lspd",
        NULL
    };
    int score = 0;
    for (const char **p = paths; *p; p++) {
        if (bare_exists(*p)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "LSPOSED: file %s\n", *p);
            score += 12;
        }
    }
    return score;
}

static int detect_lsposed_process(char *detail, size_t ds) {
    int pd = bare_open("/proc", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (pd < 0) return 0;
    char buf[16384];
    long n = bare_getdents64(pd, buf, sizeof(buf));
    bare_close(pd);
    if (n <= 0) return 0;
    int score = 0;
    long pos = 0;
    while (pos < n) {
        struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
        if (d->d_type == 10) {
            long pid = atol(d->d_name);
            if (pid > 0) {
                char mpath[64];
                snprintf(mpath, sizeof(mpath), "/proc/%ld/maps", pid);
                int mf = bare_open(mpath, 0, 0);
                if (mf >= 0) {
                    char m[4096];
                    ssize_t mn = bare_read_full(mf, (uint8_t*)m, sizeof(m) - 1);
                    bare_close(mf);
                    if (mn > 0) {
                        m[mn] = '\0';
                        if (strstr(m, "liblspd.so")) {
                            snprintf(detail + strlen(detail), ds - strlen(detail),
                                     "LSPOSED: pid %ld injected\n", pid);
                            score += 15;
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
    score += detect_lsposed_memory(out->detail, sizeof(out->detail));
    score += detect_lsposed_files(out->detail, sizeof(out->detail));
    score += detect_lsposed_process(out->detail, sizeof(out->detail));
    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 30 ? 98U : score > 15 ? 85U : score > 5 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(11, "layer11_lsposed", "1.0.0",
                "L11: LSPosed memory + file + cross-process injection detection",
                init, detect, cleanup)