#include "dir_isolation.h"
#include "../bare_syscall/bare_syscall.h"
#include "../bare_syscall/raw_file_io.h"
#include "../ipc/ipc.h"
#include <string.h>

#define MAX_DIR_PATHS 32
#define PATH_BUF_SIZE 256

static const char *suspicious_dirs[] = {
    "/data/adb",
    "/data/adb/modules",
    "/data/adb/service.d",
    "/data/adb/post-fs-data.d",
    "/data/adb/magisk",
    "/data/adb/ksu",
    "/data/adb/apatch",
    "/data/adb/lspd",
    "/sbin/.magisk",
    "/sbin/.core",
    "/debug_ramdisk",
    "/data/adb/tricky_store",
    "/data/local/tmp/.magisk",
    "/data/local/tmp/.ksu",
    "/data/local/tmp/.apatch",
    NULL
};

static const char *suspicious_files[] = {
    "magisk",
    "magiskpolicy",
    "magiskinit",
    "ksud",
    "ksu_susfs",
    "apatch",
    "kpatch",
    "lsposed",
    "lspd",
    "riru",
    "riruhide",
    "shamiko",
    "zygisk",
    "zygisk_next",
    NULL
};

static int g_initialized = 0;

int dir_isolation_init(void) {
    g_initialized = 1;
    return DIR_ISO_OK;
}

static int check_path_exists(const char *path) {
    long fd = bare_openat(-100, path, 0, 0);
    if (!bare_is_error(fd)) {
        bare_close((int)fd);
        return 1;
    }
    return 0;
}

static int check_file_in_dir(const char *dir, const char *file) {
    char full_path[PATH_BUF_SIZE];
    int pos = 0;

    for (int i = 0; dir[i] && pos < PATH_BUF_SIZE - 2; i++)
        full_path[pos++] = dir[i];
    if (pos > 0 && full_path[pos-1] != '/') full_path[pos++] = '/';
    for (int i = 0; file[i] && pos < PATH_BUF_SIZE - 1; i++)
        full_path[pos++] = file[i];
    full_path[pos] = '\0';

    return check_path_exists(full_path);
}

int dir_isolation_scan(dir_isolation_result_t *result) {
    if (!result) return DIR_ISO_ERR;

    memset(result, 0, sizeof(dir_isolation_result_t));
    int find_pos = 0;

    for (int i = 0; suspicious_dirs[i] != NULL; i++) {
        result->total_checked++;

        if (check_path_exists(suspicious_dirs[i])) {
            result->total_suspicious++;

            int remaining = (int)sizeof(result->findings) - find_pos - 2;
            if (remaining > 0) {
                int len = 0;
                const char *s = suspicious_dirs[i];
                while (s[len] && find_pos + len < (int)sizeof(result->findings) - 2) {
                    result->findings[find_pos + len] = s[len];
                    len++;
                }
                find_pos += len;
                result->findings[find_pos++] = '\n';
                result->findings[find_pos] = '\0';
            }

            if (strcmp(suspicious_dirs[i], "/data/adb") == 0) {
                for (int j = 0; suspicious_files[j] != NULL; j++) {
                    result->total_checked++;
                    if (check_file_in_dir(suspicious_dirs[i], suspicious_files[j])) {
                        result->total_suspicious++;
                    }
                }
            }
        }
    }

    if (result->total_suspicious > 5) result->risk_level = 3;
    else if (result->total_suspicious > 2) result->risk_level = 2;
    else if (result->total_suspicious > 0) result->risk_level = 1;

    return result->total_suspicious > 0 ? DIR_ISO_DETECTED : DIR_ISO_OK;
}

int dir_isolation_block_path(const char *path) {
    return check_path_exists(path);
}

int dir_isolation_check_access(const char *path) {
    if (!path) return DIR_ISO_ERR;

    for (int i = 0; suspicious_dirs[i] != NULL; i++) {
        const char *dir = suspicious_dirs[i];
        int match = 1;
        int j = 0;
        while (dir[j] && path[j]) {
            if (dir[j] != path[j]) { match = 0; break; }
            j++;
        }
        if (match && (dir[j] == '\0' || path[j] == '/' || path[j] == '\0')) {
            return DIR_ISO_DETECTED;
        }
    }

    return DIR_ISO_OK;
}
