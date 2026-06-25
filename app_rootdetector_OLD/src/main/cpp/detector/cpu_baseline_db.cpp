#include "cpu_baseline_db.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <algorithm>

CpuBaselineDb &CpuBaselineDb::instance() {
    static CpuBaselineDb db;
    return db;
}

bool CpuBaselineDb::loadBuiltinDb() {
    builtin_db_.clear();

    CpuBaselineEntry snapdragon_888 = {
        "Snapdragon 888", "arm64", 8,
        120, 450, 320, 85, 150000, 600, 2.4
    };
    builtin_db_.push_back(snapdragon_888);

    CpuBaselineEntry snapdragon_8gen1 = {
        "Snapdragon 8 Gen 1", "arm64", 8,
        110, 420, 300, 80, 140000, 580, 2.8
    };
    builtin_db_.push_back(snapdragon_8gen1);

    CpuBaselineEntry snapdragon_8gen2 = {
        "Snapdragon 8 Gen 2", "arm64", 8,
        100, 400, 280, 75, 130000, 550, 3.0
    };
    builtin_db_.push_back(snapdragon_8gen2);

    CpuBaselineEntry snapdragon_8gen3 = {
        "Snapdragon 8 Gen 3", "arm64", 8,
        90, 380, 260, 70, 120000, 520, 3.2
    };
    builtin_db_.push_back(snapdragon_8gen3);

    CpuBaselineEntry dimensity_9000 = {
        "Dimensity 9000", "arm64", 8,
        115, 440, 310, 82, 145000, 590, 2.5
    };
    builtin_db_.push_back(dimensity_9000);

    CpuBaselineEntry dimensity_9200 = {
        "Dimensity 9200", "arm64", 8,
        105, 410, 290, 78, 135000, 560, 2.9
    };
    builtin_db_.push_back(dimensity_9200);

    CpuBaselineEntry exynos_2200 = {
        "Exynos 2200", "arm64", 8,
        118, 460, 330, 88, 155000, 620, 2.3
    };
    builtin_db_.push_back(exynos_2200);

    CpuBaselineEntry kirin_9000 = {
        "Kirin 9000", "arm64", 8,
        108, 430, 305, 80, 142000, 570, 2.6
    };
    builtin_db_.push_back(kirin_9000);

    return true;
}

CpuBaselineEntry *CpuBaselineDb::findMatch(const std::string &soc) {
    for (auto &entry : builtin_db_) {
        if (soc.find(entry.soc_name) != std::string::npos ||
            entry.soc_name.find(soc) != std::string::npos) {
            return &entry;
        }
    }
    return nullptr;
}

uint64_t CpuBaselineDb::measureSyscallLatency(int syscall_num, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        syscall(syscall_num, 0, 0, 0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / iterations;
}

uint64_t CpuBaselineDb::measureCacheLatency(int iterations) {
    const size_t buf_size = 1024 * 1024;
    char *buf = new char[buf_size];
    std::memset(buf, 0, buf_size);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        for (size_t j = 0; j < buf_size; j += 64) {
            buf[j] += 1;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    delete[] buf;

    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / iterations;
}

uint64_t CpuBaselineDb::measureInterruptLatency(int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        std::this_thread::yield();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / iterations;
}

DeviceBaseline CpuBaselineDb::determineDevice() {
    DeviceBaseline baseline;
    baseline.match_found = false;

    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::string hardware;
    while (std::getline(cpuinfo, line)) {
        if (line.find("Hardware") != std::string::npos) {
            hardware = line.substr(line.find(':') + 1);
            hardware.erase(0, hardware.find_first_not_of(' '));
        }
    }

    baseline.soc_name = hardware;
    baseline.measured.soc_name = hardware;

    if (builtin_db_.empty()) loadBuiltinDb();

    CpuBaselineEntry *match = findMatch(hardware);
    if (match) {
        baseline.matched_reference = match;
        baseline.match_found = true;
    }

    baseline.measured.syscall_getpid_ns = measureSyscallLatency(172, 1000);
    baseline.measured.syscall_open_ns = measureSyscallLatency(56, 500);
    baseline.measured.cache_latency_ns = measureCacheLatency(100);
    baseline.measured.interrupt_latency_ns = measureInterruptLatency(1000);

    if (match) {
        baseline.differences.push_back(
            "getpid: measured=" + std::to_string(baseline.measured.syscall_getpid_ns) +
            "ns ref=" + std::to_string(match->syscall_getpid_ns) + "ns");
    }

    current_ = baseline;
    return baseline;
}

void CpuBaselineDb::calibrateThresholds(DeviceBaseline &baseline) {
    if (baseline.match_found && baseline.matched_reference) {
        double ratio = (double)baseline.measured.syscall_getpid_ns /
                       (double)baseline.matched_reference->syscall_getpid_ns;

        baseline.matched_reference->cache_latency_ns =
            (uint64_t)(baseline.matched_reference->cache_latency_ns * ratio);
        baseline.matched_reference->interrupt_latency_ns =
            (uint64_t)(baseline.matched_reference->interrupt_latency_ns * ratio);
    }
}

uint64_t CpuBaselineDb::getThreshold(const std::string &measurement, double multiplier) {
    if (current_.match_found && current_.matched_reference) {
        if (measurement == "syscall") {
            return (uint64_t)(current_.measured.syscall_getpid_ns * multiplier);
        } else if (measurement == "cache") {
            return (uint64_t)(current_.measured.cache_latency_ns * multiplier);
        } else if (measurement == "interrupt") {
            return (uint64_t)(current_.measured.interrupt_latency_ns * multiplier);
        }
    }
    return (uint64_t)(200 * multiplier);
}

bool CpuBaselineDb::saveCalibration(const DeviceBaseline &baseline) {
    std::ofstream file("/data/local/tmp/.cpu_baseline");
    if (!file.is_open()) return false;
    file << baseline.soc_name << "\n";
    file << baseline.measured.syscall_getpid_ns << "\n";
    file << baseline.measured.cache_latency_ns << "\n";
    file << baseline.measured.interrupt_latency_ns << "\n";
    return true;
}

bool CpuBaselineDb::loadCalibration(DeviceBaseline &baseline) {
    std::ifstream file("/data/local/tmp/.cpu_baseline");
    if (!file.is_open()) return false;
    std::string line;
    std::getline(file, baseline.soc_name);
    std::getline(file, line);
    baseline.measured.syscall_getpid_ns = std::stoull(line);
    std::getline(file, line);
    baseline.measured.cache_latency_ns = std::stoull(line);
    std::getline(file, line);
    baseline.measured.interrupt_latency_ns = std::stoull(line);
    return true;
}
