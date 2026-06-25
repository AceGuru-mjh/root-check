#include "log_center.h"
#include "../bare_syscall/bare_syscall.h"
#include "../bare_syscall/cpu_control.h"
#include "../crypto/core/crypto_core.h"
#include <string.h>

#define LOG_FILE_PATH "/data/local/tmp/.rootguard_log"
#define LOG_FILE_MAX (64 * 1024 * 1024)
#define LOG_CENTER_MAGIC 0x52474C4F

static uint32_t get_timestamp(void) {
    uint64_t ns = get_boottime_ns();
    return (uint32_t)(ns / 1000000000ULL);
}

int log_center_init(log_center_t *log, const uint8_t *key) {
    if (!log || !key) return LOG_CENTER_ERR;

    log->entry_count = 0;
    log->total_capacity = LOG_MAX_ENTRIES;
    log->next_seq = 1;

    for (int i = 0; i < 32; i++) log->master_key[i] = key[i];

    log->entries = (log_entry_t *)bare_mmap(
        NULL, LOG_MAX_ENTRIES * sizeof(log_entry_t),
        3, 0x22, -1, 0);
    if (bare_is_error((long)log->entries)) return LOG_CENTER_ERR;

    return LOG_CENTER_OK;
}

int log_center_write(log_center_t *log, uint8_t level, uint8_t domain,
                      const uint8_t *data, uint16_t len) {
    if (!log || !data || len > LOG_ENTRY_MAX) return LOG_CENTER_ERR;
    if (log->entry_count >= log->total_capacity) {
        log_center_rotate(log);
    }

    log_entry_t *entry = &log->entries[log->entry_count];
    entry->seq = log->next_seq++;
    entry->timestamp = get_timestamp();
    entry->level = level;
    entry->source_domain = domain;
    entry->data_len = len;

    for (int i = 0; i < len; i++) entry->data[i] = data[i] ^ log->master_key[i % 32];
    if (len < LOG_ENTRY_MAX) entry->data[len] = '\0';

    uint8_t hash[32];
    sha3_256((const uint8_t *)entry, sizeof(log_entry_t) - LOG_SIG_SIZE, hash);

    for (int i = 0; i < 32; i++) {
        entry->signature[i] = hash[i] ^ log->master_key[i % 32];
    }

    log->entry_count++;
    return LOG_CENTER_OK;
}

int log_center_flush(log_center_t *log) {
    if (!log) return LOG_CENTER_ERR;

    long fd = bare_openat(-100, LOG_FILE_PATH, 0x41, 0600);
    if (bare_is_error(fd)) return LOG_CENTER_ERR;

    long header = LOG_CENTER_MAGIC;
    bare_write((int)fd, (const void *)&header, sizeof(header));
    bare_write((int)fd, (const void *)&log->entry_count, sizeof(log->entry_count));

    long written = bare_write((int)fd, (const void *)log->entries,
                              log->entry_count * sizeof(log_entry_t));
    bare_close((int)fd);

    return written > 0 ? LOG_CENTER_OK : LOG_CENTER_ERR;
}

int log_center_verify_integrity(log_center_t *log) {
    if (!log) return LOG_CENTER_ERR;

    for (int i = 0; i < log->entry_count; i++) {
        log_entry_t *entry = &log->entries[i];

        uint8_t hash[32];
        sha3_256((const uint8_t *)entry, sizeof(log_entry_t) - LOG_SIG_SIZE, hash);

        int valid = 1;
        for (int j = 0; j < 32; j++) {
            if ((entry->signature[j] ^ log->master_key[j % 32]) != hash[j]) {
                valid = 0;
                break;
            }
        }

        if (!valid) return LOG_CENTER_TAMPER;
    }

    return LOG_CENTER_OK;
}

int log_center_export(log_center_t *log, const char *path) {
    if (!log || !path) return LOG_CENTER_ERR;

    int ret = log_center_verify_integrity(log);
    if (ret != LOG_CENTER_OK) return ret;

    long fd = bare_openat(-100, path, 0x41, 0600);
    if (bare_is_error(fd)) return LOG_CENTER_ERR;

    uint8_t export_key[32];
    for (int i = 0; i < 32; i++) export_key[i] = log->master_key[i];

    bare_write((int)fd, (const void *)export_key, 32);
    bare_write((int)fd, (const void *)&log->entry_count, sizeof(log->entry_count));
    bare_write((int)fd, (const void *)log->entries,
               log->entry_count * sizeof(log_entry_t));
    bare_close((int)fd);

    return LOG_CENTER_OK;
}

int log_center_rotate(log_center_t *log) {
    if (!log) return LOG_CENTER_ERR;

    int keep = log->entry_count / 2;
    int remove_count = log->entry_count - keep;

    for (int i = 0; i < keep; i++) {
        log->entries[i] = log->entries[remove_count + i];
    }

    log->entry_count = keep;
    return LOG_CENTER_OK;
}
