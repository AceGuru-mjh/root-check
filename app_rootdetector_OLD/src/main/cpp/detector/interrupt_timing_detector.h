#ifndef ROOTDETECTOR_INTERRUPT_TIMING_DETECTOR_H
#define ROOTDETECTOR_INTERRUPT_TIMING_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct InterruptTimingResult {
    bool detected;
    std::string layer;
    std::string detail;
    std::vector<long long> baseline_times;
    std::vector<long long> measured_times;
    long long avg_baseline_ns;
    long long avg_measured_ns;
    long long deviation_ns;
};

class InterruptTimingDetector {
public:
    static InterruptTimingResult detect();

private:
    static long long getTimeNs();
    static bool bindToCpu(int core_id);
    static long long measureFileReadLatency(int iterations);
    static long long measureFileReadWithInterrupt(int iterations);
    static constexpr int BASELINE_ITERATIONS = 5000;
    static constexpr int MEASURE_ITERATIONS = 5000;
    static constexpr int WARMUP_ITERATIONS = 500;
    static constexpr long long THRESHOLD_NS = 800;
};

#endif
