#include "perf_monitor.h"
#include <fstream>

PerfMonitor &PerfMonitor::instance() {
    static PerfMonitor monitor;
    return monitor;
}

void PerfMonitor::beginPlugin(const std::string &name, uint32_t layer_id) {
    ActiveTimer timer;
    timer.start = std::chrono::high_resolution_clock::now();

    uint64_t cycles;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles));
    timer.start_cycles = cycles;

    active_timers_[name] = timer;

    PluginPerfRecord &rec = records_[name];
    rec.plugin_name = name;
    rec.layer_id = layer_id;
}

void PerfMonitor::endPlugin(const std::string &name) {
    auto it = active_timers_.find(name);
    if (it == active_timers_.end()) return;

    auto end = std::chrono::high_resolution_clock::now();
    uint64_t end_cycles;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(end_cycles));

    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - it->second.start).count();

    uint64_t cycle_diff = end_cycles - it->second.start_cycles;

    PluginPerfRecord &rec = records_[name];
    rec.total_time_ns += elapsed;
    rec.cpu_cycles += cycle_diff;
    rec.call_count++;

    if (rec.call_count == 1) {
        rec.min_time_ns = elapsed;
        rec.max_time_ns = elapsed;
    } else {
        if ((uint64_t)elapsed < rec.min_time_ns) rec.min_time_ns = elapsed;
        if ((uint64_t)elapsed > rec.max_time_ns) rec.max_time_ns = elapsed;
    }

    rec.avg_time_ns = rec.total_time_ns / rec.call_count;

    active_timers_.erase(it);
}

void PerfMonitor::recordMemory(const std::string &name, uint64_t bytes) {
    records_[name].memory_used += bytes;
}

PerfReport PerfMonitor::generateReport() {
    PerfReport report;
    report.total_detection_time_ns = 0;
    report.total_cpu_cycles = 0;
    report.peak_memory = 0;
    report.plugin_count = 0;

    for (auto &[name, rec] : records_) {
        report.records.push_back(rec);
        report.total_detection_time_ns += rec.total_time_ns;
        report.total_cpu_cycles += rec.cpu_cycles;
        if (rec.memory_used > report.peak_memory)
            report.peak_memory = rec.memory_used;
        report.plugin_count++;
    }

    return report;
}

void PerfMonitor::reset() {
    records_.clear();
    active_timers_.clear();
}

bool PerfMonitor::exportReport(const std::string &path) {
    PerfReport report = generateReport();
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "=== Performance Report ===\n";
    file << "Total time: " << report.total_detection_time_ns << " ns\n";
    file << "Total cycles: " << report.total_cpu_cycles << "\n";
    file << "Peak memory: " << report.peak_memory << " bytes\n";
    file << "Plugins: " << report.plugin_count << "\n\n";

    for (const auto &rec : report.records) {
        file << rec.plugin_name << " (L" << rec.layer_id << "):\n";
        file << "  Calls: " << rec.call_count << "\n";
        file << "  Total: " << rec.total_time_ns << " ns\n";
        file << "  Avg:   " << rec.avg_time_ns << " ns\n";
        file << "  Min:   " << rec.min_time_ns << " ns\n";
        file << "  Max:   " << rec.max_time_ns << " ns\n";
        file << "  Cycles: " << rec.cpu_cycles << "\n";
        file << "  Memory: " << rec.memory_used << " bytes\n\n";
    }

    return true;
}
