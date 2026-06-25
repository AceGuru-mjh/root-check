#include "pagefault_monitor.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <android/log.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

#define LOG_TAG "PagefaultMonitor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

long long PagefaultMonitor::getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

bool PagefaultMonitor::bindToCpu(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0;
}

bool PagefaultMonitor::readSelfPagefaults(long long &minor, long long &major) {
    struct rusage usage;
    if (syscall(__NR_getrusage, RUSAGE_SELF, &usage) == 0) {
        minor = (long long)usage.ru_minflt;
        major = (long long)usage.ru_majflt;
        return true;
    }

    int fd = syscall(__NR_openat, AT_FDCWD, "/proc/self/stat", O_RDONLY, 0);
    if (fd < 0) return false;

    char buf[1024] = {};
    syscall(__NR_read, fd, buf, sizeof(buf) - 1);
    syscall(__NR_close, fd);

    int field = 0;
    char *p = buf;
    while (*p) {
        if (*p == ' ') field++;
        if (field == 9) {
            minor = atoll(p + 1);
        }
        if (field == 11) {
            major = atoll(p + 1);
            break;
        }
        p++;
    }
    return true;
}

long long PagefaultMonitor::measurePagefaultBaseline(long long &out_minor, long long &out_major) {
    long long minor_before, major_before;
    long long minor_after, major_after;
    long long total_minor = 0, total_major = 0;

    for (int i = 0; i < STABILIZE_ITERATIONS; i++) {
        readSelfPagefaults(minor_before, major_before);

        volatile char sink = 0;
        for (int j = 0; j < 10000; j++) {
            sink += (char)(j & 0xFF);
        }
        (void)sink;

        readSelfPagefaults(minor_after, major_after);
        total_minor += (minor_after - minor_before);
        total_major += (major_after - major_before);
    }

    out_minor = total_minor / STABILIZE_ITERATIONS;
    out_major = total_major / STABILIZE_ITERATIONS;
    return out_minor + out_major;
}

