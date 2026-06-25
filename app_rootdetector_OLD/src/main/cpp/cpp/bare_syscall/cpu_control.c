/*
 * cpu_control.c - CPU core binding and frequency control implementation
 *
 * Domain D: 底层裸系统汇编抽象层
 * All operations use bare syscalls, NO libc dependency
 */

#include "cpu_control.h"
#include "syscall_arm64.h"

/* CPU set structure matching kernel's cpu_set_t */
typedef struct {
    unsigned long __bits[MAX_CPU_CORES / (sizeof(unsigned long) * 8)];
} cpu_set_raw_t;

/* Helper macros for CPU set manipulation */
#define CPU_SET_RAW_ZERO(set) \
    do { \
        for (int _i = 0; _i < (int)(sizeof((set)->__bits) / sizeof((set)->__bits[0])); _i++) \
            (set)->__bits[_i] = 0; \
    } while (0)

#define CPU_SET_RAW_SET(cpu, set) \
    ((set)->__bits[(cpu) / (sizeof(unsigned long) * 8)] |= \
        (1UL << ((cpu) % (sizeof(unsigned long) * 8))))

#define CPU_SET_RAW_CLR(cpu, set) \
    ((set)->__bits[(cpu) / (sizeof(unsigned long) * 8)] &= \
        ~(1UL << ((cpu) % (sizeof(unsigned long) * 8))))

/* String helper - no libc allowed */
static int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == *b);
}

