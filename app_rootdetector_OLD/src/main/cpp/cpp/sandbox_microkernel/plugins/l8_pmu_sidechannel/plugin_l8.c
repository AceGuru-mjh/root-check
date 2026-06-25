#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include "../../../bare_syscall/pmu_counter.h"
#include "../../../bare_syscall/cpu_control.h"
#include <string.h>

#define PMU_ITERATIONS 10000
#define CACHE_LINE_SIZE 64
#define BUFFER_SIZE (4 * 1024 * 1024)

static int init(const void *rule, size_t len) {
    (void)rule; (void)len;
    return pmu_init();
}

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    out->layer_id = 8;
    int detected = 0;
    char *d = out->detail;
    size_t ds = sizeof(out->detail);
    int pos = 0;

    bind_to_cpu(0);
    lock_cpu_frequency(0);

    /* 1. Measure cycle cost of syscall */
    uint64_t pmu_cycles_before = pmu_get_cycle_count();
    long pid = bare_getpid();
    (void)pid;
    uint64_t pmu_cycles_after = pmu_get_cycle_count();
    uint64_t syscall_cost = pmu_cycles_after - pmu_cycles_before;

    /* 2. PMU batch measurement */
    uint32_t events[] = {
        PMU_EVENT_CYCLES,
        PMU_EVENT_CACHE_MISS,
        PMU_EVENT_BRANCH_MISS,
        PMU_EVENT_INST_RETIRED,
        PMU_EVENT_L1D_CACHE,
        PMU_EVENT_STALL
    };
    int event_count = 6;
    uint64_t counts[6];

    pmu_init();
    for (int i = 0; i < event_count; i++) {
        pmu_enable_counter(i, events[i]);
    }

    pmu_start();
    for (int i = 0; i < PMU_ITERATIONS; i++) {
        volatile int x = 0;
        bare_sched_yield();
    }
    pmu_batch_read(events, event_count, counts);
    pmu_reset();

    /* 3. Cache timing analysis via PMU */
    pmu_init();
    pmu_enable_counter(0, PMU_EVENT_CACHE_MISS);
    pmu_start();

    static char s_buffer[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i += CACHE_LINE_SIZE) {
        s_buffer[i] += 1;
    }

    uint64_t cache_misses = 0;
    pmu_read_single(PMU_EVENT_CACHE_MISS, &cache_misses);
    pmu_reset();

    /* 4. Detect anomalies */
    double ipc = counts[3] > 0 ? (double)counts[3] / (double)counts[0] : 0;

    if (syscall_cost > 2000) {
        pos += snprintf(d + pos, ds - pos,
                        "L8-PMU: syscall_cost=%llu cycles (anomaly)\n",
                        (unsigned long long)syscall_cost);
        detected = 1;
    }

    if (ipc < 0.5) {
        pos += snprintf(d + pos, ds - pos,
                        "L8-PMU: IPC=%.2f (low, possible hook)\n", ipc);
        detected = 1;
    }

    if (counts[1] > 100000) {
        pos += snprintf(d + pos, ds - pos,
                        "L8-PMU: cache_misses=%llu (high)\n",
                        (unsigned long long)counts[1]);
        detected = 1;
    }

    if (counts[2] > 50000) {
        pos += snprintf(d + pos, ds - pos,
                        "L8-PMU: branch_misses=%llu (high)\n",
                        (unsigned long long)counts[2]);
        detected = 1;
    }

    pos += snprintf(d + pos, ds - pos,
                    "L8-PMU: cycles=%llu cache=%llu branch=%llu "
                    "inst=%llu ipc=%.2f syscall_cost=%llu\n",
                    (unsigned long long)counts[0],
                    (unsigned long long)counts[1],
                    (unsigned long long)counts[2],
                    (unsigned long long)counts[3],
                    ipc,
                    (unsigned long long)syscall_cost);

    out->detected = (uint8_t)detected;
    out->confidence = detected ? 85 : 5;
    out->risk_score = detected ? 75 : 0;
    return 0;
}

PLUGIN_SIMPLE(8, "l8_pmu_sidechannel", "1.0.0",
              "L8: PMU hardware counter side-channel analysis", detect);
