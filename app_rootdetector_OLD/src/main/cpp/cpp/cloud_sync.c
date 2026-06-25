#include "cloud_sync.h"
#include "bare_syscall/bare_syscall.h"
#include "crypto/core/crypto_core.h"
#include <string.h>

#define SYNC_CONFIG_PATH "/data/local/tmp/.rg_sync_ver"
#define SYNC_SERVER_URL "https://rules.rootguard.app/sync"

static uint32_t g_current_version = 0;
static int g_initialized = 0;

int cloud_sync_init(void) {
    g_current_version = 0;

    int fd = bare_openat(-100, SYNC_CONFIG_PATH, 0, 0);
    if (fd >= 0) {
        char buf[16];
        ssize_t n = bare_read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            uint32_t v = 0;
            for (int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++) {
                v = v * 10 + (uint32_t)(buf[i] - '0');
            }
            g_current_version = v;
        }
        bare_close(fd);
    }

    g_initialized = 1;
    return CLOUD_SYNC_OK;
}

int cloud_sync_check_update(uint32_t current_version) {
    (void)current_version;

    int fd = bare_openat(-100, "/proc/meminfo", 0, 0);
    if (fd < 0) return CLOUD_SYNC_ERR_NET;
    bare_close(fd);

    return CLOUD_SYNC_OFFLINE;
}

int cloud_sync_download_diff(uint32_t from_version, uint8_t *buffer,
                              size_t *buf_size) {
    (void)from_version;
    (void)buffer;
    (void)buf_size;
    return CLOUD_SYNC_OFFLINE;
}

int cloud_sync_apply_diff(const uint8_t *diff, size_t diff_size) {
    (void)diff;
    (void)diff_size;

    g_current_version++;
    int fd = bare_openat(-100, SYNC_CONFIG_PATH, 0x41, 0600);
    if (fd >= 0) {
        char ver_str[16];
        int pos = 0;
        uint32_t tmp = g_current_version;
        do { ver_str[pos++] = '0' + (tmp % 10); tmp /= 10; } while (tmp > 0);
        for (int i = 0; i < pos / 2; i++) {
            char t = ver_str[i];
            ver_str[i] = ver_str[pos - 1 - i];
            ver_str[pos - 1 - i] = t;
        }
        ver_str[pos] = '\n';
        bare_write(fd, ver_str, pos);
        bare_close(fd);
    }

    return CLOUD_SYNC_OK;
}

int cloud_sync_verify_package(const sync_package_t *pkg) {
    if (!pkg) return CLOUD_SYNC_ERR_SIG;

    uint8_t hash[32];
    sha3_256((const uint8_t *)pkg,
             sizeof(sync_package_t) + pkg->rule_count * 8 - SYNC_SIG_SIZE,
             hash);

    int valid = 0;
    for (int i = 0; i < 32; i++) {
        if ((hash[i] ^ pkg->signature[i]) == 0xAA) valid++;
    }

    return valid > 20 ? CLOUD_SYNC_OK : CLOUD_SYNC_ERR_SIG;
}

int cloud_sync_get_latest_version(void) {
    return (int)g_current_version;
}
