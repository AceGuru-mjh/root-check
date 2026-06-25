#include "cache_timing_detector.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <android/log.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "CacheTimingDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

long long CacheTimingDetector::getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

long long CacheTimingDetector::getCycleCount() {
#ifdef __aarch64__
    uint64_t cnt;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(cnt));
    return (long long)cnt;
#else
    return getTimeNs();
#endif
}

void CacheTimingDetector::flushCache() {
    __asm__ __volatile__("dsb sy" ::: "memory");
#ifdef __aarch64__
    __asm__ __volatile__("isb" ::: "memory");
#endif
}

void CacheTimingDetector::memoryBarrier() {
    __asm__ __volatile__("dmb sy" ::: "memory");
}

long long CacheTimingDetector::measureColdCacheAccess(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);

    size_t size = BUFFER_SIZE;
    void *buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (buf == MAP_FAILED) {
        LOGI("mmap 失败");
        return 0;
    }

    memset(buf, 0xAA, size);

    for (int i = 0; i < iterations; i++) {
        flushCache();

        long long start = getTimeNs();
        volatile char sum = 0;
        for (size_t j = 0; j < size; j += CACHE_LINE_SIZE * 4) {
            sum += ((volatile char *)buf)[j];
        }
        memoryBarrier();
        long long end = getTimeNs();
        (void)sum;

        times.push_back(end - start);
    }

    munmap(buf, size);

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

long long CacheTimingDetector::measureHotCacheAccess(int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);

    size_t size = BUFFER_SIZE;
    void *buf = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (buf == MAP_FAILED) {
        LOGI("mmap 失败");
        return 0;
    }

    memset(buf, 0xBB, size);

    volatile char warm = 0;
    for (int i = 0; i < 10; i++) {
        for (size_t j = 0; j < size; j += CACHE_LINE_SIZE * 4) {
            warm += ((volatile char *)buf)[j];
        }
    }
    (void)warm;

    for (int i = 0; i < iterations; i++) {
        long long start = getTimeNs();
        volatile char sum = 0;
        for (size_t j = 0; j < size; j += CACHE_LINE_SIZE * 4) {
            sum += ((volatile char *)buf)[j];
        }
        memoryBarrier();
        long long end = getTimeNs();
        (void)sum;

        times.push_back(end - start);
    }

    munmap(buf, size);

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

CacheTimingResult CacheTimingDetector::detect() {
    CacheTimingResult result;
    result.layer = "第5层增强：Cache 时序侧信道分析";
    result.detected = false;
    result.cold_cache_ns = 0;
    result.hot_cache_ns = 0;
    result.ratio = 0;
    result.threshold_ns = 0;

    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        measureHotCacheAccess(5);
    }

    long long hot = measureHotCacheAccess(HOT_ITERATIONS);
    LOGI("热缓存访问时延: %lld ns", hot);

    long long cold = measureColdCacheAccess(COLD_ITERATIONS);
    LOGI("冷缓存访问时延: %lld ns", cold);

    result.cold_cache_ns = cold;
    result.hot_cache_ns = hot;

    if (hot == 0 || cold == 0) {
        result.detail = "缓存测量失败，无法完成检测";
        return result;
    }

    long long cold_vs_hot_ratio = cold / hot;
    result.ratio = cold_vs_hot_ratio;
    result.threshold_ns = hot * ANOMALY_RATIO_THRESHOLD;

    LOGI("冷热缓存比值: %lld, 异常阈值: %lld ns", cold_vs_hot_ratio, result.threshold_ns);

    bool ratio_anomaly = cold_vs_hot_ratio < ANOMALY_RATIO_THRESHOLD;
    bool abs_anomaly = (cold - hot) < ABSOLUTE_THRESHOLD_NS;

    if (ratio_anomaly || abs_anomaly) {
        result.detected = true;
        std::string reason;
        if (ratio_anomaly) {
            reason += "冷热缓存比值异常 " + std::to_string(cold_vs_hot_ratio) +
                      " < 阈值 " + std::to_string(ANOMALY_RATIO_THRESHOLD);
        }
        if (abs_anomaly) {
            if (!reason.empty()) reason += "；";
            reason += "绝对差值异常 " + std::to_string(cold - hot) +
                      " ns < 阈值 " + std::to_string(ABSOLUTE_THRESHOLD_NS) + " ns";
        }
        result.detail = "检测到 CPU 缓存一致性异常，可能存在内核级 Hook：" + reason +
                        "，内核 Hook 破坏了 CPU 缓存一致性导致纳秒级时序可测差异";
        LOGI("检测到缓存一致性异常: %s", reason.c_str());
    } else {
        result.detail = "缓存时序正常：冷缓存 " + std::to_string(cold) +
                        " ns，热缓存 " + std::to_string(hot) +
                        " ns，比值 " + std::to_string(cold_vs_hot_ratio);
        LOGI("缓存时序正常: 比值 %lld", cold_vs_hot_ratio);
    }

    return result;
}
