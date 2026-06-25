#include "kernel_probe.h"
#include "../bare_syscall/bare_syscall.h"
#include "../bare_syscall/cpu_control.h"
#include "../crypto/core/crypto_core.h"
#include "../sandbox_microkernel/plugin_interface.h"
#include <string.h>

static uint64_t g_last_kallsyms_hash = 0;
static uint64_t g_kallsyms_baseline[64];
static int g_probe_initialized = 0;
static int g_monitor_cycle = 0;

static uint64_t hash_buf(const char *buf, size_t len) {
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)(unsigned char)buf[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

int kernel_probe_init(void) {
    g_probe_initialized = 1;
    g_last_kallsyms_hash = 0;
    g_monitor_cycle = 0;
    return 0;
}

int kernel_probe_check(void *result) {
    plugin_result_t *r = (plugin_result_t *)result;
    int fd = bare_open("/proc/kallsyms", 0, 0);
    if (fd < 0) {
        strcat(r->detail, "DAEMON: kallsyms inaccessible (hidden/masked)\n");
        return 1;
    }
    char buf[65536];
    ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    buf[len] = '\0';

    uint64_t current_hash = hash_buf(buf, (size_t)len);
    if (g_last_kallsyms_hash != 0 && current_hash != g_last_kallsyms_hash) {
        snprintf(r->detail + strlen(r->detail), 256,
                 "DAEMON: kallsyms changed! hash=%llx was=%llx\n",
                 (unsigned long long)current_hash,
                 (unsigned long long)g_last_kallsyms_hash);
        r->detected = 1;
    }
    g_last_kallsyms_hash = current_hash;

    int found = 0;
    const char *sigs[] = {
        "kallsyms_lookup_name T", "sys_call_table",
        "ksu", "kernelsu", "apatch", "kpm",
        "kprobe", "fprobe", "tracehook",
        "module_alloc", "set_memory_x",
        NULL
    };
    for (const char **s = sigs; *s; s++) {
        if (strstr(buf, *s)) {
            snprintf(r->detail + strlen(r->detail), 256,
                     "DAEMON: Kernel sig '%s'\n", *s);
            found++;
        }
    }

    /* Check for hidden symbols — empty kallsyms */
    if (len < 100) {
        snprintf(r->detail + strlen(r->detail), 256,
                 "DAEMON: kallsyms truncated (%d bytes)\n", (int)len);
        found++;
    }

    return found;
}

int kernel_probe_cycle(void) {
    g_monitor_cycle++;

    char buf[4096];
    int issues = 0;

    int fd = bare_open("/proc/kallsyms", 0, 0);
    if (fd >= 0) {
        ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
        bare_close(fd);
        if (len > 0) {
            buf[len] = '\0';
            uint64_t current_hash = hash_buf(buf, (size_t)len);
            if (g_last_kallsyms_hash != 0 && current_hash != g_last_kallsyms_hash)
                issues++;
            g_last_kallsyms_hash = current_hash;
        }
    } else {
        issues++;
    }

    fd = bare_open("/proc/modules", 0, 0);
    if (fd >= 0) {
        ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
        bare_close(fd);
        if (len > 0) {
            buf[len] = '\0';
            const char *known_bad[] = {"kernelsu", "apatch", "kpm", "magisk", NULL};
            for (const char **b = known_bad; *b; b++) {
                if (strstr(buf, *b)) issues++;
            }
        }
    }

    return issues;
}

int kernel_probe_snapshot_syscall_table(void *result) {
    plugin_result_t *r = (plugin_result_t *)result;

    uint64_t vbar;
    __asm__ volatile("mrs %0, vbar_el1" : "=r"(vbar));
    snprintf(r->detail + strlen(r->detail), 256,
             "DAEMON: VBAR_EL1=0x%llx\n",
             (unsigned long long)vbar);

    uint64_t cntfrq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    snprintf(r->detail + strlen(r->detail), 256,
             "DAEMON: CNTFRQ_EL0=%llu\n",
             (unsigned long long)cntfrq);

    uint64_t mpidr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    snprintf(r->detail + strlen(r->detail), 256,
             "DAEMON: MPIDR_EL1=0x%llx\n",
             (unsigned long long)mpidr);

    return 0;
}
