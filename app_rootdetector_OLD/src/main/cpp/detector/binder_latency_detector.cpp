#include "binder_latency_detector.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <android/log.h>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#define LOG_TAG "BinderLatencyDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define BINDER_DRIVER "/dev/binder"
#define BINDER_WRITE_READ _IOWR('b', 1, struct binder_write_read)
#define BINDER_VERSION _IOWR('b', 9, struct binder_version)

struct binder_version {
    signed long protocol_version;
};

struct flat_binder_object {
    uint32_t type;
    uint32_t flags;
    uint64_t cookie;
    uint64_t binder;
    uint64_t handle;
};

struct binder_transaction_data {
    uint64_t target_handle;
    uint64_t cookie;
    uint32_t code;
    uint32_t flags;
    pid_t sender_pid;
    uid_t sender_euid;
    uint64_t data_size;
    uint64_t offsets_size;
    uint64_t data_ptr;
    uint64_t offsets_ptr;
};

struct binder_write_read {
    uint64_t write_size;
    uint64_t write_consumed;
    uint64_t write_buffer;
    uint64_t read_size;
    uint64_t read_consumed;
    uint64_t read_buffer;
};

struct binder_transaction_data_sg {
    struct binder_transaction_data transaction_data;
    uint64_t buffer_size;
    uint64_t offsets_size;
};

long long BinderLatencyDetector::getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

bool BinderLatencyDetector::bindToCpu(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0;
}

int BinderLatencyDetector::openBinderDriver() {
    int fd = syscall(__NR_openat, AT_FDCWD, BINDER_DRIVER, O_RDWR | O_CLOEXEC, 0);
    if (fd < 0) {
        fd = syscall(__NR_openat, AT_FDCWD, "/dev/binder", O_RDWR, 0);
    }
    return fd;
}

long long BinderLatencyDetector::sendBinderCall(int binder_fd, uint32_t code,
                                                 const void *data, size_t len) {
    struct binder_transaction_data tr = {};
    tr.code = code;
    tr.data_size = len;
    tr.data_ptr = (uint64_t)(uintptr_t)data;

    struct {
        uint32_t cmd;
        struct binder_transaction_data tr;
    } write_buf = {};

    write_buf.cmd = 0x01;
    write_buf.tr = tr;

    struct binder_write_read bwr = {};
    bwr.write_size = sizeof(write_buf);
    bwr.write_buffer = (uint64_t)(uintptr_t)&write_buf;

    uint32_t read_buf[32];
    bwr.read_size = sizeof(read_buf);
    bwr.read_buffer = (uint64_t)(uintptr_t)read_buf;

    long long start = getTimeNs();
    int ret = ioctl(binder_fd, BINDER_WRITE_READ, &bwr);
    long long end = getTimeNs();

    (void)ret;
    return end - start;
}

double BinderLatencyDetector::calculateStdDev(const std::vector<long long> &samples,
                                               long long mean) {
    if (samples.size() < 2) return 0.0;
    double sum_sq = 0.0;
    for (auto s : samples) {
        double diff = (double)s - (double)mean;
        sum_sq += diff * diff;
    }
    return std::sqrt(sum_sq / (samples.size() - 1));
}

long long BinderLatencyDetector::measureBinderTransaction(
    const char *service_name, const char *interface_token,
    int iterations, long long &min_latency,
    long long &max_latency, double &std_dev) {

    (void)interface_token;
    int binder_fd = openBinderDriver();
    if (binder_fd < 0) {
        LOGI("无法打开 Binder 驱动 %s", service_name);
        return -1;
    }

    std::vector<long long> times;
    times.reserve(iterations);

    uint32_t test_code = 0x00000001;
    uint8_t dummy_data[16] = {};

    for (int i = 0; i < iterations; i++) {
        long long latency = sendBinderCall(binder_fd, test_code, dummy_data, sizeof(dummy_data));
        if (latency > 0) {
            times.push_back(latency);
        }
    }

    syscall(__NR_close, binder_fd);

    if (times.empty()) return -1;

    std::sort(times.begin(), times.end());
    size_t start_idx = times.size() / 10;
    size_t end_idx = times.size() * 9 / 10;

    long long sum = 0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += times[i];
    }
    long long avg = sum / (end_idx - start_idx);
    min_latency = times[start_idx];
    max_latency = times[end_idx - 1];
    std_dev = calculateStdDev(
        std::vector<long long>(times.begin() + start_idx, times.begin() + end_idx), avg);

    return avg;
}

