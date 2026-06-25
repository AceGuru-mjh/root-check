#ifndef ROOTDETECTOR_PAGEFAULT_MONITOR_H
#define ROOTDETECTOR_PAGEFAULT_MONITOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct PagefaultResult {
    bool detected;
    std::string layer;
    std::string detail;
    long long minor_pagefaults;
    long long major_pagefaults;
    long long total_pagefaults;
    long long baseline_minor;
    long long baseline_major;
    long long anomaly_minor_threshold;
    long long anomaly_major_threshold;
};

class PagefaultMonitor {
public:
    static PagefaultResult detect();

private:
    static long long getTimeNs();
    static bool bindToCpu(int core_id);
    static bool readSelfPagefaults(long long &minor, long long &major);
    static long long measurePagefaultBaseline(long long &minor, long long &major);
    static long long measureMemoryAccessPagefaults(long long &minor, long long &major);
    static long long measureKernelRegionAccess(long long &minor, long long &major);

    static constexpr int STABILIZE_ITERATIONS = 5;
    static constexpr int MEMORY_ACCESS_ITERATIONS = 100;
    static constexpr double ANOMALY_MINOR_RATIO = 2.5;
    static constexpr double ANOMALY_MAJOR_RATIO = 2.0;
    static constexpr long long ANOMALY_MINOR_ABSOLUTE = 500;
    static constexpr long long ANOMALY_MAJOR_ABSOLUTE = 50;
};

#endif
