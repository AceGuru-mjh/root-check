#ifndef CPU_BASELINE_DB_H
#define CPU_BASELINE_DB_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

struct CpuBaselineEntry {
    std::string soc_name;
    std::string arch;
    int core_count;
    uint64_t syscall_getpid_ns;
    uint64_t syscall_open_ns;
    uint64_t syscall_read_ns;
    uint64_t cache_latency_ns;
    uint64_t binder_latency_ns;
    uint64_t interrupt_latency_ns;
    double cycles_per_ns;
};

struct DeviceBaseline {
    std::string device_model;
    std::string soc_name;
    std::string kernel_version;
    CpuBaselineEntry measured;
    CpuBaselineEntry *matched_reference;
    bool match_found;
    std::vector<std::string> differences;
};

class CpuBaselineDb {
public:
    static CpuBaselineDb &instance();

    DeviceBaseline determineDevice();
    void calibrateThresholds(DeviceBaseline &baseline);
    uint64_t getThreshold(const std::string &measurement, double multiplier);
    bool loadBuiltinDb();
    bool saveCalibration(const DeviceBaseline &baseline);
    bool loadCalibration(DeviceBaseline &baseline);

    static const int BASELINE_SAMPLE_COUNT = 1000;

private:
    CpuBaselineDb() = default;
    std::vector<CpuBaselineEntry> builtin_db_;
    std::map<std::string, CpuBaselineEntry> calibration_cache_;
    DeviceBaseline current_;

    CpuBaselineEntry *findMatch(const std::string &soc);
    uint64_t measureSyscallLatency(int syscall_num, int iterations);
    uint64_t measureCacheLatency(int iterations);
    uint64_t measureInterruptLatency(int iterations);
};

#endif
