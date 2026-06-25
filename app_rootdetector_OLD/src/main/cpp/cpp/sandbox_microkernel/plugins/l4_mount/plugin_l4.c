#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

/* fork-based mount namespace comparison */
static int compare_mount_ns(char *detail, size_t ds) {
    char parent_m[98304];
    int fd = bare_open("/proc/self/mountinfo", 0, 0);
    if (fd < 0) return 0;
    ssize_t n = bare_read_full(fd, (uint8_t*)parent_m, sizeof(parent_m) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    parent_m[n] = '\0';

    int score = 0;

    /* fork child */
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "/data/local/tmp/.mnt_%llx",
             (unsigned long long)bare_rdtsc());

    pid_t pid = fork();
    if (pid == 0) {
        char child_m[98304];
        int cf = bare_open("/proc/self/mountinfo", 0, 0);
        ssize_t cn = bare_read_full(cf, (uint8_t*)child_m, sizeof(child_m) - 1);
        bare_close(cf);
        int wf = bare_open(tmp, 0x41, 0600);
        if (wf >= 0 && cn > 0)
            bare_write_full(wf, (uint8_t*)child_m, (size_t)cn);
        if (wf >= 0) bare_close(wf);
        _exit(0);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        char child_d[98304];
        int cf = bare_open(tmp, 0, 0);
        if (cf >= 0) {
            ssize_t cn = bare_read_full(cf, (uint8_t*)child_d, sizeof(child_d) - 1);
            bare_close(cf);
            unlink(tmp);
            if (cn > 0) {
                child_d[cn] = '\0';
                if (strcmp(parent_m, child_d) != 0) {
                    snprintf(detail + strlen(detail), ds - strlen(detail),
                             "L4-MNT: ns diff\n");
                    score += 10;
                }
            }
        }
    }

    /* Check for overlayfs on /system (KernelSU feature) */
    if (strstr(parent_m, "overlay") && strstr(parent_m, "/system")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L4-MNT: overlay on /system\n");
        score += 8;
    }

    /* Check for tmpfs on /data/adb (Magisk hiding) */
    if (strstr(parent_m, "tmpfs") && strstr(parent_m, "/data/adb")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L4-MNT: tmpfs on /data/adb\n");
        score += 6;
    }

    /* Check for tmpfs on /sbin (MagiskSU / KSU) */
    if (strstr(parent_m, "tmpfs") && strstr(parent_m, "/sbin")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L4-MNT: tmpfs on /sbin\n");
        score += 5;
    }

    return score;
}

/* compare /proc/1/mountinfo with ours to detect container/chroot */
static int check_root_mount_ns(char *detail, size_t ds) {
    char pid1_m[32768];
    int fd = bare_open("/proc/1/mountinfo", 0, 0);
    if (fd < 0) return 0;
    ssize_t n = bare_read_full(fd, (uint8_t*)pid1_m, sizeof(pid1_m) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    pid1_m[n] = '\0';

    char self_m[32768];
    fd = bare_open("/proc/self/mountinfo", 0, 0);
    if (fd < 0) return 0;
    n = bare_read_full(fd, (uint8_t*)self_m, sizeof(self_m) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    self_m[n] = '\0';

    if (strcmp(pid1_m, self_m) != 0) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L4-MNT: /proc/1 vs self mountinfo diff\n");
        return 15;  /* extremely suspicious = we're in a different namespace from PID1 */
    }
    return 0;
}

/* detect pivot_root or chroot by checking /proc/mounts root */
static int check_pivot_root(char *detail, size_t ds) {
    char mounts[32768];
    int fd = bare_open("/proc/mounts", 0, 0);
    if (fd < 0) return 0;
    ssize_t n = bare_read_full(fd, (uint8_t*)mounts, sizeof(mounts) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    mounts[n] = '\0';

    /* Check if rootfs is not the expected / or is tmpfs */
    char *first_line = mounts;
    char *nl = strchr(first_line, '\n');
    if (nl) *nl = '\0';
    if (strstr(first_line, "rootfs") && !strstr(first_line, " / ")) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L4-MNT: non-standard rootfs\n");
        return 10;
    }
    return 0;
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';
    int score = 0;

    score += compare_mount_ns(out->detail, sizeof(out->detail));
    score += check_root_mount_ns(out->detail, sizeof(out->detail));
    score += check_pivot_root(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 20 ? 95U : score > 10 ? 80U : score > 3 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_MOUNT, "l4_mount", "2.0.0",
                "L4: fork namespace comparison + overlayfs/tmpfs + PID1 comparison + pivot_root",
                init, detect, cleanup)