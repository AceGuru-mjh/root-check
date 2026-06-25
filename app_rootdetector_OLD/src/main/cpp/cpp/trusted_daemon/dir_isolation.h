#ifndef DIR_ISOLATION_H
#define DIR_ISOLATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DIR_ISO_OK          0
#define DIR_ISO_DETECTED    1
#define DIR_ISO_ERR         (-1)

typedef struct {
    int  total_suspicious;
    int  total_checked;
    char findings[4096];
    int  risk_level;
} dir_isolation_result_t;

int dir_isolation_init(void);
int dir_isolation_scan(dir_isolation_result_t *result);
int dir_isolation_block_path(const char *path);
int dir_isolation_check_access(const char *path);

#ifdef __cplusplus
}
#endif

#endif
