#ifndef PMU_COUNTER_H
#define PMU_COUNTER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PMU_OK              0
#define PMU_ERR_NOTSUP      (-1)
#define PMU_ERR_ACCESS      (-2)
#define PMU_ERR_PARAM       (-3)

#define PMU_EVENT_CYCLES        0x11
#define PMU_EVENT_CACHE_MISS    0x03
#define PMU_EVENT_BRANCH_MISS   0x0C
#define PMU_EVENT_INST_RETIRED  0x08
#define PMU_EVENT_L1D_CACHE     0x04
#define PMU_EVENT_L2D_CACHE     0x16
#define PMU_EVENT_TLB_MISS      0x0D
#define PMU_EVENT_STALL         0x01

#define PMU_MAX_COUNTERS        6

typedef struct {
    uint32_t event_id;
    uint64_t count;
    uint64_t elapsed_cycles;
    double   ipc;
} pmu_sample_t;

typedef struct {
    pmu_sample_t samples[PMU_MAX_COUNTERS];
    int          sample_count;
    uint64_t     elapsed_ns;
    int          cpu_core;
} pmu_result_t;

int pmu_init(void);
int pmu_enable_counter(int counter_idx, uint32_t event_id);
int pmu_start(void);
int pmu_stop(pmu_result_t *result);
int pmu_reset(void);
int pmu_read_single(uint32_t event_id, uint64_t *count);
int pmu_batch_read(const uint32_t *events, int count, uint64_t *counts);
uint64_t pmu_get_cycle_count(void);

#ifdef __cplusplus
}
#endif

#endif
