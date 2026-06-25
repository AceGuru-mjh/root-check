#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include <string.h>

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return 0;
}

/* scan /proc/kallsyms for suspicious symbols */
static int check_kallsyms(char *detail, size_t ds) {
    int fd = bare_open("/proc/kallsyms", 0, 0);
    if (fd < 0) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L6-KSYM: no access\n");
        return 2;  /* kallsyms restricted = possible KernelSU/APatch */
    }
    char buf[131072];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    int score = 0;

    const char *sigs[] = {
        "kallsyms_lookup_name", "ksu", "kernelsu",
        "apatch", "magisk", "su_hook", "sys_call_table",
        "kp_", "kpatch", "kprobe", "frida",
        NULL
    };
    for (const char **s = sigs; *s; s++) {
        if (strstr(buf, *s)) {
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L6-KSYM: %s\n", *s);
            if (strcmp(*s, "sys_call_table") == 0) score += 5;
            else if (strcmp(*s, "ksu") == 0 || strcmp(*s, "kernelsu") == 0) score += 8;
            else if (strcmp(*s, "apatch") == 0 || strcmp(*s, "kp_") == 0) score += 8;
            else score += 4;
        }
    }

    /* Check if all symbols start with same address prefix (kallsyms hidden) */
    int all_same_prefix = 1;
    char first_prefix[16] = {0};
    char *line = buf;
    int line_idx = 0;
    while (line && *line && line_idx < 20) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strlen(line) > 18) {
            char prefix[16];
            strncpy(prefix, line, 8);
            prefix[8] = '\0';
            if (first_prefix[0] == 0) strncpy(first_prefix, prefix, 8);
            else if (strcmp(prefix, first_prefix) != 0) {
                all_same_prefix = 0;
                break;
            }
        }
        line = nl ? (nl + 1) : NULL;
        line_idx++;
    }
    if (all_same_prefix && first_prefix[0]) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L6-KSYM: masked addresses (%s...)\n", first_prefix);
        score += 10;  /* APatch kallsyms masking */
    }

    return score;
}

/* scan /proc/modules for suspicious kernel modules */
static int check_modules(char *detail, size_t ds) {
    int fd = bare_open("/proc/modules", 0, 0);
    if (fd < 0) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L6-MOD: no access\n");
        return 2;
    }
    char buf[65536];
    ssize_t n = bare_read_full(fd, (uint8_t*)buf, sizeof(buf) - 1);
    bare_close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    int score = 0;
    const char *suspect_mods[] = {
        "magisk", "ksu", "kernelsu", "apatch",
        "shamiko", "zygisk", "frida", "frida_helper",
        "kpatch", "kp_module", "hidepid",
        "root_hide", "su_hook", NULL
    };
    for (const char **m = suspect_mods; *m; m++) {
        char *p = strstr(buf, *m);
        if (p) {
            int cl = 0;
            while (p[cl] && p[cl] != '\n' && cl < 120) cl++;
            snprintf(detail + strlen(detail), ds - strlen(detail),
                     "L6-MOD: %.*s\n", cl, p);
            score += 10;
        }
    }

    /* count total modules (clean system has 0-5, rooted has many) */
    int mod_count = 0;
    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strlen(line) > 3) mod_count++;
        line = nl ? (nl + 1) : NULL;
    }
    if (mod_count > 10) {
        snprintf(detail + strlen(detail), ds - strlen(detail),
                 "L6-MOD: %d modules loaded\n", mod_count);
        score += (mod_count - 10) * 2;
    }

    return score;
}

/* check /proc/sysrq-trigger and other security-sensitive /proc entries */
static int check_proc_security(char *detail, size_t ds) {
    int score = 0;

    const char *checks[] = {
        "/proc/sysrq-trigger", NULL
    };
    for (const char **c = checks; *c; c++) {
        if (bare_exists(*c)) {
            int fd = bare_open(*c, 2, 0);  /* O_WRONLY */
            if (fd >= 0) {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L6-PROC: %s writable\n", *c);
                score += 5;
                bare_close(fd);
            }
        }
    }

    /* check if kptr_restrict is 0 (kernel addresses visible) */
    int rfd = bare_open("/proc/sys/kernel/kptr_restrict", 0, 0);
    if (rfd >= 0) {
        char val[8];
        ssize_t vn = bare_read_full(rfd, (uint8_t*)val, sizeof(val) - 1);
        bare_close(rfd);
        if (vn > 0) {
            val[vn] = '\0';
            if (val[0] == '0') {
                snprintf(detail + strlen(detail), ds - strlen(detail),
                         "L6-PROC: kptr_restrict=0\n");
                score += 3;
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

    score += check_kallsyms(out->detail, sizeof(out->detail));
    score += check_modules(out->detail, sizeof(out->detail));
    score += check_proc_security(out->detail, sizeof(out->detail));

    uint64_t t1 = bare_rdtsc();
    out->detected   = score > 0 ? 1 : 0;
    out->confidence = score > 20 ? 95U : score > 10 ? 80U : score > 3 ? 60U : 0U;
    out->risk_score = score > 100 ? 100U : (uint32_t)score;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_KERNEL, "l6_kernel", "2.0.0",
                "L6: kallsyms scanning + module count + masked addr detection + proc security",
                init, detect, cleanup)