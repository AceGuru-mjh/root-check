#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cinttypes>
#include <cstdlib>
#include <fcntl.h>

// ─── Constants ────────────────────────────────────────────

#define MMAP_PROT_READ     0x1
#define MMAP_PROT_WRITE    0x2
#define MMAP_MAP_PRIVATE   0x02
#define MMAP_MAP_ANONYMOUS 0x20
#define MMAP_MAP_POPULATE  0x08000

#define O_RDONLY  0
#define O_CLOEXEC 0x80000

// Binder
#define BINDER_DRIVER "/dev/binder"
struct binder_write_read {
    uint64_t write_size; uint64_t write_consumed;
    uint64_t write_buffer; uint64_t read_size;
    uint64_t read_consumed; uint64_t read_buffer;
};
// _IOWR('b',1,sizeof(bwr)=48) on arm64
#define BINDER_WRITE_READ 0xC0620030ULL
// _IOWR('b',9,sizeof(binder_version)=8) on arm64
#define BINDER_VERSION 0xC0629008ULL
struct binder_version {
    signed long protocol_version;
};

// Cache timing
static constexpr int CACHE_BUF_SIZE = 4 * 1024 * 1024;
static constexpr int CACHE_WARMUP = 200;
static constexpr int CACHE_COLD_ITER = 1000;
static constexpr int CACHE_HOT_ITER = 10000;
static constexpr long long CACHE_RATIO_THRESH = 3;
static constexpr long long CACHE_ABS_THRESH_NS = 200;

// Interrupt timing
static constexpr int IRQ_BASELINE_ITER = 5000;
static constexpr int IRQ_MEASURE_ITER = 5000;
static constexpr long long IRQ_THRESHOLD_NS = 800;

// Binder latency
static constexpr int BINDER_ITER = 2000;
static constexpr long long BINDER_RATIO_THRESH = 5;
static constexpr long long BINDER_ABS_NS = 300000;

// Pagefault
static constexpr int PF_MEM_ITER = 100;
static constexpr double PF_MINOR_RATIO = 2.5;
static constexpr double PF_MAJOR_RATIO = 2.0;
static constexpr long long PF_MINOR_ABS = 500;
static constexpr long long PF_MAJOR_ABS = 50;

// ─── Helpers ──────────────────────────────────────────────

static void bind_cpu(int core) {
    uint64_t mask = 1ULL << core;
    bs_sched_setaffinity(0, 8, &mask);
}

static int64_t read_stat_field(int field) {
    int fd = (int)bs_openat(-100, "/proc/self/stat", O_RDONLY, 0);
    if (fd < 0) return 0;
    char buf[512] = {};
    int64_t n = bs_read(fd, buf, 511);
    bs_close(fd);
    if (n <= 0) return 0;
    int cur = 0;
    char *p = buf;
    while (*p) {
        if (*p == ' ') { cur++; }
        if (cur == field) return (int64_t)strtoll(p + 1, nullptr, 10);
        p++;
    }
    return 0;
}

// ─── 1. Cache Timing Detector ─────────────────────────────

struct CacheResult { bool anomaly; int64_t hot_ns; int64_t cold_ns; int64_t ratio; };

static CacheResult detect_cache_timing() {
    CacheResult r = {false, 0, 0, 0};

    void *buf = (void*)(intptr_t)bs_mmap(nullptr, CACHE_BUF_SIZE,
        MMAP_PROT_READ | MMAP_PROT_WRITE,
        MMAP_MAP_PRIVATE | MMAP_MAP_ANONYMOUS | MMAP_MAP_POPULATE,
        -1, 0);
    if ((int64_t)(intptr_t)buf < 0 && buf != nullptr) return r;

    for (int w = 0; w < CACHE_WARMUP; w++) {
        volatile char sink = 0;
        for (size_t j = 0; j < (size_t)CACHE_BUF_SIZE; j += 64) {
            sink += ((volatile char*)buf)[j];
        }
        (void)sink;
    }

    // Hot cache
    int64_t hot_sum = 0;
    for (int i = 0; i < CACHE_HOT_ITER; i++) {
        uint64_t t0 = bs_clock_ns();
        volatile char sum = 0;
        for (size_t j = 0; j < (size_t)CACHE_BUF_SIZE; j += 256) {
            sum += ((volatile char*)buf)[j];
        }
        (void)sum;
        uint64_t t1 = bs_clock_ns();
        hot_sum += (int64_t)(t1 - t0);
    }
    r.hot_ns = hot_sum / CACHE_HOT_ITER;

    // Cold cache - flush via munmap+mmap each iteration
    int64_t cold_sum = 0;
    for (int i = 0; i < CACHE_COLD_ITER; i++) {
        bs_munmap(buf, CACHE_BUF_SIZE);
        buf = (void*)(intptr_t)bs_mmap(nullptr, CACHE_BUF_SIZE,
            MMAP_PROT_READ | MMAP_PROT_WRITE,
            MMAP_MAP_PRIVATE | MMAP_MAP_ANONYMOUS | MMAP_MAP_POPULATE,
            -1, 0);
        if ((int64_t)(intptr_t)buf < 0 && buf != nullptr) break;
        uint64_t t0 = bs_clock_ns();
        volatile char sum = 0;
        for (size_t j = 0; j < (size_t)CACHE_BUF_SIZE; j += 256) {
            sum += ((volatile char*)buf)[j];
        }
        (void)sum;
        uint64_t t1 = bs_clock_ns();
        cold_sum += (int64_t)(t1 - t0);
    }
    bs_munmap(buf, CACHE_BUF_SIZE);
    r.cold_ns = cold_sum / CACHE_COLD_ITER;

    if (r.hot_ns > 0 && r.cold_ns > 0) {
        r.ratio = r.cold_ns / r.hot_ns;
        r.anomaly = (r.ratio < CACHE_RATIO_THRESH) ||
                     ((r.cold_ns - r.hot_ns) < CACHE_ABS_THRESH_NS);
    }
    return r;
}

