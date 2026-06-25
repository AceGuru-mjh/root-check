#ifndef INTERRUPT_RAW_H
#define INTERRUPT_RAW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INT_RAW_OK          0
#define INT_RAW_ERR_ACCESS  (-1)
#define INT_RAW_ERR_PARAM   (-2)

#define INT_RAW_MAX_SOURCES 64

typedef struct {
    int    irq_number;
    uint64_t interrupt_count;
    char   name[64];
    int    is_suspicious;
} irq_stat_t;

typedef struct {
    irq_stat_t stats[INT_RAW_MAX_SOURCES];
    int        source_count;
    uint64_t   total_interrupts;
    uint64_t   total_since_boot;
} irq_snapshot_t;

int interrupt_raw_read(irq_snapshot_t *snapshot);
int interrupt_raw_diff(const irq_snapshot_t *before, const irq_snapshot_t *after);
int interrupt_raw_detect_anomaly(const irq_snapshot_t *current);

#ifdef __cplusplus
}
#endif

#endif
