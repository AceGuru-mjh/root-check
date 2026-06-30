// APEX-Root PMU Counter Spoofing
// Hides eBPF/tracepoint/Hook microarchitectural anomalies
// by returning fake perf_event_open counters.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <android/log.h>

#define LOG_TAG "APEX-PMU"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static int pmu_fd = -1;

struct pmu_clean_baseline {
    long long cycles;        // Total CPU cycles
    long long instructions;  // Total instructions
    long long cache_refs;    // Cache references
    long long cache_misses;  // Cache misses
    long long branches;      // Branch instructions
    long long branch_misses; // Branch misses
};

static struct pmu_clean_baseline clean_pmu = {0};

int apex_pmu_init(void) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 0;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    pmu_fd = syscall(241 /* __NR_perf_event_open on ARM64 */, &pe, 0, -1, -1, 0);
    if (pmu_fd < 0) {
        LOGD("PMU init failed (expected on some kernels): %s", strerror(errno));
        return -1;
    }

    // Read real PMU once to establish baseline
    read(pmu_fd, &clean_pmu.cycles, sizeof(clean_pmu.cycles));
    LOGD("PMU initialized, baseline cycles: %lld", clean_pmu.cycles);
    return 0;
}

/*
 * Fixed: Use arc4random() instead of random() for cryptographic safety.
 * random() is predictable and its output can be used to detect spoofing.
 */
void apex_pmu_spoof_read(void *buf, size_t count) {
    if (pmu_fd < 0) return;

    // Return clean baseline + small delta to look natural
    struct pmu_clean_baseline fake;
    fake.cycles = clean_pmu.cycles + (arc4random() % 100);
    fake.instructions = clean_pmu.cycles * 2 + (arc4random() % 50);
    fake.cache_refs = clean_pmu.cycles / 3 + (arc4random() % 20);
    fake.cache_misses = clean_pmu.cache_refs / 10 + (arc4random() % 5);
    fake.branches = clean_pmu.instructions / 4 + (arc4random() % 30);
    fake.branch_misses = fake.branches / 20 + (arc4random() % 2);

    size_t copy = count < sizeof(fake) ? count : sizeof(fake);
    memcpy(buf, &fake, copy);
    LOGD("PMU spoof: cycles=%lld misses=%lld",
         fake.cycles, fake.branch_misses);
}

void apex_pmu_cleanup(void) {
    if (pmu_fd >= 0) {
        close(pmu_fd);
        pmu_fd = -1;
    }
}
