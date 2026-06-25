#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOUD_SYNC_OK           0
#define CLOUD_SYNC_ERR_NET     (-1)
#define CLOUD_SYNC_ERR_SIG     (-2)
#define CLOUD_SYNC_ERR_VER     (-3)
#define CLOUD_SYNC_OFFLINE      1

#define SYNC_MAX_DIFF_SIZE  (64 * 1024)
#define SYNC_SIG_SIZE       64

typedef struct {
    uint32_t version;
    uint32_t rule_count;
    uint8_t  fingerprint[32];
    uint8_t  signature[SYNC_SIG_SIZE];
    uint8_t  data[];
} sync_package_t;

int cloud_sync_init(void);
int cloud_sync_check_update(uint32_t current_version);
int cloud_sync_download_diff(uint32_t from_version, uint8_t *buffer,
                              size_t *buf_size);
int cloud_sync_apply_diff(const uint8_t *diff, size_t diff_size);
int cloud_sync_verify_package(const sync_package_t *pkg);
int cloud_sync_get_latest_version(void);

#ifdef __cplusplus
}
#endif

#endif