// ─── 2. Interrupt Timing Detector ─────────────────────────

struct IrqResult { bool anomaly; int64_t baseline; int64_t measured; int64_t deviation; };

static IrqResult detect_irq_timing() {
    IrqResult r = {false, 0, 0, 0};
    bind_cpu(0);
    const char *path = "/system/build.prop";
    char buf[256];

    int64_t base_sum = 0;
    for (int i = 0; i < IRQ_BASELINE_ITER; i++) {
        int fd = (int)bs_openat(-100, path, O_RDONLY | O_CLOEXEC, 0);
        if (fd < 0) continue;
        uint64_t t0 = bs_clock_ns();
        bs_read(fd, buf, sizeof(buf));
        uint64_t t1 = bs_clock_ns();
        bs_close(fd);
        base_sum += (int64_t)(t1 - t0);
    }
    r.baseline = base_sum / IRQ_BASELINE_ITER;

    int64_t meas_sum = 0;
    for (int i = 0; i < IRQ_MEASURE_ITER; i++) {
        uint64_t t0 = bs_clock_ns();
        int fd = (int)bs_openat(-100, path, O_RDONLY | O_CLOEXEC, 0);
        if (fd >= 0) {
            bs_read(fd, buf, sizeof(buf));
            bs_close(fd);
        }
        uint64_t t1 = bs_clock_ns();
        meas_sum += (int64_t)(t1 - t0);
    }
    r.measured = meas_sum / IRQ_MEASURE_ITER;
    r.deviation = r.measured - r.baseline;
    r.anomaly = r.deviation > IRQ_THRESHOLD_NS;
    return r;
}

// ─── 3. Binder Latency Detector ───────────────────────────

struct BinderSvcResult { const char *name; int64_t avg; int64_t min_lat; int64_t max_lat; bool anomaly; };
struct BinderResult { bool any_anomaly; int64_t global_avg; int64_t max_dev; int svc_count; };