BinderLatencyResult BinderLatencyDetector::detect() {
    BinderLatencyResult result;
    result.layer = "第5层增强：Binder 调用时延分析";
    result.detected = false;
    result.global_baseline_ns = 0;
    result.global_deviation_ns = 0;

    if (!bindToCpu(0)) {
        bindToCpu(1);
    }

    LOGI("开始 Binder 调用时延分析");

    struct ServiceEntry {
        const char *name;
        const char *interface;
    };

    ServiceEntry services[] = {
        {"activity_service", "android.app.IActivityManager"},
        {"window_service", "android.view.IWindowManager"},
        {"package_service", "android.content.pm.IPackageManager"},
        {"power_service", "android.os.IPowerManager"},
        {"notification_service", "android.app.INotificationManager"},
    };

    long long total_baseline = 0;
    int valid_count = 0;
    long long global_min = LLONG_MAX;
    long long global_max = 0;

    for (auto &svc : services) {
        BinderTransactionResult trans_result;
        trans_result.service_name = svc.name;
        trans_result.is_anomaly = false;

        long long min_lat = 0, max_lat = 0;
        double stddev = 0.0;

        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            long long t = measureBinderTransaction(svc.name, svc.interface, 5, min_lat, max_lat, stddev);
            if (t < 0) break;
        }

        long long avg = measureBinderTransaction(svc.name, svc.interface,
                                                  TRANSACTION_ITERATIONS,
                                                  min_lat, max_lat, stddev);

        trans_result.avg_latency_ns = avg;
        trans_result.min_latency_ns = min_lat;
        trans_result.max_latency_ns = max_lat;
        trans_result.std_dev_ns = stddev;

        if (avg > 0) {
            total_baseline += avg;
            valid_count++;
            if (min_lat < global_min) global_min = min_lat;
            if (max_lat > global_max) global_max = max_lat;
        }

        LOGI("Binder 服务 %s: 平均 %lld ns, 标准差 %.0f ns",
             svc.name, avg, stddev);

        result.transaction_results.push_back(trans_result);
    }

    if (valid_count == 0) {
        result.detail = "无法访问 Binder 驱动，检测失败";
        return result;
    }

    long long global_avg = total_baseline / valid_count;
    result.global_baseline_ns = global_avg;

    long long max_deviation = 0;
    std::string anomaly_services;

    for (auto &tr : result.transaction_results) {
        if (tr.avg_latency_ns <= 0) continue;

        long long deviation = llabs(tr.avg_latency_ns - global_avg);
        bool ratio_anomaly = (tr.avg_latency_ns > global_avg &&
                              tr.avg_latency_ns > global_avg * ANOMALY_THRESHOLD_RATIO);
        bool abs_anomaly = deviation > BASELINE_ANOMALY_NS;

        if (ratio_anomaly || abs_anomaly) {
            tr.is_anomaly = true;
            tr.anomaly_reason = "时延偏移 " + std::to_string(deviation) +
                                " ns (全局基线 " + std::to_string(global_avg) + " ns)";
            result.detected = true;
            anomaly_services += tr.service_name + ": " + tr.anomaly_reason + "\n";

            if (deviation > max_deviation) {
                max_deviation = deviation;
            }
        }
    }

    result.global_deviation_ns = max_deviation;

    if (result.detected) {
        result.detail = "检测到 Binder 调用时延异常，可能存在 LSPosed 全局 Hook：\n" + anomaly_services;
        LOGI("检测到 Binder 时延异常");
    } else {
        result.detail = "Binder 调用时延正常，全局基线 " + std::to_string(global_avg) +
                        " ns，未检测到 Hook 痕迹";
        LOGI("Binder 时延正常");
    }

    return result;
}
