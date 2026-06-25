#ifndef ROOTDETECTOR_CACHE_TIMING_DETECTOR_H
#define ROOTDETECTOR_CACHE_TIMING_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct CacheTimingResult {
    bool detected;
    std::string layer;
    std::string detail;
    long long cold_cache_ns;
    long long hot_cache_ns;
    long long ratio;
    long long threshold_ns;
};

class CacheTimingDetector {
public:
    static CacheTimingResult detect();

private:
    static long long getTimeNs();
    static long long getCycleCount();
    static void flushCache();
    static long long measureColdCacheAccess(int iterations);
    static long long measureHotCacheAccess(int iterations);
    static void memoryBarrier();

    static constexpr int CACHE_LINE_SIZE = 64;
    static constexpr int BUFFER_SIZE = 4 * 1024 * 1024;
    static constexpr int COLD_ITERATIONS = 1000;
    static constexpr int HOT_ITERATIONS = 10000;
    static constexpr int WARMUP_ITERATIONS = 200;
    static constexpr long long ANOMALY_RATIO_THRESHOLD = 3;
    static constexpr long long ABSOLUTE_THRESHOLD_NS = 200;
};

#endif