static BinderResult detect_binder_latency() {
    BinderResult r = {false, 0, 0, 0};
    int fd = (int)bs_openat(-100, BINDER_DRIVER, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return r;

    binder_version ver;
    if (bs_ioctl(fd, BINDER_VERSION, (int64_t)&ver) < 0) { bs_close(fd); return r; }

    struct binder_transaction_data {
        uint64_t target_handle; uint64_t cookie; uint32_t code;
        uint32_t flags; int32_t sender_pid; uint32_t sender_euid;
        uint64_t data_size; uint64_t offsets_size;
        uint64_t data_ptr; uint64_t offsets_ptr;
    };

    uint8_t dummy[16] = {};
    binder_transaction_data tr = {};
    tr.code = 1;
    tr.data_size = sizeof(dummy);
    tr.data_ptr = (uint64_t)(uintptr_t)dummy;

    struct { uint32_t cmd; binder_transaction_data tr; } write_buf = {};
    write_buf.cmd = 1;
    write_buf.tr = tr;

    binder_write_read bwr = {};
    bwr.write_size = sizeof(write_buf);
    bwr.write_buffer = (uint64_t)(uintptr_t)&write_buf;
    uint32_t read_buf[32];
    bwr.read_size = sizeof(read_buf);
    bwr.read_buffer = (uint64_t)(uintptr_t)read_buf;

    int64_t total = 0;
    int64_t min_lat = INT64_MAX;
    int64_t max_lat = 0;
    int valid = 0;

    for (int i = 0; i < BINDER_ITER; i++) {
        uint64_t t0 = bs_clock_ns();
        bs_ioctl(fd, BINDER_WRITE_READ, (int64_t)&bwr);
        uint64_t t1 = bs_clock_ns();
        int64_t lat = (int64_t)(t1 - t0);
        if (lat > 0 && lat < 10000000) {
            total += lat;
            if (lat < min_lat) min_lat = lat;
            if (lat > max_lat) max_lat = lat;
            valid++;
        }
    }
    bs_close(fd);

    if (valid > 0) {
        r.global_avg = total / valid;
        r.svc_count = 1;
        // Check deviation against expected baseline (100-500us typical)
        r.any_anomaly = (r.global_avg > BINDER_ABS_NS);
        r.max_dev = r.global_avg;
    }
    return r;
}

// ─── 4. Pagefault Monitor ─────────────────────────────────

struct PfResult { bool anomaly; int64_t baseline_minor; int64_t baseline_major;
    int64_t mem_minor; int64_t mem_major; int64_t total; };

static PfResult detect_pagefault() {
    PfResult r = {false, 0, 0, 0, 0, 0};
    bind_cpu(0);

    // Baseline
    int64_t bmin = read_stat_field(9);
    int64_t bmaj = read_stat_field(11);
    bs_nanosleep(1000000);
    bmin = read_stat_field(9) - bmin;
    bmaj = read_stat_field(11) - bmaj;
    r.baseline_minor = bmin < 0 ? 0 : bmin;
    r.baseline_major = bmaj < 0 ? 0 : bmaj;

    // Memory pressure via large mmap
    int64_t pm_before_min = read_stat_field(9);
    int64_t pm_before_maj = read_stat_field(11);

    for (int i = 0; i < PF_MEM_ITER; i++) {
        size_t sz = 64 * 1024 * 1024;
        void *buf = (void*)(intptr_t)bs_mmap(nullptr, sz,
            MMAP_PROT_READ | MMAP_PROT_WRITE,
            MMAP_MAP_PRIVATE | MMAP_MAP_ANONYMOUS,
            -1, 0);
        if ((int64_t)(intptr_t)buf > 0 || buf == nullptr) continue;
        // Touch every page
        for (size_t j = 0; j < sz; j += 4096) {
            ((volatile char*)buf)[j] = 0xAA;
        }
        bs_munmap(buf, sz);
    }

    r.mem_minor = read_stat_field(9) - pm_before_min;
    r.mem_major = read_stat_field(11) - pm_before_maj;
    r.total = r.mem_minor + r.mem_major;

    int64_t minor_thresh = (int64_t)((double)r.baseline_minor * PF_MINOR_RATIO);
    if (minor_thresh < PF_MINOR_ABS) minor_thresh = PF_MINOR_ABS;
    int64_t major_thresh = (int64_t)((double)r.baseline_major * PF_MAJOR_RATIO);
    if (major_thresh < PF_MAJOR_ABS) major_thresh = PF_MAJOR_ABS;

    r.anomaly = (r.mem_minor > minor_thresh) || (r.mem_major > major_thresh);
    return r;
}

// ─── Plugin entry point ──────────────────────────────────

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 5;
    result.service_name = "Side-channel Analysis";
    result.success = true;
    result.confidence = 1.0f;

    bool any_anomaly = false;
    std::string report;

    // 1. Cache timing
    CacheResult cr = detect_cache_timing();
    report += "[Cache] hot=" + std::to_string(cr.hot_ns) + "ns cold=" +
              std::to_string(cr.cold_ns) + "ns ratio=" + std::to_string(cr.ratio);
    if (cr.anomaly) { report += " ANOMALY"; any_anomaly = true; }
    report += "; ";

    // 2. Interrupt timing
    IrqResult ir = detect_irq_timing();
    report += "[IRQ] base=" + std::to_string(ir.baseline) + "ns meas=" +
              std::to_string(ir.measured) + "ns dev=" + std::to_string(ir.deviation) + "ns";
    if (ir.anomaly) { report += " ANOMALY"; any_anomaly = true; }
    report += "; ";

    // 3. Binder latency
    BinderResult br = detect_binder_latency();
    report += "[Binder] avg=" + std::to_string(br.global_avg) + "ns";
    if (br.any_anomaly) { report += " ANOMALY"; any_anomaly = true; }
    report += "; ";

    // 4. Pagefault
    PfResult pr = detect_pagefault();
    report += "[Pagefault] minor=" + std::to_string(pr.mem_minor) + " major=" +
              std::to_string(pr.mem_major) + " baseline_minor=" +
              std::to_string(pr.baseline_minor);
    if (pr.anomaly) { report += " ANOMALY"; any_anomaly = true; }

    result.description = report;
    if (any_anomaly) {
        result.success = false;
        result.confidence = 0.7f;
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    5, "Side-channel Analysis", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }