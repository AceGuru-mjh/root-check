#ifndef PERF_MONITOR_H
#define PERF_MONITOR_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <map>

struct PluginPerfRecord {
    std::string plugin_name;
    uint32_t layer_id;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    uint64_t avg_time_ns;
    uint64_t cpu_cycles;
    uint32_t call_count;
    uint64_t memory_used;
};

struct PerfReport {
    std::vector<PluginPerfRecord> records;
    uint64_t total_detection_time_ns;
    uint64_t total_cpu_cycles;
    uint64_t peak_memory;
    int plugin_count;
};

class PerfMonitor {
public:
    static PerfMonitor &instance();

    void beginPlugin(const std::string &name, uint32_t layer_id);
    void endPlugin(const std::string &name);
    void recordMemory(const std::string &name, uint64_t bytes);

    PerfReport generateReport();
    void reset();
    bool exportReport(const std::string &path);

private:
    PerfMonitor() = default;

    struct ActiveTimer {
        std::chrono::high_resolution_clock::time_point start;
        uint64_t start_cycles;
    };

    std::map<std::string, PluginPerfRecord> records_;
    std::map<std::string, ActiveTimer> active_timers_;
};

#endif
