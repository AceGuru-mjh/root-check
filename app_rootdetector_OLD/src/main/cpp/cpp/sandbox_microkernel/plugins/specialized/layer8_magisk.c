#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

static int detect_magisk_socket(char *detail, size_t ds) {
    const char *socks[] = {
        "/dev/socket/magiskd",     /* Magisk daemon socket */
        "/dev/socket/zygisk",      /* Zygisk socket */
        "/data/adb/magisk",        /* Module directory marker */
        "/data/adb/zygisk",        /* Zygisk directory */
        "/data/adb/magisk.db",     /* Magisk database */
        "/data/adb/modules",       /* Modules directory */
        "/data/adb/post-fs-data.d",
        "/data/adb/service.d",
        "/cache/magisk.log",
        NULL
    };
    int score = 0;
    for (const char **s = socks; *s; s++) {
        if (bare_exists(*s)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "MAGISK: %s\n", *s);
            score += 10;
        }
    }
    return score;
}

static int detect_magisk_process(char *detail, size_t ds) {
    int fd = bare_open("/proc", BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (fd < 0) return 0;
    char buf[32768];
    long n = bare_getdents64(fd, buf, sizeof(buf));
    bare_close(fd);
    if (n <= 0) return 0;

    int score = 0;
    long pos = 0;
    while (pos < n) {
        struct bare_dirent64 *d = (struct bare_dirent64 *)(buf + pos);
        if (d->d_type == 10) {  /* DT_LNK = process */
            long pid = atol(d->d_name);
            if (pid > 0) {
                char path[128];
                snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
                int cf = bare_open(path, 0, 0);
                if (cf >= 0) {
                    char cmd[256];
                    ssize_t cn = bare_read_full(cf, (uint8_t*)cmd, sizeof(cmd) - 1);
                    bare_close(cf);
                    if (cn > 0) {
                        cmd[cn] = '\0';
                        if (strstr(cmd, "magiskd") || strstr(cmd, "magisk")) {
                            snprintf(detail + strlen(detail), ds - strlen(detail),
                                     "MAGISK: process %ld %s\n", pid, cmd);
                            score += 15;
                        }
                        if (strstr(cmd, "zygisk") || strstr(cmd, "zygote")) {
                            /* Zygisk injects into zygote, check maps */
                            char mpath[128];
                            snprintf(mpath, sizeof(mpath), "/proc/%ld/maps", pid);
                            int mf = bare_open(mpath, 0, 0);
                            if (mf >= 0) {
                                char m[4096];
                                ssize_t mn = bare_read_full(mf, (uint8_t*)m, sizeof(m) - 1);
                                bare_close(mf);
                                if (mn > 0) {
                                    m[mn] = '\0';
                                    if (strstr(m, "libzygisk.so") ||
                                        strstr(m, "liblspd.so")) {
                                        snprintf(detail + strlen(detail), ds - strlen(detail),
                                                 "MAGISK: zygote injected %ld\n", pid);
                                        score += 20;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        pos += d->d_reclen;
    }
    return score;
}

static int detect_magisk_boot(char *detail, size_t ds) {
    const char *boot_devs[] = {
        "/dev/block/by-name/boot", "/dev/block/by-name/boot_a",
        NULL
    };
    int fd = -1;
    for (const char **b = boot_devs; *b; b++) {
        fd = (int)bare_open(*b, 0, 0);
        if (fd >= 0) break;
    }
    if (fd < 0) return 0;
    uint8_t buf[131072];
    ssize_t n = bare_read_full(fd, buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    const char *ms[] = {"MAGISK","magisk","init.magisk","ramdisk.cpio",NULL};
    int score = 0;
    for (const char **m = ms; *m; m++) {
        if (memmem(buf, (size_t)n, *m, strlen(*m))) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "MAGISK: boot sig %s\n", *m);
            score += 25;
        }
    }
    return score;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;
    score += detect_magisk_socket(out->detail, sizeof(out->detail));
    score += detect_magisk_process(out->detail, sizeof(out->detail));
    score += detect_magisk_boot(out->detail, sizeof(out->detail));
    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 40 ? 98U : score > 20 ? 85U : score > 5 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(8, "layer8_magisk", "1.0.0",
                "L8: Magisk/Zygisk socket + process + boot partition detection",
                init, detect, cleanup)