/* Integer to string conversion - no libc allowed */
static int int_to_str(int val, char *buf, int bufsize) {
    int i = 0;
    int neg = 0;
    
    if (val < 0) {
        neg = 1;
        val = -val;
    }
    
    if (val == 0) {
        if (bufsize < 2) return -1;
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    /* Convert digits in reverse */
    char tmp[16];
    int len = 0;
    while (val > 0 && len < 15) {
        tmp[len++] = '0' + (val % 10);
        val /= 10;
    }
    
    /* Check buffer space */
    int needed = len + neg + 1;
    if (bufsize < needed) return -1;
    
    /* Add negative sign */
    if (neg) {
        buf[i++] = '-';
    }
    
    /* Reverse digits */
    for (int j = len - 1; j >= 0; j--) {
        buf[i++] = tmp[j];
    }
    
    buf[i] = '\0';
    return i;
}

/* String concatenation - no libc allowed */
static int str_cat(char *dst, const char *src, int dstsize) {
    int i = 0;
    while (dst[i] && i < dstsize - 1) i++;
    
    int j = 0;
    while (src[j] && i < dstsize - 1) {
        dst[i++] = src[j++];
    }
    dst[i] = '\0';
    return i;
}

__attribute__((visibility("default")))
int bind_to_cpu(int core_id) {
    if (core_id < 0 || core_id >= MAX_CPU_CORES) {
        return CPU_ERR_INVALID_CORE;
    }
    
    cpu_set_raw_t set;
    CPU_SET_RAW_ZERO(&set);
    CPU_SET_RAW_SET(core_id, &set);
    
    long ret = bare_sched_setaffinity(0, sizeof(set), &set);
    if (bare_is_error(ret)) {
        return CPU_ERR_SYSCALL_FAIL;
    }
    
    return CPU_OK;
}

__attribute__((visibility("default")))
int bind_to_cpu_set(const int *mask, int count) {
    if (!mask || count <= 0 || count > MAX_CPU_CORES) {
        return CPU_ERR_INVALID_CORE;
    }
    
    cpu_set_raw_t set;
    CPU_SET_RAW_ZERO(&set);
    
    for (int i = 0; i < count; i++) {
        if (mask[i] >= 0 && mask[i] < MAX_CPU_CORES) {
            CPU_SET_RAW_SET(mask[i], &set);
        }
    }
    
    long ret = bare_sched_setaffinity(0, sizeof(set), &set);
    if (bare_is_error(ret)) {
        return CPU_ERR_SYSCALL_FAIL;
    }
    
    return CPU_OK;
}

__attribute__((visibility("default")))
int lock_cpu_frequency(int core_id) {
    return set_cpu_governor(core_id, GOV_PERFORMANCE);
}

__attribute__((visibility("default")))
int set_cpu_governor(int core_id, int governor) {
    if (core_id < 0 || core_id >= MAX_CPU_CORES) {
        return CPU_ERR_INVALID_CORE;
    }
    
    /* Build path: /sys/devices/system/cpu/cpuX/cpufreq/scaling_governor */
    char path[128];
    int pos = 0;
    const char *prefix = "/sys/devices/system/cpu/cpu";
    const char *suffix = "/cpufreq/scaling_governor";
    
    /* Copy prefix */
    for (int i = 0; prefix[i] && pos < 127; i++) {
        path[pos++] = prefix[i];
    }
    
    /* Add core number */
    char num[8];
    int len = int_to_str(core_id, num, sizeof(num));
    if (len < 0) return CPU_ERR_INVALID_CORE;
    for (int i = 0; i < len && pos < 127; i++) {
        path[pos++] = num[i];
    }
    
    /* Copy suffix */
    for (int i = 0; suffix[i] && pos < 127; i++) {
        path[pos++] = suffix[i];
    }
    path[pos] = '\0';
    
    /* Select governor string */
    const char *gov_str;
    switch (governor) {
        case GOV_PERFORMANCE:  gov_str = "performance\n"; break;
        case GOV_POWERSAVE:    gov_str = "powersave\n"; break;
        case GOV_USERSPACE:    gov_str = "userspace\n"; break;
        case GOV_ONDEMAND:     gov_str = "ondemand\n"; break;
        case GOV_CONSERVATIVE: gov_str = "conservative\n"; break;
        case GOV_SCHEDUTIL:    gov_str = "schedutil\n"; break;
        default:               return CPU_ERR_GOVERNOR_FAIL;
    }
    
    /* Open file for writing */
    long fd = bare_openat(BARE_AT_FDCWD, path, BARE_O_WRONLY, 0);
    if (bare_is_error(fd)) {
        return CPU_ERR_GOVERNOR_FAIL;
    }
    
    /* Write governor string */
    int gov_len = 0;
    while (gov_str[gov_len]) gov_len++;
    
    long written = bare_write((int)fd, gov_str, gov_len);
    bare_close((int)fd);
    
    if (bare_is_error(written) || written != gov_len) {
        return CPU_ERR_GOVERNOR_FAIL;
    }
    
    return CPU_OK;
}

__attribute__((visibility("default")))
uint64_t get_time_ns(void) {
    struct kernel_timespec ts;
    long ret = bare_clock_gettime(BARE_CLOCK_MONOTONIC_RAW, &ts);
    if (bare_is_error(ret)) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

__attribute__((visibility("default")))
uint64_t get_monotonic_time_ns(void) {
    struct kernel_timespec ts;
    long ret = bare_clock_gettime(BARE_CLOCK_MONOTONIC, &ts);
    if (bare_is_error(ret)) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

__attribute__((visibility("default")))
uint64_t get_boottime_ns(void) {
    struct kernel_timespec ts;
    long ret = bare_clock_gettime(BARE_CLOCK_BOOTTIME, &ts);
    if (bare_is_error(ret)) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

__attribute__((visibility("default")))
uint64_t get_cycle_count(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

__attribute__((visibility("default")))
int get_online_cpu_count(void) {
    /* Read /sys/devices/system/cpu/online */
    const char *path = "/sys/devices/system/cpu/online";
    long fd = bare_openat(BARE_AT_FDCWD, path, BARE_O_RDONLY, 0);
    if (bare_is_error(fd)) {
        return CPU_ERR_SYSCALL_FAIL;
    }
    
    char buf[64];
    long nread = bare_read((int)fd, buf, sizeof(buf) - 1);
    bare_close((int)fd);
    
    if (bare_is_error(nread) || nread <= 0) {
        return CPU_ERR_SYSCALL_FAIL;
    }
    
    buf[nread] = '\0';
    
    /* Parse format like "0-7" or "0-3,5-7" */
    int count = 0;
    int i = 0;
    while (buf[i]) {
        /* Find start of range */
        int start = 0;
        while (buf[i] >= '0' && buf[i] <= '9') {
            start = start * 10 + (buf[i] - '0');
            i++;
        }
        
        if (buf[i] == '-') {
            i++;
            int end = 0;
            while (buf[i] >= '0' && buf[i] <= '9') {
                end = end * 10 + (buf[i] - '0');
                i++;
            }
            count += (end - start + 1);
        } else if (buf[i - 1] >= '0' && buf[i - 1] <= '9') {
            /* Single CPU */
            count++;
        }
        
        /* Skip comma or whitespace */
        while (buf[i] == ',' || buf[i] == ' ' || buf[i] == '\n') {
            i++;
        }
    }
    
    return count > 0 ? count : CPU_ERR_SYSCALL_FAIL;
}

__attribute__((visibility("default")))
void flush_cpu_cache(void *addr, size_t len) {
    __builtin___clear_cache((char *)addr, (char *)addr + len);
}
