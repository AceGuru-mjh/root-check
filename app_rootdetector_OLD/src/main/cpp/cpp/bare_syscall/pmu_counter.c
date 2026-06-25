#include "pmu_counter.h"
#include "cpu_control.h"
#include "syscall_arm64.h"

#define PMU_REG_PMCR         0xCD90
#define PMU_REG_PMCNTENSET   0xCDA0
#define PMU_REG_PMCNTENCLR   0xCDA1
#define PMU_REG_PMOVSR       0xCDB0
#define PMU_REG_PMSWINC      0xCDB4
#define PMU_REG_PMSELR       0xCDB8
#define PMU_REG_PMCEID0      0xCDD0
#define PMU_REG_PMCCNTR      0xCDD8
#define PMU_REG_PMXEVTYPER   0xCDD4
#define PMU_REG_PMXEVCNTR    0xCDD8
#define PMU_REG_PMUSERENR    0xCDA4
#define PMU_REG_PMINTEN      0xCDB0

static int g_pmu_initialized = 0;
static int g_pmu_counters_enabled = 0;
static uint64_t g_start_cycles = 0;

static uint64_t read_pmu_reg(int reg) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmcr_el0" : "=r"(val));
    return val;
}

static void write_pmu_reg(int reg, uint64_t val) {
    __asm__ volatile("msr pmcr_el0, %0" : : "r"(val));
}

static uint64_t read_pmcntenset(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmcntenset_el0" : "=r"(val));
    return val;
}

static void write_pmcntenset(uint64_t val) {
    __asm__ volatile("msr pmcntenset_el0, %0" : : "r"(val));
}

static void write_pmcntenclear(uint64_t val) {
    __asm__ volatile("msr pmcntenset_el0, %0" : : "r"(val));
}

static uint64_t read_pmovsr(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmovsr_el0" : "=r"(val));
    return val;
}

static void write_pmovsr(uint64_t val) {
    __asm__ volatile("msr pmovsr_el0, %0" : : "r"(val));
}

static void write_pmselr(uint64_t val) {
    __asm__ volatile("msr pmselr_el0, %0" : : "r"(val));
}

static uint64_t read_pmxevcntr(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmxevcntr_el0" : "=r"(val));
    return val;
}

static void write_pmxevtyper(uint64_t val) {
    __asm__ volatile("msr pmxevtyper_el0, %0" : : "r"(val));
}

static uint64_t read_pmccntr(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmccntr_el0" : "=r"(val));
    return val;
}

static uint64_t read_pmceid0(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmceid0_el0" : "=r"(val));
    return val;
}

static uint64_t read_pmuserenr(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, pmuserenr_el0" : "=r"(val));
    return val;
}

static void write_pmuserenr(uint64_t val) {
    __asm__ volatile("msr pmuserenr_el0, %0" : : "r"(val));
}

int pmu_init(void) {
    if (g_pmu_initialized) return PMU_OK;

    uint64_t pmceid0 = read_pmceid0();
    if (pmceid0 == 0) return PMU_ERR_NOTSUP;

    uint64_t pmuserenr = read_pmuserenr();
    if (!(pmuserenr & 1)) {
        write_pmuserenr(pmuserenr | 1);
    }

    uint64_t pmcr = read_pmu_reg(PMU_REG_PMCR);
    pmcr |= (1 << 0);
    pmcr |= (1 << 2);
    write_pmu_reg(PMU_REG_PMCR, pmcr);

    write_pmovsr(0xFFFFFFFFFFFFFFFFULL);
    write_pmcntenclear(0xFFFFFFFFFFFFFFFFULL);

    g_pmu_initialized = 1;
    return PMU_OK;
}

int pmu_enable_counter(int counter_idx, uint32_t event_id) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;
    if (counter_idx < 0 || counter_idx >= PMU_MAX_COUNTERS) return PMU_ERR_PARAM;

    write_pmselr((uint64_t)counter_idx);
    write_pmxevtyper((uint64_t)event_id);
    write_pmcntenset(1ULL << (uint64_t)counter_idx);

    return PMU_OK;
}

int pmu_start(void) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;

    g_start_cycles = read_pmccntr();
    g_pmu_counters_enabled = 1;
    return PMU_OK;
}

int pmu_stop(pmu_result_t *result) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;
    if (!result) return PMU_ERR_PARAM;

    uint64_t end_cycles = read_pmccntr();
    uint64_t total_cycles = end_cycles - g_start_cycles;

    result->sample_count = 0;
    result->elapsed_ns = 0;
    result->cpu_core = 0;

    for (int i = 0; i < PMU_MAX_COUNTERS; i++) {
        write_pmselr((uint64_t)i);
        uint64_t ovsr = read_pmovsr();

        if (ovsr & (1ULL << i)) {
            pmu_sample_t *s = &result->samples[result->sample_count];
            s->event_id = 0;
            s->count = read_pmxevcntr();
            s->elapsed_cycles = total_cycles;
            s->ipc = total_cycles > 0 ? (double)s->count / (double)total_cycles : 0.0;
            result->sample_count++;
        }
    }

    uint64_t cntfrq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    if (cntfrq > 0) {
        result->elapsed_ns = (total_cycles * 1000000000ULL) / cntfrq;
    }

    int cpu;
    __asm__ volatile("mrs %0, tpidrro_el0" : "=r"(cpu));
    result->cpu_core = (int)(cpu & 0xFF);

    g_pmu_counters_enabled = 0;
    write_pmovsr(0xFFFFFFFFFFFFFFFFULL);

    return PMU_OK;
}

int pmu_reset(void) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;

    write_pmovsr(0xFFFFFFFFFFFFFFFFULL);
    write_pmcntenclear(0xFFFFFFFFFFFFFFFFULL);
    g_pmu_counters_enabled = 0;

    uint64_t pmcr = read_pmu_reg(PMU_REG_PMCR);
    pmcr |= (1 << 1);
    write_pmu_reg(PMU_REG_PMCR, pmcr);

    return PMU_OK;
}

int pmu_read_single(uint32_t event_id, uint64_t *count) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;
    if (!count) return PMU_ERR_PARAM;

    write_pmselr(0);
    write_pmxevtyper((uint64_t)event_id);
    *count = read_pmxevcntr();

    return PMU_OK;
}

int pmu_batch_read(const uint32_t *events, int count, uint64_t *counts) {
    if (!g_pmu_initialized) return PMU_ERR_NOTSUP;
    if (!events || !counts || count > PMU_MAX_COUNTERS) return PMU_ERR_PARAM;

    for (int i = 0; i < count; i++) {
        write_pmselr((uint64_t)i);
        write_pmxevtyper((uint64_t)events[i]);
        counts[i] = read_pmxevcntr();
    }

    return PMU_OK;
}

uint64_t pmu_get_cycle_count(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
