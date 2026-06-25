#include "anti_hook.h"
#include "../bare_syscall/bare_syscall.h"
#include "../crypto/core/crypto_core.h"
#include "../sandbox_microkernel/plugin_interface.h"
#include <string.h>

int anti_hook_check_libraries(void) {
    int fd = bare_open("/proc/self/maps", 0, 0);
    if (fd < 0) return 0;
    char buf[65536];
    ssize_t len = bare_read_full(fd, buf, sizeof(buf) - 1);
    bare_close(fd);
    if (len <= 0) return 0;
    buf[len] = '\0';

    const char *hook_patterns[] = {
        "frida", "xposed", "substrate", "cydia",
        "libinject", "gum-js-loop", "libgum",
        "frida-agent", "frida-gadget", "libriru",
        "libwhale", "libDobby", "libhookzz",
        "libpine", "libsandhook", "libepic",
        "libratel", NULL
    };

    for (const char **p = hook_patterns; *p; p++) {
        if (strstr(buf, *p)) return 1;
    }

    /* Check for anonymous executable regions (code injection) */
    char *line = buf;
    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next) *next = '\0';

        if ((strstr(line, "rwxp") || strstr(line, "rwx ")) &&
            strstr(line, "[vdso]") == NULL &&
            strstr(line, "[vvar]") == NULL) {
            if (!strstr(line, "lib")) {
                return 1;
            }
        }
        line = next ? (next + 1) : NULL;
    }

    return 0;
}

int anti_hook_check_got(void) {
    int fd = bare_open("/proc/self/exe", 0, 0);
    if (fd < 0) return 0;

    char buf[65536];
    ssize_t n = bare_read_full(fd, buf, sizeof(buf));
    bare_close(fd);
    if (n <= 0) return 0;

    size_t size = (size_t)n;
    int found = 0;

    /* Look for suspicious B/BR/BLR instruction patterns
     * that may indicate inline hooks in the first 32KB */
    size_t scan_size = size < 32768 ? size : 32768;
    for (size_t i = 0; i < scan_size - 4; i++) {
        uint32_t insn;
        bare_memcpy(&insn, buf + i, 4);

        uint32_t opcode = insn & 0xFC000000;
        if (opcode == 0x14000000 || opcode == 0x94000000) {
            int32_t offset = (int32_t)(insn & 0x03FFFFFF);
            if (offset < 0) offset |= ~0x01FFFFFF;
            offset <<= 2;

            uintptr_t target = (uintptr_t)&buf[i] + offset;

            if (target < (uintptr_t)buf || target >= (uintptr_t)(buf + size)) {
                found++;
            }
        }
    }

    return found > 5 ? 1 : 0;
}

int anti_hook_check(void *result) {
    plugin_result_t *r = (plugin_result_t *)result;
    int found = 0;

    if (anti_hook_check_libraries()) {
        strcat(r->detail, "DAEMON: Hook library detected\n");
        found++;
    }
    if (anti_hook_check_got()) {
        strcat(r->detail, "DAEMON: Branch to external detected\n");
        found++;
    }

    if (found) r->detected = 1;
    return found;
}