long long PagefaultMonitor::measureMemoryAccessPagefaults(long long &out_minor, long long &out_major) {
    long long minor_before, major_before;
    long long minor_after, major_after;
    long long total_minor = 0, total_major = 0;

    size_t region_size = 64 * 1024 * 1024;

    for (int i = 0; i < MEMORY_ACCESS_ITERATIONS; i++) {
        readSelfPagefaults(minor_before, major_before);

        void *buf = mmap(NULL, region_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (buf == MAP_FAILED) continue;

        memset(buf, 0xCC, region_size);

        volatile char sum = 0;
        for (size_t j = 0; j < region_size; j += 4096) {
            sum += ((volatile char *)buf)[j];
        }
        (void)sum;

        munmap(buf, region_size);

        readSelfPagefaults(minor_after, major_after);
        total_minor += (minor_after - minor_before);
        total_major += (major_after - major_before);
    }

    out_minor = total_minor / MEMORY_ACCESS_ITERATIONS;
    out_major = total_major / MEMORY_ACCESS_ITERATIONS;
    return out_minor + out_major;
}

long long PagefaultMonitor::measureKernelRegionAccess(long long &out_minor, long long &out_major) {
    long long minor_before, major_before;
    long long minor_after, major_after;

    readSelfPagefaults(minor_before, major_before);

    const char *kernel_paths[] = {
        "/proc/kallsyms",
        "/proc/modules",
        "/sys/kernel/notes",
        "/proc/mem",
        "/dev/kmem",
    };

    char buf[256];
    for (auto path : kernel_paths) {
        int fd = syscall(__NR_openat, AT_FDCWD, path, O_RDONLY, 0);
        if (fd >= 0) {
            syscall(__NR_read, fd, buf, sizeof(buf));
            syscall(__NR_close, fd);
        }
    }

    size_t probe_size = 4 * 1024 * 1024;
    void *probe = mmap(NULL, probe_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (probe != MAP_FAILED) {
        memset(probe, 0xDD, probe_size);
        volatile char sum = 0;
        for (size_t i = 0; i < probe_size; i += 4096) {
            sum += ((volatile char *)probe)[i];
        }
        (void)sum;
        munmap(probe, probe_size);
    }

    readSelfPagefaults(minor_after, major_after);
    out_minor = minor_after - minor_before;
    out_major = major_after - major_before;
    return out_minor + out_major;
}

PagefaultResult PagefaultMonitor::detect() {
    PagefaultResult result;
    result.layer = "第5层增强：PageFault 缺页异常监控";
    result.detected = false;
    result.minor_pagefaults = 0;
    result.major_pagefaults = 0;
    result.total_pagefaults = 0;
    result.baseline_minor = 0;
    result.baseline_major = 0;
    result.anomaly_minor_threshold = 0;
    result.anomaly_major_threshold = 0;

    if (!bindToCpu(0)) {
        bindToCpu(1);
    }

    LOGI("开始 PageFault 缺页异常监控");

    long long base_minor = 0, base_major = 0;
    measurePagefaultBaseline(base_minor, base_major);
    result.baseline_minor = base_minor;
    result.baseline_major = base_major;

    result.anomaly_minor_threshold = (long long)(base_minor * ANOMALY_MINOR_RATIO);
    if (result.anomaly_minor_threshold < ANOMALY_MINOR_ABSOLUTE) {
        result.anomaly_minor_threshold = ANOMALY_MINOR_ABSOLUTE;
    }
    result.anomaly_major_threshold = (long long)(base_major * ANOMALY_MAJOR_RATIO);
    if (result.anomaly_major_threshold < ANOMALY_MAJOR_ABSOLUTE) {
        result.anomaly_major_threshold = ANOMALY_MAJOR_ABSOLUTE;
    }

    LOGI("基线缺页: minor=%lld, major=%lld", base_minor, base_major);
    LOGI("异常阈值: minor=%lld, major=%lld",
         result.anomaly_minor_threshold, result.anomaly_major_threshold);

    long long mem_minor = 0, mem_major = 0;
    measureMemoryAccessPagefaults(mem_minor, mem_major);
    LOGI("内存访问缺页: minor=%lld, major=%lld", mem_minor, mem_major);

    long long kern_minor = 0, kern_major = 0;
    measureKernelRegionAccess(kern_minor, kern_major);
    LOGI("内核区域访问缺页: minor=%lld, major=%lld", kern_minor, kern_major);

    result.minor_pagefaults = mem_minor + kern_minor;
    result.major_pagefaults = mem_major + kern_major;
    result.total_pagefaults = result.minor_pagefaults + result.major_pagefaults;

    bool minor_anomaly = result.minor_pagefaults > result.anomaly_minor_threshold;
    bool major_anomaly = result.major_pagefaults > result.anomaly_major_threshold;

    if (minor_anomaly || major_anomaly) {
        result.detected = true;
        std::string reason;
        if (minor_anomaly) {
            reason += "minor fault " + std::to_string(result.minor_pagefaults) +
                      " > 阈值 " + std::to_string(result.anomaly_minor_threshold);
        }
        if (major_anomaly) {
            if (!reason.empty()) reason += "；";
            reason += "major fault " + std::to_string(result.major_pagefaults) +
                      " > 阈值 " + std::to_string(result.anomaly_major_threshold);
        }
        result.detail = "检测到 PageFault 异常：" + reason +
                        "，可能存在内存注入或内核隐藏行为";
        LOGI("检测到 PageFault 异常: %s", reason.c_str());
    } else {
        result.detail = "缺页异常统计正常：minor=" + std::to_string(result.minor_pagefaults) +
                        " (阈值 " + std::to_string(result.anomaly_minor_threshold) +
                        "), major=" + std::to_string(result.major_pagefaults) +
                        " (阈值 " + std::to_string(result.anomaly_major_threshold) + ")";
        LOGI("PageFault 正常");
    }

    return result;
}
