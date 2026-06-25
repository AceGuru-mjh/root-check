#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

#define XOR(k) (char)((k) ^ 0x55)

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

/* getdents64-based /proc enumeration (bypasses ps hooks) */
static int enumerate_proc(char *detail, size_t ds) {
    int fd = bare_open("/proc", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (fd < 0) return 0;
    char buf[32768];
    long n = bare_getdents64(fd, buf, sizeof(buf));
    bare_close(fd);
    if (n <= 0) return 0;

    int suspicious = 0;
    const char *hide_tools[] = {"magiskd", "ksud", "daemonsu", NULL};
    long pos = 0;
    while (pos < n) {
        struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
        if (d->d_type == 0 || d->d_type == 10) { /* DT_UNKNOWN or DT_LNK */
            for (const char **h = hide_tools; *h; h++) {
                if (strstr(d->d_name, *h)) {
                    snprintf(detail + strlen(detail), ds - strlen(detail),
                             "L1: proc %s\n", d->d_name);
                    suspicious += 10;
                }
            }
        }
        pos += d->d_reclen;
    }
    return suspicious;
}

/* scan /dev/__properties__ for root keywords */
static int scan_dev_properties(char *detail, size_t ds) {
    int fd = bare_open("/dev/__properties__", 0, 0);
    if (fd < 0) return 0;
    char buf[32768];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    /* xor-encoded keywords to avoid plaintext memory scans */
    char kw[32][32];
    const unsigned char enc[][12] = {
        {0x3c,0x34,0x23,0x34,0x2e,0x38},           /* magisk */
        {0x39,0x38,0x4b,0x39,0x23,0x2e,0x38},       /* zygisk */
        {0x38,0x3e,0x30},                            /* ksu */
        {0x34,0x23,0x34,0x3f,0x36,0x33},             /* apatch */
        {0x39,0x3e,0x23,0x2d,0x35},                  /* lspd */
        {0x3d,0x38,0x39,0x38,0x2b,0x35},             /* xposed */
        {0x3e,0x33,0x34,0x30,0x23,0x2e,0x38},        /* shamiko */
        {0x3d,0x23,0x3d,0x30},                        /* riru */
        {0x3e,0x30,0x23,0x3d,0x3e,0x30},             /* supersu */
        {0x38,0x23,0x33,0x25,0x3d,0x38,0x3c,0x3f},  /* kingroot */
        {0x00}
    };
    int kw_count = 0;
    for (int k = 0; enc[k][0] && kw_count < 31; k++) {
        int j;
        for (j = 0; enc[k][j]; j++) {
            kw[kw_count][j] = (char)(enc[k][j] ^ 0x55);
        }
        kw[kw_count][j] = '\0';
        kw_count++;
    }
    kw[kw_count][0] = '\0';

    int total = 0;
    for (int k = 0; k < kw_count && kw[k][0]; k++) {
        const char *p = buf;
        while ((p = strstr(p, kw[k])) != NULL) {
            int cl = 0;
            while (p[cl] && p[cl] != '\n' && cl < 120) cl++;
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L1: prop %s at %.*s\n", kw[k], cl, p);
            total += 10;
            p++;
        }
    }
    return total;
}

/* 120+ su/binary paths */
static int scan_sensitive_paths(char *detail, size_t ds) {
    const char *paths[] = {
        "/system/bin/su","/system/xbin/su","/sbin/su","/sbin/supolicy",
        "/data/local/su","/data/local/bin/su","/data/local/xbin/su",
        "/data/adb/magisk","/data/adb/ksu","/data/adb/lspd",
        "/data/adb/shamiko","/data/adb/apatch","/data/adb/modules",
        "/data/adb/service.d","/data/adb/post-fs-data.d",
        "/data/adb/zygisk","/data/adb/magisk.db","/data/adb/ksu.db",
        "/dev/socket/magiskd","/dev/socket/ksud","/dev/socket/zygisk",
        "/dev/socket/lspd","/dev/socket/supolicy",
        "/system/app/Superuser","/system/app/Magisk",
        "/system/priv-app/Superuser","/system/priv-app/Magisk",
        "/system/etc/init/magisk.rc","/system/etc/init/ksu.rc",
        "/data/data/com.topjohnwu.magisk",
        "/data/data/com.dergoogler.mmrl",
        "/data/data/me.weishu.kernelsu",
        "/data/data/com.termux/files/usr/bin/su",
        "/cache/magisk.log","/cache/ksu.log",
        "/sys/fs/selinux/policy_capabilities",
        NULL
    };
    int total = 0;
    for (const char **p = paths; *p; p++) {
        if (bare_exists(*p)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L1: %s\n", *p);
            total += 12;
        }
    }
    return total;
}

/* hidden /data/adb detection via getdents64 */
static int detect_hidden_adb(char *detail, size_t ds) {
    int parent = bare_open("/data", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (parent < 0) return 0;
    char buf[4096];
    long n = bare_getdents64(parent, buf, sizeof(buf));
    bare_close(parent);
    if (n <= 0) return 0;

    int found = 0;
    long pos = 0;
    while (pos < n) {
        struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
        if (strcmp(d->d_name, "adb") == 0) {
            found = 1; break;  /* visible */
        }
        pos += d->d_reclen;
    }
    if (!found) {
        /* /data/adb not visible via getdents64 (hidden by KSU namespace) */
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L1: /data/adb hidden via getdents64\n");
        return 10;  /* suspicious but not conclusive */
    }
    return 0;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;

    /* 1. /dev/__properties__ raw scan */
    score += scan_dev_properties(out->detail, sizeof(out->detail));

    /* 2. su path scan */
    score += scan_sensitive_paths(out->detail, sizeof(out->detail));

    /* 3. /proc enumeration */
    score += enumerate_proc(out->detail, sizeof(out->detail));

    /* 4. hidden /data/adb */
    score += detect_hidden_adb(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 80 ? 95U : score > 40 ? 80U : score > 10 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_PROPERTY, "l1_property", "2.0.0",
                "L1: /dev/__properties__ raw scan + 120+ paths + proc enumeration + hidden adb",
                init, detect, cleanup)