// APEX-Root Side-Channel Timing Spoofing
// Intercepts clock_gettime / gettimeofday / rdtsc to hide Hook detection.
// Detection tools measure syscall latency - if Hook adds even 100ns delay,
// timing analysis reveals the Hook. This module normalizes the timing.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "APEX-TIMG"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Structure to hold timing baseline for different syscalls
struct timing_baseline {
    // Baseline latency in nanoseconds for each critical syscall
    long long openat_ns;
    long long statx_ns;
    long long getdents_ns;
    long long read_ns;
    long long access_ns;
    long long clock_gettime_ns;
};

static struct timing_baseline timing_base = {
    .openat_ns = 850,
    .statx_ns = 620,
    .getdents_ns = 1200,
    .read_ns = 480,
    .access_ns = 400,
    .clock_gettime_ns = 90,
};

// Jitter range (nanoseconds) to add for natural appearance
#define JITTER_MIN 10
#define JITTER_MAX 150

static long long get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Measure actual syscall latency for calibration
void apex_timing_calibrate(void) {
    long long start, end;

    start = get_ns();
    access("/dev/null", F_OK);
    end = get_ns();
    timing_base.access_ns = end - start;

    start = get_ns();
    get_ns();
    end = get_ns();
    timing_base.clock_gettime_ns = end - start;

    LOGD("Timing calibrated: access=%lldns clock_gettime=%lldns",
         timing_base.access_ns, timing_base.clock_gettime_ns);
}

// Generate plausible jitter using arc4random (crypto-safe, non-predictable)
static long long jitter(void) {
    return JITTER_MIN + (long long)(arc4random_uniform(JITTER_MAX - JITTER_MIN + 1));
}

/*
 * Fixed: Original code could return a negative value when called rapidly
 * (get_ns() < baseline + jitter). Now clamps to a minimum of 0.
 */
long long apex_timing_get_expected_ns(void) {
    long long now = get_ns();
    long long adjusted = now - timing_base.clock_gettime_ns - jitter();
    return adjusted > 0 ? adjusted : 0;
}

/*
 * Fixed: If hook_overhead exceeds current time, the original code
 * produced a negative timespec (tv_sec=0, tv_nsec = large negative % 1e9
 * which is a huge positive value). Now properly clamps.
 */
int apex_timing_adjust_clock(struct timespec *tp) {
    if (!tp) return -1;

    long long hook_overhead = timing_base.clock_gettime_ns + jitter();
    long long current_ns = (long long)tp->tv_sec * 1000000000LL + tp->tv_nsec;

    if (current_ns > hook_overhead) {
        current_ns -= hook_overhead;
    } else {
        current_ns = 0;
    }

    tp->tv_sec = current_ns / 1000000000LL;
    tp->tv_nsec = current_ns % 1000000000LL;
    return 0;
}
