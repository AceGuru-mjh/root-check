#include "interrupt_timing_detector.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "InterruptTimingDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

long long InterruptTimingDetector::getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

bool InterruptTimingDetector::bindToCpu(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0;
}

long long InterruptTimingDetector::measureFileReadLatency(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);

    const char *test_file = "/system/build.prop";
    char buf[256];

    for (int i = 0; i < iterations; i++) {
        int fd = syscall(__NR_openat, AT_FDCWD, test_file, O_RDONLY | O_CLOEXEC, 0);
        if (fd < 0) continue;

        long long start = getTimeNs();
        syscall(__NR_read, fd, buf, sizeof(buf));
        long long end = getTimeNs();

        syscall(__NR_close, fd);
        times.push_back(end - start);
    }

    if (times.empty()) return 0;

    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;

    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    return sum / (end_idx - start_idx);
}

long long InterruptTimingDetector::measureFileReadWithInterrupt(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);

    const char *test_file = "/system/build.prop";
    char buf[256];

    for (int i = 0; i < iterations; i++) {
        long long start = getTimeNs();

        int fd = syscall(__NR_openat, AT_FDCWD, test_file, O_RDONLY | O_CLOEXEC, 0);
        if (fd < 0) continue;
        syscall(__NR_read, fd, buf, sizeof(buf));
        syscall(__NR_close, fd);

        long long end = getTimeNs();
        times.push_back(end - start);
    }

    if (times.empty()) return 0;

    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;

    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    return sum / (end_idx - start_idx);
}

InterruptTimingResult InterruptTimingDetector::detect() {
    InterruptTimingResult result;
    result.layer = "第5层增强：中断调度侧信道分析";
    result.detected = false;
    result.avg_baseline_ns = 0;
    result.avg_measured_ns = 0;
    result.deviation_ns = 0;

    if (!bindToCpu(0)) {
        LOGI("绑定 CPU 0 失败，尝试 CPU 1");
        if (!bindToCpu(1)) {
            result.detail = "无法绑定到指定 CPU 核心";
            return result;
        }
    }

    LOGI("已绑定 CPU，开始中断调度侧信道分析");

    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        measureFileReadLatency(10);
    }

    long long baseline = measureFileReadLatency(BASELINE_ITERATIONS);
    LOGI("基线读取时延: %lld ns", baseline);

    long long measured = measureFileReadWithInterrupt(MEASURE_ITERATIONS);
    LOGI("含中断读取时延: %lld ns", measured);

    long long deviation = measured - baseline;

    result.avg_baseline_ns = baseline;
    result.avg_measured_ns = measured;
    result.deviation_ns = deviation;

    if (deviation > THRESHOLD_NS) {
        result.detected = true;
        result.detail = "检测到中断调度异常：含中断时延 " +
            std::to_string(measured) + " ns - 基线 " +
            std::to_string(baseline) + " ns = " +
            std::to_string(deviation) + " ns > 阈值 " +
            std::to_string(THRESHOLD_NS) + " ns，可能存在 Rootkit Hook 导致的额外软中断时延";
        LOGI("检测到中断调度异常: 偏差 %lld ns", deviation);
    } else {
        result.detail = "中断调度正常：偏差 " + std::to_string(deviation) + " ns ≤ 阈值 " +
            std::to_string(THRESHOLD_NS) + " ns";
        LOGI("中断调度正常: 偏差 %lld ns", deviation);
    }

    return result;
}
