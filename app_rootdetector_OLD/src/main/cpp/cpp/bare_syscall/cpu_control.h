/*
 * cpu_control.h - CPU core binding and frequency control via bare syscalls
 *
 * Domain D: 底层裸系统汇编抽象层
 * All operations use bare syscalls, NO libc dependency
 */

#ifndef CPU_CONTROL_H
#define CPU_CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of CPU cores supported */
#define MAX_CPU_CORES       64

/* CPU frequency governor modes */
#define GOV_PERFORMANCE     0
#define GOV_POWERSAVE       1
#define GOV_USERSPACE       2
#define GOV_ONDEMAND        3
#define GOV_CONSERVATIVE    4
#define GOV_SCHEDUTIL       5

/* Error codes */
#define CPU_OK              0
#define CPU_ERR_INVALID_CORE    (-1)
#define CPU_ERR_SYSCALL_FAIL    (-2)
#define CPU_ERR_FREQ_LOCKED     (-3)
#define CPU_ERR_GOVERNOR_FAIL   (-4)

/**
 * Bind current thread to a specific CPU core
 * Uses sched_setaffinity bare syscall
 *
 * @param core_id  CPU core index (0-based)
 * @return 0 on success, negative errno on failure
 */
__attribute__((visibility("default")))
int bind_to_cpu(int core_id);

/**
 * Bind current thread to a set of CPU cores
 *
 * @param mask  Array of core IDs to bind to
 * @param count Number of cores in the mask
 * @return 0 on success, negative errno on failure
 */
__attribute__((visibility("default")))
int bind_to_cpu_set(const int *mask, int count);

/**
 * Lock CPU frequency to maximum (performance mode)
 * Writes to /sys/devices/system/cpu/cpuX/cpufreq/scaling_governor
 *
 * @param core_id  CPU core index
 * @return 0 on success, negative error code on failure
 */
__attribute__((visibility("default")))
int lock_cpu_frequency(int core_id);

/**
 * Set CPU frequency governor
 *
 * @param core_id   CPU core index
 * @param governor  Governor mode (GOV_PERFORMANCE, GOV_POWERSAVE, etc.)
 * @return 0 on success, negative error code on failure
 */
__attribute__((visibility("default")))
int set_cpu_governor(int core_id, int governor);

/**
 * Get current time in nanoseconds (CLOCK_MONOTONIC_RAW)
 * Uses clock_gettime bare syscall
 *
 * @return Current time in nanoseconds, 0 on failure
 */
__attribute__((visibility("default")))
uint64_t get_time_ns(void);

/**
 * Get current time using CLOCK_MONOTONIC
 *
 * @return Current time in nanoseconds, 0 on failure
 */
__attribute__((visibility("default")))
uint64_t get_monotonic_time_ns(void);

/**
 * Get current time using CLOCK_BOOTTIME
 *
 * @return Current time in nanoseconds, 0 on failure
 */
__attribute__((visibility("default")))
uint64_t get_boottime_ns(void);

/**
 * High-resolution timing for performance measurement
 * Uses ARM64 cntvct_el0 counter register
 *
 * @return Cycle count
 */
__attribute__((visibility("default")))
uint64_t get_cycle_count(void);

/**
 * Get number of online CPU cores
 *
 * @return Number of cores, or negative errno on failure
 */
__attribute__((visibility("default")))
int get_online_cpu_count(void);

/**
 * Flush CPU cache for a memory region
 *
 * @param addr  Start address
 * @param len   Length in bytes
 */
__attribute__((visibility("default")))
void flush_cpu_cache(void *addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CPU_CONTROL_H */
