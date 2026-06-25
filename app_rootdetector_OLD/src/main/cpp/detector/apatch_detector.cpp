#include "apatch_detector.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <android/log.h>

#define LOG_TAG "APatchDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define ITERATIONS 10000
#define THRESHOLD_NS 500

long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

DetectionResult APatchDetector::detect() {
    DetectionResult result;
    result.layer = "第5层：APatch 侧信道检测";
    result.detected = false;
    
    // 绑定到 CPU 核心 0
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    
    // 打开 /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        result.detail = "无法打开 /dev/null";
        return result;
    }
    
    // 预热
    for (int i = 0; i < 100; i++) {
        ftruncate(fd, 0xffff);
        ftruncate(fd, 0x10000);
    }
    
    // 测量 T1: 触发 APatch 后门代码
    long long start = get_time_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        ftruncate(fd, 0xffff);
    }
    long long t1 = (get_time_ns() - start) / ITERATIONS;
    
    // 测量 T2: 不触发 APatch 后门代码
    start = get_time_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        ftruncate(fd, 0x10000);
    }
    long long t2 = (get_time_ns() - start) / ITERATIONS;
    
    close(fd);
    
    long long diff = t1 - t2;
    
    LOGI("APatch 检测: T1=%lld ns, T2=%lld ns, 差值=%lld ns", t1, t2, diff);
    
    if (diff > THRESHOLD_NS) {
        result.detected = true;
        result.detail = "检测到 APatch：时间差 " + std::to_string(diff) + " ns > 阈值 " + std::to_string(THRESHOLD_NS) + " ns";
    } else {
        result.detail = "未检测到 APatch：时间差 " + std::to_string(diff) + " ns";
    }
    
    return result;
}
