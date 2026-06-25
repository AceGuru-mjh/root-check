#include "memory_checker.h"
#include "../bare_syscall/bare_syscall.h"
#include "../crypto/core/crypto_core.h"
#include "../sandbox_microkernel/plugin_interface.h"
#include <string.h>

int memory_checker_scan(void *result) {
    plugin_result_t *r = (plugin_result_t *)result;
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char maps[65536];
    ssize_t len = bare_read_full(fd, maps, sizeof(maps) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    maps[len] = '\0';

    int found = 0;
    char *line = maps;
    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next) *next = '\0';

        if (strstr(line, "rwxp") || strstr(line, "rwx ")) {
            snprintf(r->detail + strlen(r->detail), 256,
                     "DAEMON: W+X region: %s\n", line);
            found++;
        }

        char *perm_end = strchr(line, ' ');
        if (perm_end && perm_end > line + 4) {
            char perm[8];
            int plen = (int)(perm_end - line) - 5;
            if (plen > 7) plen = 7;
            bare_memcpy(perm, line + 5, plen);
            perm[plen] = '\0';

            if (strstr(perm, "s") || strstr(perm, "S")) {
                if (strstr(perm, "w") && strstr(perm, "x")) {
                    snprintf(r->detail + strlen(r->detail), 256,
                             "DAEMON: Shared W+X: %s\n", line);
                    found++;
                }
            }
        }

        /* Check for anonymous executable mappings (JIT spray) */
        if (strstr(line, "---p") && (strstr(line, "MemFD") || strstr(line, "memfd"))) {
            snprintf(r->detail + strlen(r->detail), 256,
                     "DAEMON: memfd JIT: %s\n", line);
            found++;
        }

        line = next ? (next + 1) : NULL;
    }

    if (found) r->detected = 1;
    return found;
}

int memory_checker_scan_process(pid_t pid) {
    char path[64];
    int pos = 0;
    const char *prefix = "/proc/";
    for (int i = 0; prefix[i] && pos < 63; i++) path[pos++] = prefix[i];

    char pidbuf[16];
    int pi = 0;
    int tmp = pid;
    if (tmp == 0) { pidbuf[pi++] = '0'; }
    else { while (tmp > 0 && pi < 15) { pidbuf[pi++] = '0' + (tmp % 10); tmp /= 10; } }
    for (int i = pi - 1; i >= 0 && pos < 63; i--) path[pos++] = pidbuf[i];

    const char *suffix = "/maps";
    for (int i = 0; suffix[i] && pos < 63; i++) path[pos++] = suffix[i];
    path[pos] = '\0';

    int fd = bare_open(path, 0, 0);
    if (fd < 0) return 0;
    char maps[65536];
    ssize_t len = bare_read_full(fd, maps, sizeof(maps) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    maps[len] = '\0';

    return strstr(maps, "rwxp") || strstr(maps, "rwx ") ? 1 : 0;
}
