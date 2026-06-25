#ifndef LOG_CENTER_H
#define LOG_CENTER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_CENTER_OK       0
#define LOG_CENTER_ERR      (-1)
#define LOG_CENTER_FULL     (-2)
#define LOG_CENTER_TAMPER   (-3)

#define LOG_LEVEL_INFO      0
#define LOG_LEVEL_WARN      1
#define LOG_LEVEL_ALERT     2
#define LOG_LEVEL_CRITICAL  3

#define LOG_ENTRY_MAX       4096
#define LOG_MAX_ENTRIES     10000
#define LOG_SIG_SIZE        64

typedef struct {
    uint32_t seq;
    uint32_t timestamp;
    uint8_t  level;
    uint8_t  source_domain;
    uint16_t data_len;
    uint8_t  data[LOG_ENTRY_MAX];
    uint8_t  signature[LOG_SIG_SIZE];
} log_entry_t;

typedef struct {
    log_entry_t *entries;
    int          entry_count;
    int          total_capacity;
    uint32_t     next_seq;
    uint8_t      master_key[32];
} log_center_t;

int log_center_init(log_center_t *log, const uint8_t *key);
int log_center_write(log_center_t *log, uint8_t level, uint8_t domain,
                      const uint8_t *data, uint16_t len);
int log_center_flush(log_center_t *log);
int log_center_verify_integrity(log_center_t *log);
int log_center_export(log_center_t *log, const char *path);
int log_center_rotate(log_center_t *log);

#ifdef __cplusplus
}
#endif

#endif
