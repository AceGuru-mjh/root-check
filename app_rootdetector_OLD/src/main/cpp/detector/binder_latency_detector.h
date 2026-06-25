#ifndef ROOTDETECTOR_BINDER_LATENCY_DETECTOR_H
#define ROOTDETECTOR_BINDER_LATENCY_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct BinderTransactionResult {
    std::string service_name;
    long long avg_latency_ns;
    long long min_latency_ns;
    long long max_latency_ns;
    double std_dev_ns;
    bool is_anomaly;
    std::string anomaly_reason;
};

struct BinderLatencyResult {
    bool detected;
    std::string layer;
    std::string detail;
    std::vector<BinderTransactionResult> transaction_results;
    long long global_baseline_ns;
    long long global_deviation_ns;
};

class BinderLatencyDetector {
public:
    static BinderLatencyResult detect();

private:
    static long long getTimeNs();
    static bool bindToCpu(int core_id);
    static long long measureBinderTransaction(const char *service_name,
                                              const char *interface_token,
                                              int iterations,
                                              long long &min_latency,
                                              long long &max_latency,
                                              double &std_dev);
    static int openBinderDriver();
    static long long sendBinderCall(int binder_fd, uint32_t code,
                                    const void *data, size_t len);
    static double calculateStdDev(const std::vector<long long> &samples,
                                  long long mean);

    static constexpr int TRANSACTION_ITERATIONS = 2000;
    static constexpr int WARMUP_ITERATIONS = 200;
    static constexpr long long ANOMALY_THRESHOLD_RATIO = 5;
    static constexpr long long BASELINE_ANOMALY_NS = 300000;
};

#endif
